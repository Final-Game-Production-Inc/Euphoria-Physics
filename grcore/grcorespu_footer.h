				default:
					grcErrorf("unknown spu command %d at %p",any->command>>1,any);
					Assert(false);
					break;
			}
			SPU_BOOKMARK(0);
			// Point to next command
			any = nextAny;

#if 0
			// measure the high water mark at the end of each command
			pSpuGcmState->m_LookAheadStat6 = Max(pSpuGcmState->m_LookAheadStat6, (spuScratch - scratchSave));
#endif
		}
		else	// GPU command
		{
			SPU_BOOKMARK(0x1500);
			u32 *rawStart = (u32*) any;
			u32 *rawCur = rawStart;
			do {
				u32 gpuCmd = *rawCur;
				if (gpuCmd & 1)
					break;
				u32 thisWordSize = (gpuCmd & (CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_CALL | CELL_GCM_METHOD_FLAG_RETURN))? 1 : ((gpuCmd >> 18) & (0x1FFC>>2)) + 1;
				rawCur += thisWordSize;
				DRAWABLESPU_STATS_INC(GcmCommands);
			} while (rawCur < (u32*) stop);

			int wordSize = rawCur - rawStart;
			if (GCM_CONTEXT->current + wordSize > GCM_CONTEXT->end)
				gcmCallback(GCM_CONTEXT, wordSize);
			// Copy it
			memcpy(GCM_CONTEXT->current, rawStart, wordSize << 2);
			GCM_CONTEXT->current += wordSize;
			if (GCM_CONTEXT->current > GCM_CONTEXT->end)
				Quitf("Too many raw commands in segment %p > %p (wordSize=%u)", GCM_CONTEXT->current, GCM_CONTEXT->end, wordSize);
	
			// Point to next command
			any = (spuCmd_Any*) rawCur;
			SPU_BOOKMARK(0);
		}
	}

	// sysTimer footer;
#if SPU_GCM_FIFO
	// Try to copy only a subset of the 4k of constants if we can help it (preferably none of it)
	if (g_HighestConstant)
		sysDmaPut(grcFragmentProgram::sm_Constants + g_LowestConstant, pSpuGcmState->FragmentConstants + (g_LowestConstant << 4), (g_HighestConstant - g_LowestConstant) << 4, FIFOTAG);

	effectCache.Finalize();

#if 0
	pSpuGcmState->m_FullFlushes = effectCache.m_FullFlushes;
	pSpuGcmState->m_NumFlushed = effectCache.m_NumFlushed;
	pSpuGcmState->m_NumHits = effectCache.m_NumHits;
	pSpuGcmState->m_NumMisses = effectCache.m_NumMisses;
	pSpuGcmState->m_SizeAccumulator = effectCache.m_SizeAccumulator;
#endif

#if DRAWABLESPU_STATS
	int result;
	uint64_t eaStats = (uint64_t)pSpuGcmState->statsBuffer;
	Assertf(((u32)initialScratchSave & 127) == 0, "Spu scratch buffer isn't 128 byte aligned!");
	spuDrawableStats *statsBuffer = (spuDrawableStats*) initialScratchSave;
	CompileTimeAssert(sizeof(spuDrawableStats) == 128);					// can't get bigger than this without changing this code
	if (eaStats) do {
		cellDmaGetllar(statsBuffer,eaStats,0,0);
		result = cellDmaWaitAtomicStatus();

#define ACCUM(x)	statsBuffer->x += g_Stats.x;
#define ACCUM_CONTEXT(x)	statsBuffer->DrawCallsPerContext[x] += g_Stats.DrawCallsPerContext[x];
		ACCUM(GcmCommands);
		ACCUM(MacroCommands);
		ACCUM(MissingTechnique);
		ACCUM(ModelsCulled);
		ACCUM(ModelsDrawn);
		ACCUM(GeomsCulled);
		ACCUM(GeomsDrawn);
		ACCUM(DrawableDrawCalls);
		ACCUM(DrawableDrawSkinnedCalls);
		ACCUM(EntityDrawCalls);
		ACCUM(RingBufferUsed);
		ACCUM(GcmDrawCalls);
		ACCUM(GcmDrawIndices);
		ACCUM_CONTEXT(DCC_NO_CATEGORY);
		ACCUM_CONTEXT(DCC_PROPS);
		ACCUM_CONTEXT(DCC_VEG);
		ACCUM_CONTEXT(DCC_LOD);
		ACCUM_CONTEXT(DCC_SLOD1);
		ACCUM_CONTEXT(DCC_SLOD2);
		ACCUM_CONTEXT(DCC_SLOD3);
		ACCUM_CONTEXT(DCC_SLOD4);
		ACCUM_CONTEXT(DCC_PEDS);
		ACCUM_CONTEXT(DCC_VEHICLES);
#undef ACCUM

		cellDmaPutllc(statsBuffer,eaStats,0,0);
		result = cellDmaWaitAtomicStatus();
	} while (result & 1);
#endif

	// Service audio blits
	if (!pSpuGcmState->isGPADCapturing)	{
		const u32 gpuMemCpyEa = pSpuGcmState->GpuMemCpyAddr;
		gpuMemCpy::Job nextJob;
		u32 doneSrcOffset;
		while (gpuMemCpy::Pop(gpuMemCpyEa,&nextJob,&doneSrcOffset,FIFOTAG)) {
			qword *blit = ReserveMethodSizeAligned(GCM_CONTEXT,8);

			// Construct the blit, based on this original code:
			// uint32_t *current = data->current;
			// CELL_GCM_METHOD_COPY2D_SET_CONTEXT_DMA_BUFFER(current, CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER, CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER);
			// CELL_GCM_METHOD_COPY2D_OFFSET_PITCH_LINE_FORMAT_NOTIFY(current, 
			//	gcm::LocalOffset(j.src),gcm::MainOffset(j.dest),0,0,j.bytes,1,1,1,0);
			// CELL_GCM_METHOD_COPY2D_OFFSET_OUT(current, 0);
			blit[0] = MAKE_QWORD(CELL_GCM_METHOD(CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_IN, 2), CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER | nextJob.SrcLoc, CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER | nextJob.DestLoc, CELL_GCM_METHOD(CELL_GCM_NV0039_OFFSET_IN, 8));
			blit[1] = MAKE_QWORD_ZZ(nextJob.SrcOffset, nextJob.DestOffset);
			blit[2] = MAKE_QWORD(nextJob.Bytes, 1, 0x101, 0);
			blit[3] = MAKE_QWORD_ZZ(CELL_GCM_METHOD(CELL_GCM_NV0039_OFFSET_OUT, 1), 0);	// last two words are NOP's available for other use.

			// Do the done blit now.  This blit is always from vram to main memory, 4 bytes.
			blit[4] = MAKE_QWORD(CELL_GCM_METHOD(CELL_GCM_NV0039_SET_CONTEXT_DMA_BUFFER_IN, 2), CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER, CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER, CELL_GCM_METHOD(CELL_GCM_NV0039_OFFSET_IN, 8));
			blit[5] = MAKE_QWORD_ZZ(doneSrcOffset, nextJob.DoneOffset);
			blit[6] = MAKE_QWORD(4, 1, 0x101, 0);
			blit[7] = MAKE_QWORD_ZZ(CELL_GCM_METHOD(CELL_GCM_NV0039_OFFSET_OUT, 1), 0);	// last two words are NOP's available for other use.
		}
	}

	// Flush the gcm fifo back to main memory
	gcmCallback(GCM_CONTEXT,0);
	// ...and make sure any output DMA is finished.
	// TODO: Use the output buffer so that SPURS will do the wait on our behalf.
	sysDmaWaitTagStatusAll(1<<FIFOTAG || 1 << spuBindTag);

#if DRAWABLESPU_STATS && 0
	spu_printf("vtx %u/%u; texpage %u/%u; rast %u/%u; depthstencil %u/%u; blend %u/%u\n",
		s_vtxDeclCache.hits,s_vtxDeclCache.misses,
		s_texturePageCache.hits,s_texturePageCache.misses,
		grcStateBlock::s_RasterizerStateCache.hits,grcStateBlock::s_RasterizerStateCache.misses,
		grcStateBlock::s_DepthStencilStateCache.hits,grcStateBlock::s_DepthStencilStateCache.misses,
		grcStateBlock::s_BlendStateCache.hits,grcStateBlock::s_BlendStateCache.misses);
#endif

#if !__PROFILE && !__FINAL
	jobStart = ~spu_readch(SPU_RdDec) - jobStart;
	pSpuGcmState->JobTime += jobStart;
#endif
#if SYS_DMA_TIMING
	pSpuGcmState->DmaTime += g_DmaWaitTime;
#endif
#endif	// SPU_GCM_FIFO
