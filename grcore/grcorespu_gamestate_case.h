#if HACK_GTA4
BEGIN_SPU_COMMAND(GTA4__SetShadowType)
	pSpuGcmState->shadowType = (u8)cmd->shadowType; // eEdgeShadowType
END_SPU_COMMAND
BEGIN_SPU_COMMAND(GTA4__SetShadowMatrix)
	pSpuGcmState->shadowMatrix= cmd->shadowMatrix;

	// force regeneration of edge's viewProjectionMatrix (used by warped shadows):
	// (stolen from grcViewport__RegenerateDevice):
	// similar code executed in grcViewport__RegenerateDevice and grcState__SetWorldFast:
	if(1)
	{
		spuMatrix43 &ew = *union_cast<spuMatrix43*>( &pSpuGcmState->EdgeWorld );
		ew.Transpose(pSpuGcmState->World);
		pSpuGcmState->LocalLRTB.Transform(pSpuGcmState->FrustumLRTB, pSpuGcmState->World);

		spuMatrix44 &destVPM = *union_cast<spuMatrix44*>( &pSpuGcmState->EdgeInfo.viewProjectionMatrix );
		if(spuGcmShadowWarpEnabled(pSpuGcmState->shadowType))
		{
			destVPM.Transpose(pSpuGcmState->shadowMatrix);
		}
		else
		{
			spuMatrix44 viewProj;
			viewProj.Transform(pSpuGcmState->Proj,pSpuGcmState->View);
			destVPM.Transpose(viewProj);
		}
		//pSpuGcmState->EdgeInfo.dirty = 1;

		// GEOMETRY_JOBS.FlushPendingJobSegment();
	}
END_SPU_COMMAND
BEGIN_SPU_COMMAND(GTA4__SetDamageTexture)
	pSpuGcmState->IsVehicleGeom = cmd->subcommand? 1:0;
	pSpuGcmState->damageTexture = cmd->damageTexture;
	pSpuGcmState->damageBoundRadius = cmd->boundRadius;
END_SPU_COMMAND
BEGIN_SPU_COMMAND(GTA4__SetDamageTextureOffset)
	pSpuGcmState->damageTextureOffset[0] = cmd->damageTexOffset.x;
	pSpuGcmState->damageTextureOffset[1] = cmd->damageTexOffset.y;
	pSpuGcmState->damageTextureOffset[2] = cmd->damageTexOffset.z;
END_SPU_COMMAND
BEGIN_SPU_COMMAND(GTA4__SetCharacterClothData)
	pSpuGcmState->clothNumVerts = cmd->subcommand;
	pSpuGcmState->clothMorphData = cmd->morphData;
END_SPU_COMMAND
BEGIN_SPU_COMMAND(GTA4__SetTintDescriptor)
	if(cmd->tintShaderIdx && pSpuGcmState->tintDescriptorPtr && pSpuGcmState->tintDescriptorCount)
	{	// edgegeomspu tint palette ptr setup:
		if(cmd->tintShaderIdx==0xff)
		{	// reset:
			pSpuGcmState->edgeTintPalettePtr = NULL;
		}
		else
		{	// full tint palette EDGE selection:
			const u32 shaderIdx = cmd->tintShaderIdx-1;

			const u32 tintDmaTag=8;
			// fetch tint descriptor:
			const u32 dmaSize = ((pSpuGcmState->tintDescriptorCount*sizeof(u32))+0xf)&(~0xf);
			FastAssert(dmaSize < 256);
			u32* tintTab = (u32*)AllocaAligned(u8, dmaSize, 16);
			sysDmaGet((void*)tintTab, (u32)pSpuGcmState->tintDescriptorPtr, dmaSize, tintDmaTag);
			sysDmaWaitTagStatusAll(1<<tintDmaTag);	// wait for tintTab
			
			FastAssert(shaderIdx < pSpuGcmState->tintDescriptorCount);
			u32 tint = tintTab[ shaderIdx ];
			pSpuGcmState->edgeTintPalettePtr= tint? ((void*)(tint&~0xf)) : NULL;
			pSpuGcmState->edgeTintFlags		= tint&0xf;
		}
	}
	else
	{	// drawablespu tint descriptor setup:
		pSpuGcmState->tintDescriptorCount	= cmd->tintDescriptorCount;
		pSpuGcmState->tintDescriptorPtr		= cmd->tintDescriptorPtr;
	}
END_SPU_COMMAND
BEGIN_SPU_COMMAND(GTA4__SetWriteRsxLabel)
	cell::Gcm::Inline::cellGcmSetWriteCommandLabel(GCM_CONTEXT, cmd->subcommand, (u32)cmd->value);
END_SPU_COMMAND
#elif HACK_MC4
BEGIN_SPU_COMMAND(MC4__SetDamageTexture)
	SPU_BOOKMARK(0x1300);
	pSpuGcmState->damageTexture = cmd->damageTexture;
END_SPU_COMMAND
BEGIN_SPU_COMMAND(MC4__SetDamageParams)
	SPU_BOOKMARK(0x1310);
	pSpuGcmState->damageParams[0] = cmd->damageParams[0];
	pSpuGcmState->damageParams[1] = cmd->damageParams[1];
	pSpuGcmState->damageParams[2] = cmd->damageParams[2];
	pSpuGcmState->damageParams[3] = cmd->damageParams[3];
END_SPU_COMMAND
BEGIN_SPU_COMMAND(MC4__SetRestMatrix)
	SPU_BOOKMARK(0x1320);
	pSpuGcmState->restMtx = cmd->mtx;
	sysDmaGet(spuScratch,(uint64_t)cmd->mtx,sizeof(spuMatrix44),spuGetTag);
	sysDmaWaitTagStatusAll(1<<spuGetTag);
	cellGcmSetVertexProgramParameterBlock(GCM_CONTEXT,188,4,(float*)spuScratch);
END_SPU_COMMAND
BEGIN_SPU_COMMAND(MC4__SetTransformType)
	SPU_BOOKMARK(0x1330);
	pSpuGcmState->transformType = cmd->value;
END_SPU_COMMAND
BEGIN_SPU_COMMAND(MC4__SetTransformParam)
	SPU_BOOKMARK(0x1340);
	pSpuGcmState->transformParam = cmd->value;
END_SPU_COMMAND
BEGIN_SPU_SIMPLE_COMMAND(MC4__SetBoneIndex)
	SPU_BOOKMARK(0x1350);
	const EdgeGeomLocalToWorldMatrix &ew = pSpuGcmState->bones[any->subcommand];
	pSpuGcmState->EdgeWorld = ew;
	pSpuGcmState->World.Transpose(reinterpret_cast<const spuMatrix43&>(ew));

	/* float *f = (float*) &pSpuGcmState->EdgeWorld;
	grcDisplayf("EdgeWorld:\n x %f %f %f | %f\n y %f %f %f | %f\n z %f %f %f | %f\n",
		f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7],f[8],f[9],f[10],f[11]);

	f = (float*) &pSpuGcmState->World;
	grcDisplayf("World:\n a %f %f %f %f\n b %f %f %f %f\n c %f %f %f %f\n d %f %f %f %f",
		f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7],f[8],f[9],f[10],f[11],f[12],f[13],f[14],f[15]); */

	cellGcmSetVertexProgramParameterBlock(GCM_CONTEXT,0,4,(float*)&pSpuGcmState->World);

	// Compute world dot view and send it down
	spuMatrix44 worldView;
	worldView.Transform(pSpuGcmState->View, pSpuGcmState->World);
	cellGcmSetVertexProgramParameterBlock(GCM_CONTEXT,4,4,(float*)&worldView);

	// Compute world dot view dot projection and send it down
	spuMatrix44 worldViewProj;
	worldViewProj.Transform(pSpuGcmState->Proj, worldView);
	cellGcmSetVertexProgramParameterBlock(GCM_CONTEXT,8,4,(float*)&worldViewProj);
END_SPU_COMMAND
BEGIN_SPU_COMMAND(MC4__SendBones)
	SPU_BOOKMARK(0x1360);
	Assert(cmd->subcommand <= 20);
	for (int i=0;i<cmd->subcommand; i++) {
		pSpuGcmState->bones[i] = cmd->mtxs[i];
		pSpuGcmState->bones[i].matrixData[3] += cmd->ox;
		pSpuGcmState->bones[i].matrixData[7] += cmd->oy;
		pSpuGcmState->bones[i].matrixData[11] += cmd->oz;
	}
END_SPU_COMMAND
#endif
