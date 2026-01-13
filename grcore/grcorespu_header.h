#if !__PROFILE && !__FINAL
	uint32_t jobStart = ~spu_readch(SPU_RdDec);
#endif

	spuScratch = (char*) taskParams.Scratch.Data;
	spuScratchTop = (char*) taskParams.Scratch.Data + taskParams.Scratch.Size;
	spuCmd_Any *any = (spuCmd_Any*) taskParams.Input.Data;
	spuCmd_Any *stop = (spuCmd_Any*) ((char*)taskParams.Input.Data + taskParams.Input.Size);

#if SPU_GCM_FIFO
	// sysTimer header;

	// A bit cheesy, but wait for all DMA's to finish, so that previous job's output is known to be done by now.
	sysDmaWaitTagStatusAll(SYS_DMA_MASK_ALL);

	// Retrieve global state; written to output buffer so it will automatically
	// be copied back when the job is done.
	pSpuGcmState = (spuGcmState*)taskParams.Output.Data;
	sysDmaGetAndWait(pSpuGcmState,taskParams.UserData[0].asUInt,sizeof(spuGcmState),TAG);
	
	// Remember the globals heap offset active when the job was created so we know how much to overwrite.
	g_SamplerStateCount = taskParams.UserData[1].asUInt;
	int samplerSize = g_SamplerStateCount * sizeof(grcSamplerState);
	g_SamplerStates = (grcSamplerState*) spuScratch; spuScratch += samplerSize;
	sysDmaGetAndWait(g_SamplerStates, pSpuGcmState->SamplerStateStore, samplerSize, TAG);

	// grcEffect::sm_Globals.m_Count = taskParams.UserData[2].asUInt;

	// EDGE requires fifo to be 128-byte-aligned for stall hole filling logic to work right.
	spuScratch = (char*)(uint32_t(spuScratch + 127) & ~127);

	// sysDmaGet(&grcEffect::sm_Globals, pSpuGcmState->Globals, sizeof(grcParameter) * grcEffect::sm_Globals.GetCount(), TAG);
	grcFragmentProgram::sm_Constants = (u128*) spuScratch;
	spuScratch += 4096;
	sysDmaGetAndWait(grcFragmentProgram::sm_Constants, pSpuGcmState->FragmentConstants, 4096, TAG);
	// Remember the lowest and highest ranges we actually wrote out
	g_LowestConstant = 256;
	g_HighestConstant = 0;

	gcmFlushOffset = 0;
	ctxt.begin = ctxt.current = (uint32_t*) spuScratch;
	ctxt.end = (uint32_t*) (spuScratch + SPU_FIFO_SEGMENT_SIZE - sizeof(uint32_t));
	spuScratch += SPU_FIFO_SEGMENT_SIZE;
	ctxt.callback = gcmCallback;

	// Mark previous fifo segment as being free (at the beginning of the new segment)
	// (when we flushed the previous segment at the end of the previous job, this
	// got left behind, unsent, in the last fifo segment)
	pSpuGcmState->CommandFifo.Free(pSpuGcmState->NextSegment,&ctxt);

	sysDmaWaitTagStatusAll(1<<TAG);
	// grcDisplayf("header %fus",header.GetUsTime()); -- measured at about 3 microseconds
#endif

	const int ucodeSize = SPU_FRAGMENT_PROGRAM_CACHE_SIZE;
	u128 *ucodeBuffer = (u128*) spuScratch;
	spuScratch += ucodeSize;
	grcFragmentProgram::InitCache(ucodeBuffer,ucodeSize);

	grcEffectCache effectCache;
	s_effectCache = &effectCache;
	effectCache.Init(taskParams.UserData[3].asUInt);
	s_vtxDeclCache.Init();
#if !USE_PACKED_GCMTEX
	s_texturePageCache.Init();
#endif
	grcStateBlock::s_DepthStencilStateCache.Init();
	grcStateBlock::s_RasterizerStateCache.Init();
	grcStateBlock::s_BlendStateCache.Init();

	UpdateForceShader();

	g_RenderStateBlockMask = pSpuGcmState->RenderStateBlockMask;

	/* for (u32 *t=(u32*)any; t<(u32*)stop; t+=4)
		grcDisplayf("%p: %8x %8x %8x %8x",t,t[0],t[1],t[2],t[3]); */

	// spuCmd_Any *lastAny = NULL, *lastStop = NULL;

	// Command list utilities
	struct CLUtils
	{
		__forceinline static spuCmd_Any* GetNext(spuCmd_Any* current)
		{
			return (spuCmd_Any*)((char*)current + current->size);
		}

		enum {
			AnyCommand = 0xFFFFFFFF
		};

		// Given a pointer to the next command, returns it if it matches the command1 command, otherwise returns NULL
		__forceinline static spuCmd_Any* LookAhead(spuCmd_Any* nextAny, spuCmd_Any* stop, u32 command1)
		{
			if (nextAny >= stop)
			{
				return NULL;
			}
			// could also check nextAny->command == (command1 << 1) | 0x1 if its faster;
			if (command1 != AnyCommand && ((nextAny->command & 1) == 0 || (nextAny->command >> 1) != command1))
			{
				return NULL;
			}
			return nextAny;
		}

		// Given a pointer to the next command and 3 command enums, returns the 'command2' spuCmd if the next 2 commands
		// had matching enums - otherwise returns NULL.
		__forceinline static spuCmd_Any* LookAhead(spuCmd_Any* nextAny, spuCmd_Any* stop, u32 command1, u32 command2)
		{
			nextAny = LookAhead(nextAny, stop, command1);
			if (!nextAny) return NULL;
			nextAny = GetNext(nextAny);
			nextAny = LookAhead(nextAny, stop, command2);
			return nextAny;
		}
	
		// Given a pointer to the next command and 3 command enums, returns the 'command3' spuCmd if the next 3 commands
		// had matching enums - otherwise returns NULL.
		__forceinline static spuCmd_Any* LookAhead(spuCmd_Any* nextAny, spuCmd_Any* stop, u32 command1, u32 command2, u32 command3)
		{
			nextAny = LookAhead(nextAny, stop, command1);
			if (!nextAny) return NULL;
			nextAny = GetNext(nextAny);
			nextAny = LookAhead(nextAny, stop, command2);
			if (!nextAny) return NULL;
			nextAny = GetNext(nextAny);
			nextAny = LookAhead(nextAny, stop, command3);
			return nextAny;
		}
	};

	// This is actually a PPU-side address;
	// sysCriticalSectionTokenSpu *pCritSec = (sysCriticalSectionTokenSpu*)pSpuGcmState->KnownRefToken;
	// uint32_t eaKnownRefPool = pSpuGcmState->KnownRefPool;

#if SYS_DMA_VALIDATION
	sysDmaContext = pSpuGcmState->FaultContext;
#endif

#if DRAWABLESPU_STATS
	memset(&g_Stats,0,sizeof(g_Stats));
	pSpuGcmState->isCapturingStats = false;
#endif

	// drawablespu's SetForceShader command allocates semi-persistent storage from the scratch buffer
	// All other commands recycle all their scratch storage usage between each command
	char *initialScratchSave = spuScratch;
	char *scratchSave = initialScratchSave;
	while (any < stop)
	{
		spuScratch = scratchSave;
		// grcDisplayf("%p: %x",any,*(u32*)any);
		if (any->command & 1)		// SPU command
		{
			spuCmd_Any *nextAny = (spuCmd_Any*)((char*)any + any->size);
			DRAWABLESPU_STATS_INC(MacroCommands);

			Assert(any->size);
			// Process the SPU command
			switch (any->command >> 1)
			{
