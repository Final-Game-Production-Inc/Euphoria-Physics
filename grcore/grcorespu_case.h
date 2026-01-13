BEGIN_SPU_SIMPLE_COMMAND(grcEffect__ClearCachedState)
	SPU_BOOKMARK(0x1170);
	grcEffect::ClearCachedState();
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__SetGlobalFloatCommon)
	SPU_BOOKMARK(0x1180);

	//grcDisplayf("SetGlobalFloatCommon va=%x fa=%x t=%d data=%p->%p size=%u",global.VertexAddress,global.FragmentAddress,
	//	 global.Type,cmd->payload,global.Data,cmd->size-4);
	if (cmd->Usage & USAGE_VERTEXPROGRAM)
		cellGcmSetVertexProgramParameterBlock(GCM_CONTEXT,cmd->Register,cmd->qwCount, union_cast<float*>(cmd->alignedPayload));
	if (cmd->Usage & USAGE_FRAGMENTPROGRAM)
		grcFragmentProgram::SetParameter(cmd->Register,union_cast<float*>(cmd->alignedPayload),cmd->qwCount);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__SetVarFloatCommon)
	SPU_BOOKMARK(0x1190);
	effectCache.Flush(cmd->effect); // Ideally would write through here
	CopyInstanceData((uint64_t)cmd->dest,cmd->alignedPayload,cmd->stride,cmd->count);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__SetVar_Color32)
	effectCache.Flush(cmd->effect); // Ideally would write through here
	uint64_t dest = (uint64_t) cmd->dest;		// in ppu address space
	int count = cmd->subcommand;
	Assert(count);
	const qword color32_to_rgba = MAKE_SHUFFLE(00,00,00,A1,00,00,00,A2,00,00,00,A3,00,00,00,A0);		// YZWX
	__vector4 oo255 = { 1.0f/255, 1.0f/255, 1.0f/255, 1.0f/255 };
	const u32 *src = cmd->colors;
	do {
		qword scalarInt = si_from_int(*src++);
		vec_uint4 unpackedInt = (vec_uint4) si_shufb(scalarInt, scalarInt, color32_to_rgba);
		vec_float4 unscaledFloat = spu_convtf(unpackedInt, 0);
		vec_float4 finalFloat = spu_mul(unscaledFloat, oo255);
		sysDmaPutAndWait(&finalFloat,dest,16,spuGetTag);
		dest += 16;
	} while (--count);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__SetVarFloatCommonByRef)
	effectCache.Flush(cmd->effect);
	int size = ((cmd->stride * cmd->count) + 15) & ~15;
	sysDmaGetAndWait(spuScratch,(uint64_t)cmd->src,size,spuGetTag);
	CopyInstanceData((uint64_t)cmd->dest,spuScratch,cmd->stride,cmd->count);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__SetGlobalVar_grcTexture)
SPU_BOOKMARK(0x11a0);
	//grcDisplayf("SetTexture va=%x fa=%x t=%d tex=%p rsct%d rs=%p",global.VertexAddress,global.FragmentAddress,
	//	global.Type,cmd->textureObject,global.DataSize,global.Data);
	grcTextureObject *texObj = NULL;
#if USE_PACKED_GCMTEX
	if (cmd->textureHandle) {
		texObj = (grcTextureObject*) spuScratch;
		spuScratch += SPU_TEXTURE_SIZE;
		sysDmaGetAndWait(texObj, (uint64_t) (pSpuGcmState->PackedTextureArray + cmd->textureHandle), SPU_TEXTURE_SIZE, spuGetTag);
	}
#else
	if (cmd->texture) {
		spuTexture *gcmTex = (spuTexture*) spuScratch;
		spuScratch += SPU_TEXTURE_SIZE;
		sysDmaGetAndWait(gcmTex, (uint64_t) cmd->texture, SPU_TEXTURE_SIZE, spuGetTag);
		texObj = &gcmTex->GetGcmTexture();
	}
#endif
	if (cmd->Usage & USAGE_VERTEXPROGRAM)
		grcVertexProgram::SetTexture(cmd->Register,texObj,cmd->SamplerStateSet);
	if (cmd->Usage & USAGE_FRAGMENTPROGRAM)
		grcFragmentProgram::SetTexture(cmd->Register,texObj,cmd->SamplerStateSet);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__SetVar_grcTexture)
	// Using this size as part of the overwrite of the command
	CompileTimeAssert(sizeof(spuCmd_grcEffect__SetVar_grcTexture)>=16);
	SPU_BOOKMARK(0x11b0);
	effectCache.Flush(cmd->effect); // Ideally just write through here
	uint32_t eaTexturePtr = (uint32_t)cmd->data;
	// either the command is the first one and its aligned, or its further along, and all previous commands are destroyable
#	if PGHANDLE_REF_COUNT
#		if USE_PACKED_GCMTEX
#		else
			pgBase **const oldTexHandle = (pgBase**)sysDmaGetUInt32(eaTexturePtr, spuBindTag);
			if (Likely(oldTexHandle))
			{
				const u32 oldTexHandleIndex = oldTexHandle - PPU_SYMBOL(g_pgHandleArray);
				ASSERT_ONLY(u16 updated =) sysInterlockedDecrement(oldTexHandleIndex + PPU_SYMBOL(g_pgHandleRefCountArray));
				Assertf((u16)(updated+1), "underflow");
			}
#		endif
#	endif
	qword* buffer = (qword*)(uint32_t(cmd) & ~15);
#if USE_PACKED_GCMTEX
	*buffer = (qword)spu_splats(cmd->textureHandle);
#else
	*buffer = (qword)spu_splats((uint32_t)cmd->textureHandle.ptr);
#endif
	// Fenced put so that multiple grcEffect__SetVar_grcTexture commands are orderred correctly.
	::rage::_sysDmaSmall(SYS_DMA_PUTF, sysDmaEa2Ls(eaTexturePtr, buffer), eaTexturePtr, sizeof(uint32_t), spuBindTag);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__Bind)
	SPU_BOOKMARK(0x11c0);

#if __BANK
// NOTE: temporary debug data to gather info on deleted textures
// before crashing on drawable spu
	ppuCmdAddr = (u32)cmd;
	ppuEffectAddr = (u32)cmd->effect;
#endif

	// effectCache.Get will wait on spuBindTag so that any of the puts from
	// grcEffect__SetVar_grcTexture will have completed before being got, IF the
	// effect is not already cached.  Will not wait on spuBindTag if the effect
	// is already cached, but that is ok, as there should be nothing in that
	// case that we need to wait on (cache is invalidated before doing dma puts
	// to the effect).
	grcEffect *effect = effectCache.Get(cmd->effect);
	cmd->effect = effect;
	unsigned technique = cmd->technique;
	unsigned passIndex = cmd->passIndex;
	BANK_ONLY( g_MinRegisterCount = cmd->minRegisterCount );

	grcInstanceData *instanceData = cmd->instanceData;
	spuGet(instanceData);
	instanceData->SpuGet(effect,technique,passIndex);
	effect->Bind((grcEffectTechnique)technique,passIndex,*instanceData);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__UnBind)
	SPU_BOOKMARK(0x11e0);
	cmd->effect = effectCache.Get(cmd->effect);
	cmd->effect->UnBind((grcEffectTechnique)cmd->technique,cmd->passIndex);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcViewport__RegenerateDevice)
	SPU_BOOKMARK(0x11f0);
	pSpuGcmState->World = cmd->world;
	pSpuGcmState->View = cmd->view;
	pSpuGcmState->Proj = cmd->proj;
	pSpuGcmState->FrustumLRTB = cmd->frustum;
	pSpuGcmState->LocalLRTB.Transform(cmd->frustum,cmd->world);
	spuMatrix43 &ew = *union_cast<spuMatrix43*>( &pSpuGcmState->EdgeWorld );
	ew.Transpose(cmd->world);

	spuMatrix44 &dest = *union_cast<spuMatrix44*>( pSpuGcmState->EdgeInfo.viewProjectionMatrix );
#if HACK_GTA4
	if(spuGcmShadowWarpEnabled(pSpuGcmState->shadowType))
	{
		dest.Transpose(pSpuGcmState->shadowMatrix);
	}
	else
#endif // HACK_GTA4
	{
		spuMatrix44 temp;
		temp.Transform(cmd->proj,cmd->view);
		dest.Transpose(temp);
	}

	cellGcmSetVertexProgramParameterBlock(GCM_CONTEXT,0,16, union_cast<float*>(&cmd->world));
	grcFragmentProgram::SetParameter(0,union_cast<float*>(&cmd->world),16);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcViewport__SetWindow)
	SPU_BOOKMARK(0x1200);
	EdgeGeomViewportInfo &vi = pSpuGcmState->EdgeInfo;
	for(int i=0;i<4;i++) vi.scissorArea[i] = cmd->scissorArea[i];
	cellGcmSetScissor(GCM_CONTEXT,cmd->scissorArea[0],cmd->scissorArea[1],cmd->scissorArea[2],cmd->scissorArea[3]);
	cellGcmSetViewport(GCM_CONTEXT,cmd->scissorArea[0],cmd->scissorArea[1],cmd->scissorArea[2],cmd->scissorArea[3],cmd->depthRange[0],cmd->depthRange[1],cmd->viewportScales,cmd->viewportOffsets);
	vi.depthRange[0] = cmd->depthRange[0];
	vi.depthRange[1] = cmd->depthRange[1];
	for(int i=0;i<3;i++) vi.viewportScales[i] = cmd->viewportScales[i];
	for(int i=0;i<3;i++) vi.viewportOffsets[i] = cmd->viewportOffsets[i];
	vi.sampleFlavor = cmd->subcommand;
END_SPU_COMMAND


BEGIN_SPU_COMMAND(grcDevice__SetScissor)
	EdgeGeomViewportInfo &vi = pSpuGcmState->EdgeInfo;
	for(int i=0;i<4;i++) vi.scissorArea[i] = cmd->scissorArea[i];
	cellGcmSetScissor(GCM_CONTEXT,cmd->scissorArea[0],cmd->scissorArea[1],cmd->scissorArea[2],cmd->scissorArea[3]);
END_SPU_COMMAND

BEGIN_SPU_SIMPLE_COMMAND(grcState__SetLightingMode)
	SPU_BOOKMARK(0x1210);
	pSpuGcmState->LightingMode = any->subcommand;
END_SPU_COMMAND

#if 0
BEGIN_SPU_SIMPLE_COMMAND(grcEffect__SetCullMode)
	SPU_BOOKMARK(0x1220);
	pSpuGcmState->EdgeCullMode = any->subcommand;
END_SPU_COMMAND
#endif

BEGIN_SPU_SIMPLE_COMMAND(grcEffect__SetEdgeViewportCullEnable)
	SPU_BOOKMARK(0x1230);
	pSpuGcmState->EdgeViewportCullEnable = any->subcommand;
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcDevice__WriteStateStruct)
	SPU_BOOKMARK(0x1240);
	memset((char*)pSpuGcmState+cmd->offset, cmd->value, cmd->count+1);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcDevice__Jump)
	SPU_BOOKMARK(0x1250);
	// Make sure there's room for the jump there and back before flushing
	if (GCM_CONTEXT->current + 2 > GCM_CONTEXT->end)
		gcmCallback(GCM_CONTEXT,2);
	// Jump into client buffer
	cell::Gcm::UnsafeInline::cellGcmSetJumpCommand(GCM_CONTEXT,cmd->jumpLocalOffset);
	// Compute the appropriate address in the final GCM buffer
	uint32_t jump = CELL_GCM_METHOD_FLAG_JUMP | ((uint32_t)GCM_CONTEXT->current - (uint32_t)GCM_CONTEXT->begin) + pSpuGcmState->NextSegmentOffset;
	sysDmaPutUInt32(jump, (uint64_t) cmd->jumpBack, TAG);
	sysDmaWaitTagStatusAll(1<<TAG);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__RenderStateBlockMask)
	SPU_BOOKMARK(0x1260);
	CompileTimeAssert(sizeof(pSpuGcmState->RenderStateBlockMask) == sizeof(g_RenderStateBlockMask));
	CompileTimeAssert(sizeof(g_RenderStateBlockMask) == sizeof(cmd->Mask));
	pSpuGcmState->RenderStateBlockMask = g_RenderStateBlockMask = cmd->Mask;
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcDevice__ChangeTextureRemap)
	SPU_BOOKMARK(0x1270);
#if USE_PACKED_GCMTEX
	sysDmaPutUInt8(cmd->subcommand,cmd->texture + 2,TAG);	// we're modifying a byte within a pre-shifted word, so cannot use offsetof here
	sysDmaPutUInt16(cmd->remap,cmd->texture + offsetof(PackedCellGcmTexture,remap),TAG);
#else
	sysDmaPutUInt8(cmd->subcommand,cmd->texture + offsetof(CellGcmTexture,format),TAG);
	sysDmaPutUInt32(cmd->remap,cmd->texture + offsetof(CellGcmTexture,remap),TAG);
#endif
END_SPU_COMMAND

#if 0
BEGIN_SPU_COMMAND(grcDevice__RunCommandBuffer)
	SPU_BOOKMARK(0x1280);
	// grcDisplayf("command buffer at %p",cmd->commandBuffer);
	if (lastAny)
		Quitf("command buffer stack overflow");
	lastAny = nextAny;
	lastStop = stop;
	nextAny = (spuCmd_Any*)scratchSave;
	sysDmaGet(nextAny,cmd->addrAndSize & ~(COMMAND_BUFFER_SEGMENT_SIZE-1),(cmd->addrAndSize & (COMMAND_BUFFER_SEGMENT_SIZE-1)) << 4,TAG);
	scratchSave += COMMAND_BUFFER_SEGMENT_SIZE;
	sysDmaWaitTagStatusAll(1<<TAG);
	// Shouldn't ever reach this, because there will be NextCommandBufferSegment commands in there.
	stop = (spuCmd_Any*)((char*)nextAny + COMMAND_BUFFER_SEGMENT_SIZE);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcDevice__NextCommandBufferSegment)
	SPU_BOOKMARK(0x1290);
	nextAny = (spuCmd_Any*)(scratchSave - COMMAND_BUFFER_SEGMENT_SIZE);
	sysDmaGet(nextAny,cmd->addrAndSize & ~(COMMAND_BUFFER_SEGMENT_SIZE-1),(cmd->addrAndSize & (COMMAND_BUFFER_SEGMENT_SIZE-1)) << 4,TAG);
	sysDmaWaitTagStatusAll(1<<TAG);
END_SPU_COMMAND

BEGIN_SPU_SIMPLE_COMMAND(grcDevice__ReturnFromCommandBuffer)
	SPU_BOOKMARK(0x12a0);
	if (!lastAny)
		Quitf("command buffer stack underflow");
	else {
		nextAny = lastAny;
		stop = lastStop;
		scratchSave -= COMMAND_BUFFER_SEGMENT_SIZE;
		lastAny = NULL;
	}
END_SPU_COMMAND

#endif

BEGIN_SPU_COMMAND(grcDevice__DrawPrimitive)
SPU_BOOKMARK(0x12b0);
	spuVertexDeclaration *vd = s_vtxDeclCache.Get(cmd->decl);
	DrawPrimitive(vd,cmd->subcommand,cmd->vertexData,cmd->startVertex,cmd->vertexCount);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcDevice__DrawIndexedPrimitive)
	SPU_BOOKMARK(0x12c0);
	spuVertexDeclaration *vd = s_vtxDeclCache.Get(cmd->decl);
	DrawIndexedPrimitive(vd,cmd->subcommand,cmd->vertexData,cmd->indexData,cmd->indexCount);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcDevice__DrawInstancedPrimitive)
	SPU_BOOKMARK(0x12c8);
	grcStateBlock::Flush();
	spuVertexDeclaration *vd = s_vtxDeclCache.Get(cmd->decl);
	BindVertexDeclaration(GCM_CONTEXT,pSpuGcmState,vd,cmd->vertexData,g_VertexShaderInputs);
	const float *data = (float*) spuScratch;
	sysDmaGetAndWait(data,(uint64_t)cmd->instData, (cmd->instCount * cmd->elemSizeQW) << 4,spuGetTag);
	for (int i=0; i<cmd->instCount; i++, data += (cmd->elemSizeQW << 2)) {
		cellGcmSetVertexProgramParameterBlock(GCM_CONTEXT,INSTANCE_SHADER_CONSTANT_SLOT,cmd->elemSizeQW,data);
		myCellGcmSetDrawIndexArray(GCM_CONTEXT,cmd->subcommand,cmd->indexCount,cmd->indexData);
		DRAWABLESPU_STATS_INC(GcmDrawCalls);
		DRAWABLESPU_STATS_ADD(GcmDrawIndices,cmd->indexCount);
	}
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcDevice__SetSurface)
	SPU_BOOKMARK(0x12d0);
	pSpuGcmState->SurfaceColorFormat = cmd->colorFormat;
	pSpuGcmState->SurfaceDepthFormat = cmd->depthFormat;
	pSpuGcmState->SurfaceDepthFormatType = cmd->depthFormatType;
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcGeometryJobs__SetOccluders)
	pSpuGcmState->EdgeOccluderQuads = cmd->occluderQuads;
	pSpuGcmState->EdgeOccluderQuadCount = cmd->subcommand;
END_SPU_COMMAND

BEGIN_SPU_SIMPLE_COMMAND(grcGeometryJobs__SetEdgeNoPixelTestEnable)
	pSpuGcmState->EdgeInfo.noPixelMask = any->subcommand;
END_SPU_SIMPLE_COMMAND

#if __BANK
BEGIN_SPU_COMMAND(grcGeometryJobs__SetEdgeCullDebugFlags)
	pSpuGcmState->EdgeCullDebugFlags = cmd->flags;
END_SPU_COMMAND
#endif // __BANK

BEGIN_SPU_COMMAND(grcDevice__SetPixelShaderConstantF)
	grcFragmentProgram::SetParameter(cmd->subcommand,(float*)cmd->alignedPayload,cmd->size>>4);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcDevice__SetEdgeClipPlane)
	pSpuGcmState->EdgeClipPlanes[cmd->subcommand] = cmd->plane;
END_SPU_COMMAND

BEGIN_SPU_SIMPLE_COMMAND(grcDevice__SetEdgeClipPlaneEnable)
	pSpuGcmState->EdgeClipPlaneEnable = any->subcommand;
	if (any->subcommand == 0x00) // disabled clip planes must be zero, so when we're done clear them all
	{
		for (int i = 0; i < EDGE_NUM_MODEL_CLIP_PLANES; i++)
		{
			pSpuGcmState->EdgeClipPlanes[i] = (__vector4)si_il(0);
		}
	}
END_SPU_COMMAND

#if !__FINAL
BEGIN_SPU_SIMPLE_COMMAND(grmModel__SetCullerDebugFlags)
	pSpuGcmState->CullerDebugFlags = any->subcommand;
END_SPU_COMMAND

BEGIN_SPU_SIMPLE_COMMAND(grcEffect__SetFragStripDebugFlags)
	pSpuGcmState->FragmentStrippingFlags = any->subcommand;
END_SPU_COMMAND
#endif // !__FINAL

BEGIN_SPU_SIMPLE_COMMAND(grmModel__SetCullerAABB)
	pSpuGcmState->CullerAABB = any->subcommand&0x01;
END_SPU_COMMAND

#if !__FINAL
BEGIN_SPU_COMMAND(grcDevice__PushFaultContext)
	Assert(pSpuGcmState->FaultIndex < spuGcmState::FaultStackSize);
	pSpuGcmState->FaultStack[pSpuGcmState->FaultIndex++] = pSpuGcmState->FaultNext;

	Assert((pSpuGcmState->FaultNext + cmd->subcommand) < spuGcmState::FaultContextSize);
	memcpy(pSpuGcmState->FaultContext + pSpuGcmState->FaultNext,cmd->labelText,cmd->subcommand);
	pSpuGcmState->FaultNext += cmd->subcommand;

	Assert((pSpuGcmState->FaultNext + 4) < spuGcmState::FaultContextSize);
	strcpy(pSpuGcmState->FaultContext + pSpuGcmState->FaultNext, " / ");
	pSpuGcmState->FaultNext += 3;
END_SPU_COMMAND

BEGIN_SPU_SIMPLE_COMMAND(grcDevice__PopFaultContext)
	Assert(pSpuGcmState->FaultIndex > 0);
	pSpuGcmState->FaultNext -= 3;
	pSpuGcmState->FaultContext[pSpuGcmState->FaultNext] = '\0';
	pSpuGcmState->FaultNext = pSpuGcmState->FaultStack[--(pSpuGcmState->FaultIndex)];
END_SPU_SIMPLE_COMMAND
#endif //!__FINAL...

BEGIN_SPU_COMMAND(grcDevice__CreateGraphicsJob)
	GEOMETRY_JOBS.AddOtherJob(GCM_CONTEXT,const_cast<char*>(cmd->JobStart), const_cast<char*>(cmd->JobSize), cmd->InputSize, cmd->InputData, cmd->ScratchSize, cmd->SpuStackSize, cmd->subcommand, cmd->asInt);
END_SPU_COMMAND

#if HACK_GTA4
BEGIN_SPU_COMMAND(grcEffect__CopyByte)
	u32 src	= (u32)cmd->srcPtr;
	u32 dst	= (u32)cmd->dstPtr;

	// if subcommand!=0, then srcPtr is treated as direct u8 value rather than ptr
	u8 data8 = cmd->subcommand? u8(src) : sysDmaGetUInt8(src, 0);
	sysDmaPutUInt8(data8, dst, spuGetTag);
END_SPU_COMMAND
#endif //HACK_GTA4...

#if DRAWABLESPU_STATS
BEGIN_SPU_COMMAND(grcEffect__FetchStats)
	pSpuGcmState->statsBuffer = cmd->dstPtr;
END_SPU_COMMAND
#endif

BEGIN_SPU_COMMAND(grcStateBlock__SetDepthStencilState)
	grcStateBlock::SetDepthStencilState(cmd->handle,cmd->subcommand);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcStateBlock__SetRasterizerState)
	grcStateBlock::SetRasterizerState(cmd->handle);
END_SPU_COMMAND

#if !__FINAL
BEGIN_SPU_SIMPLE_COMMAND(grcStateBlock__SetWireframeOverride)
	WireframeOverride = any->subcommand;
END_SPU_COMMAND
#endif

BEGIN_SPU_COMMAND(grcStateBlock__SetBlendState)
	grcStateBlock::SetBlendState(cmd->handle,cmd->argb,cmd->multisample,cmd->subcommand);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__SetSamplerState)
	effectCache.Flush(cmd->effect); // Ideally just write through here
	char tempStructure[sizeof(grcInstanceData)];
	grcInstanceData *temp = (grcInstanceData*)tempStructure;
	sysDmaGetAndWait(temp,(uint64_t)cmd->instanceData,sizeof(grcInstanceData),spuGetTag);

	// Determine address of the sampler state set(note -- we're intentionally dereferencing Data, 
	// which is a PPU-side pointer, solely to take the address of the resulting array access)
	uint64_t eaSamplerStateSet = (uint64_t)&(temp->Entries[cmd->subcommand].SamplerStateSet);
	sysDmaPutUInt8((uint32_t)cmd->handle,eaSamplerStateSet,spuGetTag);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__PushSamplerState)
	effectCache.Flush(cmd->effect); // Ideally just write through here
	char tempStructure[sizeof(grcInstanceData)];
	grcInstanceData *temp = (grcInstanceData*)tempStructure;
	sysDmaGetAndWait(temp,(uint64_t)cmd->instanceData,sizeof(grcInstanceData),spuGetTag);

	// Determine address of the sampler state set(note -- we're intentionally dereferencing Data, 
	// which is a PPU-side pointer, solely to take the address of the resulting array access)
	uint64_t eaSamplerStateSet = (uint64_t)&(temp->Entries[cmd->subcommand].SamplerStateSet);
	uint16_t oldSamplerStateSet = sysDmaGetUInt16(eaSamplerStateSet,spuGetTag);
	sysDmaPutUInt16((cmd->handle << 8) | (oldSamplerStateSet >> 8),eaSamplerStateSet,spuGetTag);
	Assert(u8(oldSamplerStateSet)==INVALID_STATEBLOCK);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcEffect__PopSamplerState)
	effectCache.Flush(cmd->effect); // Ideally just write through here
	char tempStructure[sizeof(grcInstanceData)];
	grcInstanceData *temp = (grcInstanceData*)tempStructure;
	sysDmaGetAndWait(temp,(uint64_t)cmd->instanceData,sizeof(grcInstanceData),spuGetTag);

	// Determine address of the sampler state set(note -- we're intentionally dereferencing Data, 
	// which is a PPU-side pointer, solely to take the address of the resulting array access)
	uint64_t eaSamplerStateSet = (uint64_t)&(temp->Entries[cmd->subcommand].SamplerStateSet);
	uint16_t savedSamplerStateSet = sysDmaGetUInt16(eaSamplerStateSet,spuGetTag);
	savedSamplerStateSet = (savedSamplerStateSet << 8) ASSERT_ONLY(| INVALID_STATEBLOCK);
	sysDmaPutUInt16(savedSamplerStateSet,eaSamplerStateSet,spuGetTag);
END_SPU_COMMAND

#if LAZY_STATEBLOCKS
BEGIN_SPU_SIMPLE_COMMAND(grcStateBlock__Flush)
	if (any->subcommand)
		grcStateBlock::MakeDirty();
	grcStateBlock::Flush();
END_SPU_COMMAND
#endif

BEGIN_SPU_COMMAND(grcDevice__grcBegin)
	qword fmt[4];
	uint32_t common = cmd->common;
	fmt[0] = MAKE_QWORD(common | (3 << 4) | CELL_GCM_VERTEX_F,common | CELL_GCM_VERTEX_F,common | cmd->subcommand,common | cmd->colorCount);
	fmt[1] = (qword)spu_splats(common | CELL_GCM_VERTEX_F);
	fmt[2] = MAKE_QWORD(common | cmd->texCount,common | CELL_GCM_VERTEX_F,common | CELL_GCM_VERTEX_F,common | CELL_GCM_VERTEX_F);
	fmt[3] = (qword)spu_splats(common | CELL_GCM_VERTEX_F);
	GenerateVertexFormatCmdBuf(GCM_CONTEXT, pSpuGcmState, (const u32*)fmt, 0);
END_SPU_COMMAND

BEGIN_SPU_COMMAND(grcDevice__BindVertexFormat)
	spuVertexDeclaration *vertexDeclaration = s_vtxDeclCache.Get((spuVertexDeclaration*)cmd->vertexdecl);
	qword fmt0 = si_lqd(si_from_ptr(vertexDeclaration->FormatV), 0);
	qword fmt1 = si_lqd(si_from_ptr(vertexDeclaration->FormatV), 16);
	qword fmt2 = si_lqd(si_from_ptr(vertexDeclaration->FormatV), 32);
	qword fmt3 = si_lqd(si_from_ptr(vertexDeclaration->FormatV), 48);

	CompileTimeAssert(__alignof(*pSpuGcmState) >= 16);
	CompileTimeAssert((OffsetOf(__typeof__(*pSpuGcmState), CachedStates.VertexFormats) & 15) == 0);
	qword cache = si_from_ptr(pSpuGcmState->CachedStates.VertexFormats);
	si_stqd(fmt0, cache, 0);
	si_stqd(fmt1, cache, 16);
	si_stqd(fmt2, cache, 32);
	si_stqd(fmt3, cache, 48);

	qword *cmdBuf = ReserveMethodSizeAligned(GCM_CONTEXT,5);
	cmdBuf[0] = MAKE_QWORD(0,0x00040000 | CELL_GCM_NV4097_SET_FREQUENCY_DIVIDER_OPERATION,vertexDeclaration->StreamFrequencyMode,0x00400000 | CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_FORMAT);
	cmdBuf[1] = fmt0;
	cmdBuf[2] = fmt1;
	cmdBuf[3] = fmt2;
	cmdBuf[4] = fmt3;
END_SPU_COMMAND

// EOF
