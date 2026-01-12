#if __SPU

#define		TASK_SPU_2	1
#define ENTRYPOINT UpdateCloth
#define		__SPU_SCRATCHPAD	1
#if 0//!__FINAL
#define SYS_DMA_EVERYTHING
#endif // !__FINAL

#include "system/taskheaderspu.h"
#include "system/task_spu.h"
#include "system/spuscratch.h"
#include "vector/geometry.cpp"
#include "physics/inst.cpp"
#include "phbound/bound.cpp"
#include "phbound/boundcapsule.cpp"
#include "phbound/boundbvh.cpp"
#include "clothdata.cpp"
#include "cloth/clothcontroller.h"
#include "cloth_verlet_col.cpp"
#include "cloth_verlet_col_cloth.cpp"
#include "cloth_verlet_update.cpp"
#include "grcore/fvf.cpp"
#include "grcore/vertexbuffereditor.cpp"
#include "grmodel/geometry.h"

using namespace rage;

extern "C" void _exit() { __debugbreak(); }

bool sysLsCheckPointer(const void* start, const void* end, bool forWrite);
template<class T> __forceinline void sysValidateWrite(T* addr, u32 count = 1)
{
	if (!sysLsCheckPointer(addr, addr + count, true)) 
		__debugbreak();
}
template<> __forceinline void sysValidateWrite(void* addr, u32 count)
{
	sysValidateWrite((u8*)addr, count);
}
template<class T> __forceinline void sysValidateRead(const T* addr, u32 count = 1)
{
	if (!sysLsCheckPointer(addr, addr + count, false)) 
		__debugbreak();
}
template<> __forceinline void sysValidateRead(const void* addr, u32 count)
{
	sysValidateRead((u8*)addr, count);
}

#if 0 // __ASSERT
// force pointers to be reset when scratch objects go out of scope
#define sysScratchGet sysScratchGetAndSave
#endif

const Matrix34 rage::M34_IDENTITY;

namespace rage 
{

#define TRAP_VIRTUAL_CALLS (!__FINAL)
#define dprintf(...) //Printf(__VA_ARGS__)

#if TRAP_VIRTUAL_CALLS
	void VirtualCall(void* pClass)
	{
		Quitf("Virtual function called on class instance %p", pClass);
	}
	void (*g_pVTable[32])(void*);
#endif

	int grmGeometry::s_BuildBuffer = 0;

	template <typename _T>
	struct atSpuArray : public atArray<_T>
	{
		atSpuArray(int count, _T* elements)
		{
			atArray<_T>::m_Count = count;
			atArray<_T>::m_Capacity = count;
			atArray<_T>::m_Elements = elements;
		}
	};

	class grcVertexBufferSpu : public grcVertexBufferGCM
	{
	public:
		inline u8* Get()
		{
			Assert( m_Stride == 32 );
			u8* ppu = (u8*)m_LockPtr;
			sysScratchGet(m_Fvf);
			sysScratchGet(m_LockPtr, m_VertCount * m_Stride);
			Assert(m_LockPtr);
			sysDmaWait(1);

			return ppu;
		}

		inline void Put(u8* ppu)
		{
			sysDmaLargePut(m_LockPtr, (u32)ppu, m_VertCount * m_Stride, 0);
		}
	};

	grcVertexBuffer* grmGeometryQB::GetVertexBuffer(bool bDrawBuffer)
	{
		int nIndex = 0;
		if( m_DoubleBuffered & TRIPLE_VERTEX_BUFFER )
		{
			Assert(!bDrawBuffer);
			nIndex = s_BuildBuffer;
		}
		return m_VB[nIndex];
	}

	grcIndexBuffer* grmGeometryQB::GetIndexBuffer(bool bDrawBuffer)
	{
		int nIndex = 0;
		if( m_DoubleBuffered & TRIPLE_INDEX_BUFFER )
		{
			Assert(!bDrawBuffer);
			nIndex = s_BuildBuffer;
		}
		return m_IB[nIndex];
	}

	class grmGeometryQBSpu : public grmGeometryQB
	{
	public:
		void Init()
		{
#if TRAP_VIRTUAL_CALLS
			*(const void**)this = g_pVTable;
#endif

			int nIndex = 0;
			if( m_DoubleBuffered & TRIPLE_INDEX_BUFFER )
				nIndex = s_BuildBuffer;

			sysScratchGet(m_MtxPalette, m_MtxCount);
			sysScratchGet(m_VB[s_BuildBuffer]);
			sysScratchGet(m_IB[nIndex]);		
			sysDmaWait(1);
		}
		static void SetBuildBuffer(int i) {s_BuildBuffer = i;}
	};


	class clothControllerSpu : public clothController
	{
	public:
		void Init(phVerletCloth* pClothSim, phVerletCloth* pClothDrw, int simLodIndex, int drwLodIndex) 
		{
			m_Cloth[simLodIndex] = pClothSim;
			if( pClothDrw )
				m_Cloth[drwLodIndex] = pClothDrw;			// used in morphing

			sysScratchGet(m_Bridge);
			sysScratchGet(m_MorphController);
			sysDmaWait(1<<0);

			clothBridgeSimGfx* clothBridge = GetBridge();
			Assert( clothBridge );

			phMorphController* morphController = GetMorphController();
			Assert( morphController );

			int meshVerts = clothBridge->GetMeshVertCount(simLodIndex);
			sysScratchGet( clothBridge->GetClothDisplayMapRef(simLodIndex), meshVerts );

			if( drwLodIndex != simLodIndex )
			{
				meshVerts = clothBridge->GetMeshVertCount(drwLodIndex);
				sysScratchGet( clothBridge->GetClothDisplayMapRef(drwLodIndex), meshVerts );

				Assert( morphController );
				sysScratchGet( morphController->m_MapData[drwLodIndex] );
			}

			sysDmaWait(1<<0);
 
 			if( drwLodIndex != simLodIndex )
 			{
				const int vtxCount = morphController->GetMorphMapCount( drwLodIndex, simLodIndex );
				const int indexCount = morphController->GetIndexMapCount( drwLodIndex, simLodIndex );

				sysScratchGet( morphController->m_MapData[drwLodIndex]->m_MMap[simLodIndex].weights.m_Data.GetElements(), vtxCount );
				sysScratchGet( morphController->m_MapData[drwLodIndex]->m_MMap[simLodIndex].vtxIndex.m_Data.GetElements(), vtxCount );
				sysScratchGet( morphController->m_MapData[drwLodIndex]->m_MMap[simLodIndex].index0.m_Data.GetElements(), vtxCount );
				sysScratchGet( morphController->m_MapData[drwLodIndex]->m_MMap[simLodIndex].index1.m_Data.GetElements(), vtxCount );
				sysScratchGet( morphController->m_MapData[drwLodIndex]->m_MMap[simLodIndex].index2.m_Data.GetElements(), vtxCount );

				sysScratchGet( morphController->m_MapData[drwLodIndex]->m_IMap[simLodIndex].m_Data.GetElements(), indexCount );

 				sysDmaWait(1<<0);
 			}

		}
	};


} // namespace rage

u8*     g_instLvlIdxToMtxAddrMM = NULL;
Mat34V* g_instLastMtxAddrMM = NULL;

void UpdateCloth(sysTaskContext& c)
{
	SPU_BOOKMARK(0x3110);

#if TRAP_VIRTUAL_CALLS
	for(u32 i=0; i<sizeof(g_pVTable)/sizeof(g_pVTable[0]); ++i)
		g_pVTable[i] = &VirtualCall;
#endif

	const_cast<Matrix34&>(M34_IDENTITY).Identity();

	phVerletClothUpdate& u = *(c.GetUserDataAs<phVerletClothUpdate>());

	sysScratchInit(c.GetScratch(c.ScratchSize()), c.ScratchSize());

#if ENABLE_SPU_COMPARE
	gpCompareBuf		= u.compareBuf;
#endif

	gSPU_PauseClothSimulation = u.pauseSimulation;

#if ENABLE_SPU_DEBUG_DRAW

	const int verletDebugSpuSize = (sizeof(phVerletSPUDebug)+15)&(~15);
	u8* verletSpuNodeBuffer = sysScratchAllocObj<u8>( verletDebugSpuSize );
	phVerletSPUDebug* verletSpuDebug = NULL;
	if( u.verletSpuDebugAddr != 0 )
	{
		sysDmaLargeGet(verletSpuNodeBuffer, u.verletSpuDebugAddr, verletDebugSpuSize, 0);
		sysDmaWait(1<<0);
		verletSpuDebug = (phVerletSPUDebug*)verletSpuNodeBuffer;
		Assert( verletSpuDebug );
		gpDebugDrawBuf	= verletSpuDebug->debugDrawBuf;
	}
	else
	{
		gpDebugDrawBuf	= NULL;
	}

	gSPUDebugDrawSpheresCount	= 0;
	gSPUDebugDrawTrianglesCount	= 0;
	gSPUDebugDrawBoxesCount		= 0;
	gSPUDebugDrawCapsulesCount	= 0;

#endif

	g_instLvlIdxToMtxAddrMM = u.instLastMtxIdxAddrMM;
	g_instLastMtxAddrMM = u.instLastMatricsAddrMM;

	phVerletCloth* pCloth = (phVerletCloth*)c.GetInput((sizeof(phVerletCloth)+15)&~15);
	Assert( pCloth );

	phVerletCloth* pClothDrw = 0;

	const bool bMorph = (u.simLodIndex != u.drwLodIndex) ? true: false;
	if( bMorph )
	{
		pClothDrw = (phVerletCloth*)c.GetInput((sizeof(phVerletCloth)+15)&~15);
		Assert( pClothDrw );
	}

	SPU_BOOKMARK(0x3120);

	sysScratchGetAndSave(pCloth->m_VerletClothType, 1, 1);
	sysScratchGetAndSave(pCloth->m_CollisionInst);

	sysScratchInfo* spuVertexPositions = sysScratchGetAndSave(pCloth->m_ClothData.m_VertexPositions);
	Assert( spuVertexPositions );
	sysScratchGetAndSave(pCloth->m_ClothData.m_VertexPrevPositions);		//  used by vehicle damage

	sysScratchGetAndSave(pCloth->m_ClothData.m_VertexPinnedPositions);

	sysScratchInfo* spuVertexPositionsD = 0;

	if( bMorph )
	{
		spuVertexPositionsD = sysScratchGetAndSave(pClothDrw->m_ClothData.m_VertexPositions);
		Assert( spuVertexPositionsD );
	}

	sysDmaWait(1<<1);

	sysDmaWait(1);

	ScalarV timeStep;
	timeStep.Set(u.m_Gravity.w);
	grmGeometryQBSpu::SetBuildBuffer(u.vertexBufferIdx);

	clothControllerSpu* pController = (clothControllerSpu*)c.GetInput((sizeof(clothController)+15)&~15);
	Assert( pController );
	pController->Init(pCloth, pClothDrw, u.simLodIndex, u.drwLodIndex);

	grmGeometryQBSpu* pClothGeometry = 0;
	if( !u.bClothSimMeshOnly )
	{
		pClothGeometry = c.GetInputAs<grmGeometryQBSpu>();		
		pClothGeometry->Init();
	}
	else
	{
		Assert( !pClothGeometry );
	}

	grcVertexBufferSpu* pVertexBuffer = (pClothGeometry ? (grcVertexBufferSpu*)pClothGeometry->GetVertexBuffer() : 0);
	u8* ppu = 0;
	Mat34V frame = u.m_Frame;

	if( pVertexBuffer && pVertexBuffer->grcVertexBufferSpu::IsValid() )
	{
		ppu = pVertexBuffer->Get();		

		clothBridgeSimGfx* clothBridge = pController->GetBridge();
		Assert( clothBridge );
		sysScratchGet( clothBridge->GetVertexWeightsRef(u.simLodIndex), clothBridge->GetMeshVertCount(u.simLodIndex) );
		sysScratchGet( clothBridge->GetInflationScaleRef(u.simLodIndex), clothBridge->GetMeshVertCount(u.simLodIndex) );

		int nVert = pController->GetCloth( u.simLodIndex )->GetNumVertices();
		Vec3V* normals = sysScratchAllocObj<Vec3V>( nVert );
		Assert( normals );

		Assert( sizeof(u16) == sizeof(POLYGON_INDEX) );
		Assert( (u.simLodIndex == u.drwLodIndex ) || (pController->GetMorphController() && (u.drwLodIndex < u.simLodIndex) && (nVert == pController->GetMorphController()->GetIndexMapCount(u.drwLodIndex, u.simLodIndex))) );

		POLYGON_INDEX* indexMap = (POLYGON_INDEX*)((u.simLodIndex == u.drwLodIndex ) ? pController->GetClothDisplayMap( u.drwLodIndex ): pController->GetMorphController()->GetMap( u.drwLodIndex, u.simLodIndex ));
		Assert( indexMap );

		grcVertexBufferEditor editor(pVertexBuffer,true, true);
		pController->GetVertexNormals( (Vector3*)normals, 1/*normalOffset*/, pVertexBuffer->GetVertexStride(), (u8*)editor.GetVertexBuffer()->GetFastLockPtr(), indexMap, nVert, pVertexBuffer->GetFvf()->GetTangentChannel(0) );

		int stride = 1;

		sysDmaWait(1<<0);

		const float* vertWeights = clothBridge->GetVertexWeights(u.simLodIndex);
		Assert( vertWeights );
		const float* inflationScale = clothBridge->GetInflationScale(u.simLodIndex);
		Assert( inflationScale );

		if( u.userDataAddress )
		{
			Mat34V tMat[3] = { Mat34V(V_IDENTITY), Mat34V(V_IDENTITY), Mat34V(V_IDENTITY), };

			Vec3V atVec = Negate( frame.GetCol1() );
			Vec3V vForce = Normalize( VECTOR3_TO_VEC3V( u.m_Force ) );
			Vec3V vUp	 = Normalize( Cross( vForce, atVec ) );
			Vec3V vRight = Normalize( Cross( vForce, vUp) );

			tMat[1].SetCol2( vUp );
			tMat[1].SetCol1( vForce );
			tMat[1].SetCol0( vRight );

			tMat[2].SetCol2( Negate(vUp) );
			tMat[2].SetCol1( vForce );
			tMat[2].SetCol0( vRight );


			int* matIndices = sysScratchAllocObj<int>( nVert );
			Assert( matIndices );
			sysDmaLargeGet((void*)matIndices, u.userDataAddress, sizeof(int)* nVert, 0);
			sysDmaWait(1<<0);

			pCloth->ApplyAirResistanceTransform( matIndices, tMat, inflationScale, vertWeights, (Vec3V*)normals, stride, u.m_Force );
		}
		else
		{
			pCloth->ApplyAirResistance( inflationScale, vertWeights, (Vec3V*)normals, stride, u.m_Force );
		}

		Assert( normals );
		sysScratchReset( (u8*)normals );
	}

	Vector3 _zero(Vector3::ZeroType);
	Vector3 translationV = VEC3V_TO_VECTOR3(frame.GetCol3());
	if( !u.separateMotion )
	{
		translationV = _zero;
	}
	else
	{
		frame.SetCol3( VECTOR3_TO_VEC3V(_zero) );
	}

// TODO: check if we need this wait here - svetli
	sysDmaWait(1<<0);

	pController->ApplyPinning( frame, u.simLodIndex );

	phVerletCloth* pVerletClothType = const_cast<phVerletCloth*>( (pCloth->m_VerletClothType ? pCloth->m_VerletClothType.ptr: pCloth) );
	Assert( pVerletClothType );

	sysScratchInfo* pEdgeData =	sysScratchGetAndSave(pVerletClothType->m_EdgeData);
	Assert( pEdgeData );

	sysScratchGetAndSave(pVerletClothType->m_CustomEdgeData);

	sysDmaWait(1<<0);

	SPU_BOOKMARK(0x3130);

	pCloth->UpdateEnvironmentCloth( u.m_Force.w, timeStep, pCloth->m_CollisionInst, VECTOR3_TO_VEC3V(u.m_Gravity), ScalarV(V_ONE) );
	
	SPU_BOOKMARK(0x3140);

	if( bMorph )
	{
		pController->Morph( u.drwLodIndex, u.simLodIndex );
	}
 	
	sysScratchReset( (u8*)pEdgeData->ls );		// move up/back the ptr/watermark in the scratchpad

	pCloth->ComputeClothBoundingVolume();
	if ( ppu )
	{		
		Assert( pClothGeometry );
		pController->ApplySimulationToMesh( (grmGeometryQB&)*pClothGeometry, translationV, u.drwLodIndex, Vec3V(V_ZERO));
#if ENABLE_SPU_COMPARE
		if( !gpCompareBuf )
#endif
		{
			Assert( pVertexBuffer );
			pVertexBuffer->Put(ppu);		
		}
		sysDmaWait(1<<0);
	}


#if ENABLE_SPU_COMPARE
	if( !gpCompareBuf )
#endif
	{
// Note: DMA back m_BoundingCenterAndRadius, m_BBMin, m_BBMax
#if !NO_BOUND_CENTER_RADIUS
		sysDmaLargePut((void*)&pCloth->m_BoundingCenterAndRadius, u.boundingCenterAndRadiusAddress, 48, 0);
#else
		sysDmaLargePut((void*)&pCloth->m_BBMin, u.boundingCenterAndRadiusAddress, 32, 0);
#endif

		spuVertexPositions->Writeback();
		if( spuVertexPositionsD )
			spuVertexPositionsD->Writeback();

#if ENABLE_SPU_DEBUG_DRAW
		if( gpDebugDrawBuf )
		{
			Vec4V countersOut;
			countersOut.SetXi( gSPUDebugDrawSpheresCount );
			countersOut.SetYi( gSPUDebugDrawBoxesCount );
			countersOut.SetZi( gSPUDebugDrawTrianglesCount );
			countersOut.SetWi( gSPUDebugDrawCapsulesCount );

			Assert( verletSpuDebug );
			sysDmaLargePut((void*)&countersOut,	verletSpuDebug->debugDrawCountOut, 16, 0);
			sysDmaWait(1<<0);
		}
#endif
		sysDmaWait(1<<0);
	}

	SPU_BOOKMARK(0x0);

	SPU_ONLY(checkSpursMarkers("cloth_writeback"));
}

namespace rage 
{

	void phMorphController::MorphLevelONE( int hiLOD, int lowLOD, Vec3V* RESTRICT hiLODStream, Vec3V* RESTRICT lowLODStream )
	{
		const int hiLODvertsCount = GetMorphMapCount(hiLOD, lowLOD);
		const phMorphData& morphData = m_MapData[hiLOD]->m_MMap[lowLOD];
		int i = 0;
		const Vec4V* RESTRICT weightsStream = morphData.weights.m_Data.GetElements();
		const u16* RESTRICT vtxIndexStream  = morphData.vtxIndex.m_Data.GetElements();
		const u16* RESTRICT idx0Stream = morphData.index0.m_Data.GetElements();
		const u16* RESTRICT idx1Stream = morphData.index1.m_Data.GetElements();
		const u16* RESTRICT idx2Stream = morphData.index2.m_Data.GetElements();

	#if 1

		const int count4 = (hiLODvertsCount >> 2) << 2;
		//	const int count8 = (hiLODvertsCount >> 3) << 3;
		//	for( ; i < count8; i += 8)
		for( ; i < count4; i += 4)
		{
			// 		PrefetchDC( &weightsStream[i+8] );
			// 		PrefetchDC( &vtxIndexStream[i+8] );
			// 		PrefetchDC( &idx0Stream[i+8] );
			// 		PrefetchDC( &idx1Stream[i+8] );
			// 		PrefetchDC( &idx2Stream[i+8] );

			const int	i0	= i,
				i1	= i + 1,
				i2	= i + 2,
				i3	= i + 3;
			// 					i4	= i + 4,
			// 					i5	= i + 5,
			// 					i6	= i + 6,
			// 					i7	= i + 7;

			Vec4V weights0 = weightsStream[i0];
			Vec4V weights1 = weightsStream[i1];
			Vec4V weights2 = weightsStream[i2];
			Vec4V weights3 = weightsStream[i3];
			// 		Vec4V weights4 = weightsStream[i4];
			// 		Vec4V weights5 = weightsStream[i5];
			// 		Vec4V weights6 = weightsStream[i6];
			// 		Vec4V weights7 = weightsStream[i7];

			SoA_Vec4V soa_weights_0123;
			ToSoA( soa_weights_0123, weights0, weights1, weights2, weights3 );
			//		SoA_Vec4V soa_weights_4567
			//		ToSoA( soa_weights_4567, weights4, weights5, weights6, weights7 );

			SoA_ScalarV soa_w0_0123 = soa_weights_0123.GetX();
			SoA_ScalarV soa_w1_0123 = soa_weights_0123.GetY();
			SoA_ScalarV soa_w2_0123 = soa_weights_0123.GetZ();
			SoA_ScalarV soa_d_0123  = soa_weights_0123.GetW();
			// 		SoA_ScalarV soa_w0_4567 = soa_weights_4567.GetX();
			// 		SoA_ScalarV soa_w1_4567 = soa_weights_4567.GetY();
			// 		SoA_ScalarV soa_w2_4567 = soa_weights_4567.GetZ();
			// 		SoA_ScalarV soa_d_4567  = soa_weights_4567.GetW();

			const u16 idx00 = idx0Stream[i0];
			const u16 idx01 = idx0Stream[i1];
			const u16 idx02 = idx0Stream[i2];
			const u16 idx03 = idx0Stream[i3];
			// 		const u16 idx04 = idx0Stream[i4];
			// 		const u16 idx05 = idx0Stream[i5];
			// 		const u16 idx06 = idx0Stream[i6];
			// 		const u16 idx07 = idx0Stream[i7];

			const u16 idx10 = idx1Stream[i0];
			const u16 idx11 = idx1Stream[i1];
			const u16 idx12 = idx1Stream[i2];
			const u16 idx13 = idx1Stream[i3];
			// 		const u16 idx14 = idx1Stream[i4];
			// 		const u16 idx15 = idx1Stream[i5];
			// 		const u16 idx16 = idx1Stream[i6];
			// 		const u16 idx17 = idx1Stream[i7];

			const u16 idx20 = idx2Stream[i0];
			const u16 idx21 = idx2Stream[i1];
			const u16 idx22 = idx2Stream[i2];
			const u16 idx23 = idx2Stream[i3];
			// 		const u16 idx24 = idx2Stream[i4];
			// 		const u16 idx25 = idx2Stream[i5];
			// 		const u16 idx26 = idx2Stream[i6];
			// 		const u16 idx27 = idx2Stream[i7];

			Vec3V vtxLow00 = lowLODStream[ idx00 ];
			Vec3V vtxLow01 = lowLODStream[ idx01 ];
			Vec3V vtxLow02 = lowLODStream[ idx02 ];
			Vec3V vtxLow03 = lowLODStream[ idx03 ];
			// 		Vec3V vtxLow04 = lowLODStream[ idx04 ];
			// 		Vec3V vtxLow05 = lowLODStream[ idx05 ];
			// 		Vec3V vtxLow06 = lowLODStream[ idx06 ];
			// 		Vec3V vtxLow07 = lowLODStream[ idx07 ];

			Vec3V vtxLow10 = lowLODStream[ idx10 ];
			Vec3V vtxLow11 = lowLODStream[ idx11 ];
			Vec3V vtxLow12 = lowLODStream[ idx12 ];
			Vec3V vtxLow13 = lowLODStream[ idx13 ];
			// 		Vec3V vtxLow14 = lowLODStream[ idx14 ];
			// 		Vec3V vtxLow15 = lowLODStream[ idx15 ];
			// 		Vec3V vtxLow16 = lowLODStream[ idx16 ];
			// 		Vec3V vtxLow17 = lowLODStream[ idx17 ];

			Vec3V vtxLow20 = lowLODStream[ idx20 ];
			Vec3V vtxLow21 = lowLODStream[ idx21 ];
			Vec3V vtxLow22 = lowLODStream[ idx22 ];
			Vec3V vtxLow23 = lowLODStream[ idx23 ];
			// 		Vec3V vtxLow24 = lowLODStream[ idx24 ];
			// 		Vec3V vtxLow25 = lowLODStream[ idx25 ];
			// 		Vec3V vtxLow26 = lowLODStream[ idx26 ];
			// 		Vec3V vtxLow27 = lowLODStream[ idx27 ];

			SoA_Vec3V soa_vtxLow0_0123, soa_vtxLow1_0123, soa_vtxLow2_0123;
			ToSoA( soa_vtxLow0_0123, vtxLow00, vtxLow01, vtxLow02, vtxLow03 );
			ToSoA( soa_vtxLow1_0123, vtxLow10, vtxLow11, vtxLow12, vtxLow13 );
			ToSoA( soa_vtxLow2_0123, vtxLow20, vtxLow21, vtxLow22, vtxLow23 );

			// 		SoA_Vec3V soa_vtxLow0_4567, soa_vtxLow1_4567, soa_vtxLow2_4567;
			// 		ToSoA( soa_vtxLow0_4567, vtxLow04, vtxLow05, vtxLow06, vtxLow07 );
			// 		ToSoA( soa_vtxLow1_4567, vtxLow14, vtxLow15, vtxLow16, vtxLow17 );
			// 		ToSoA( soa_vtxLow2_4567, vtxLow24, vtxLow25, vtxLow26, vtxLow27 );

			SoA_Vec3V soa_v1_0123, soa_v2_0123;
			Subtract( soa_v1_0123, soa_vtxLow0_0123, soa_vtxLow1_0123 );
			Subtract( soa_v2_0123, soa_vtxLow2_0123, soa_vtxLow1_0123 );

			// 		SoA_Vec3V soa_v1_4567, soa_v2_4567;
			// 		Subtract( soa_v1_4567, soa_vtxLow0_4567, soa_vtxLow1_4567 );
			// 		Subtract( soa_v2_4567, soa_vtxLow2_4567, soa_vtxLow1_4567 );

			SoA_Vec3V soa_crossTemp_0123;
			Cross(soa_crossTemp_0123, soa_v1_0123, soa_v2_0123);
			//		SoA_Vec3V soa_crossTemp_4567;
			//		Cross(soa_crossTemp_4567, soa_v1_4567, soa_v2_4567);

			SoA_Vec3V soa_n0123;
			NormalizeFast( soa_n0123, soa_crossTemp_0123 );
			//		SoA_Vec3V soa_n4567;
			//		NormalizeFast( soa_n4567, soa_crossTemp_4567 );

			SoA_Vec3V soa_w2_0123_vtx0123, soa_w1_0123_vtx0123, soa_w0_0123_vtx0123, soa_d_0123_n0123;
			Scale( soa_w2_0123_vtx0123, soa_vtxLow0_0123, soa_w2_0123 );
			Scale( soa_w1_0123_vtx0123, soa_vtxLow1_0123, soa_w1_0123 );
			Scale( soa_w0_0123_vtx0123, soa_vtxLow2_0123, soa_w0_0123 );
			Scale( soa_d_0123_n0123, soa_n0123, soa_d_0123 );

			// 		SoA_Vec3V soa_w2_4567_vtx4567, soa_w1_4567_vtx4567, soa_w0_4567_vtx4567, soa_d_4567_n4567;
			// 		Scale( soa_w2_4567_vtx4567, soa_vtxLow0_4567, soa_w2_4567 );
			// 		Scale( soa_w1_4567_vtx4567, soa_vtxLow1_4567, soa_w1_4567 );
			// 		Scale( soa_w0_4567_vtx4567, soa_vtxLow2_4567, soa_w0_4567 );
			// 		Scale( soa_d_4567_n4567, soa_n4567, soa_d_4567 );

			SoA_Vec3V soa_temp0_0123, soa_temp1_0123;
			Add( soa_temp0_0123, soa_w2_0123_vtx0123, soa_w1_0123_vtx0123 );
			Add( soa_temp1_0123, soa_w0_0123_vtx0123, soa_d_0123_n0123 );
			// 		SoA_Vec3V soa_temp0_4567, soa_temp1_4567;
			// 		Add( soa_temp0_4567, soa_w2_4567_vtx4567, soa_w1_4567_vtx4567 );
			// 		Add( soa_temp1_4567, soa_w0_4567_vtx4567, soa_d_4567_n4567 );

			SoA_Vec3V soa_hiLODStream4_0123;
			Add( soa_hiLODStream4_0123, soa_temp0_0123, soa_temp1_0123 );
			//		SoA_Vec3V soa_hiLODStream4_4567;
			//		Add( soa_hiLODStream4_4567, soa_temp0_4567, soa_temp1_4567 );

			Vec3V v0, v1, v2, v3;
			ToAoS( v0, v1, v2, v3, soa_hiLODStream4_0123 );
			//		Vec3V v4, v5, v6, v7;
			//		ToAoS( v4, v5, v6, v7, soa_hiLODStream4_4567 );

			hiLODStream[vtxIndexStream[i0]] = v0;
			hiLODStream[vtxIndexStream[i1]] = v1;
			hiLODStream[vtxIndexStream[i2]] = v2;
			hiLODStream[vtxIndexStream[i3]] = v3;
			// 		hiLODStream[vtxIndexStream[i4]] = v4;
			// 		hiLODStream[vtxIndexStream[i5]] = v5;
			// 		hiLODStream[vtxIndexStream[i6]] = v6;
			// 		hiLODStream[vtxIndexStream[i7]] = v7;
		}

	#endif

		for( ; i < hiLODvertsCount; ++i )
		{
 			Vec4V weights	= weightsStream[i];
			u16 vtxIndex	= vtxIndexStream[i];
			u16 idx0		= idx0Stream[i];
			u16 idx1		= idx1Stream[i];
			u16 idx2		= idx2Stream[i];

			ScalarV w0( Vec::V4SplatX( weights.GetIntrin128() ) );
			ScalarV w1( Vec::V4SplatY( weights.GetIntrin128() ) );
			ScalarV w2( Vec::V4SplatZ( weights.GetIntrin128() ) );
			ScalarV  d( Vec::V4SplatW( weights.GetIntrin128() ) );

			Vec3V vtxLow0 = lowLODStream[ idx0 ];
			Vec3V vtxLow1 = lowLODStream[ idx1 ];
			Vec3V vtxLow2 = lowLODStream[ idx2 ];

			Vec3V v1 = Subtract( vtxLow0, vtxLow1 );
			Vec3V v2 = Subtract( vtxLow2, vtxLow1 );
			Vec3V n = NormalizeFast( Cross( v1, v2 ) );

 		#if __ASSERT	
	// 		Assert( vtxIndex != 9999 && vtxIndex != ((u16)-1) );
	//		Assert( vtxIndex < MMapCount );
 		#endif

			hiLODStream[vtxIndex] =	Scale( w2 , vtxLow0 ) + 
									Scale( w1 , vtxLow1 ) + 
									Scale( w0 , vtxLow2 ) + 
									Scale( d ,  n );
		} // end for
	}

	void phMorphController::Morph( int mapperIndex, int oldLodIndex, Vec3V* RESTRICT streamControlled, Vec3V* RESTRICT streamController, phMapData::enWEIGHT_LEVEL wLevel, phMapData::enWEIGHT_LEVEL wLevel2 )
	{
		if( mapperIndex == oldLodIndex )
			return;							// direct mapping is used

		Assert( streamControlled );
		Assert( streamController );
		Assert( m_MapData[oldLodIndex] );
		Assert( m_MapData[mapperIndex] );
		Assert( wLevel == phMapData::enWEIGHT_ONE );
//		if( wLevel == phMapData::enWEIGHT_ONE )
		{
			MorphLevelONE( mapperIndex, oldLodIndex, streamControlled, streamController );
		}	
// TODO: don't use this on the SPU for now - svetli
/*
		else if( phMapData::enWEIGHT_TWO )
		{
			Assert(0); 
//			MorphLevelTWO( mapperIndex, oldLodIndex, streamControlled, streamController );
		}
		else
		{
			Assert(0); 		// invalid morph level ?!
		}
*/
	}

	void clothController::Morph( const int controlledMeshIdx, const int controllerMeshIdx )
	{
		PF_FUNC( ClothMorph );	

// TODO: place weight level somewhere !
		phMapData::enWEIGHT_LEVEL wLevel = phMapData::enWEIGHT_ONE;

		phMorphController* morphController = GetMorphController();
		Assert( morphController );
		morphController->Morph( controlledMeshIdx, controllerMeshIdx
			, (Vec3V*)GetCloth(controlledMeshIdx)->GetClothData().GetVertexPointer()
			, (Vec3V*)GetCloth(controllerMeshIdx)->GetClothData().GetVertexPointer()
			, wLevel
			, phMapData::enWEIGHT_ONE
			);
	}

	void clothController::ApplyPinning (Mat34V_In attachedToFrame, int lodIndex)
	{
		Assert( lodIndex > -1 && lodIndex < 4 );
		phVerletCloth* cloth = m_Cloth[lodIndex];
		Assert(cloth);

		phClothData& clothData = cloth->GetClothData();
#if NO_PIN_VERTS_IN_VERLET
		int numPinnedVerts = clothData.GetNumPinVerts();
#else
		int numPinnedVerts = cloth->GetPinCount();
#endif
		if( numPinnedVerts )
		{
			const Vec3V* pinVerts = clothData.GetPinVertexPointer();
			Assert( pinVerts );

			const int prevPosCount = clothData.GetPrevPositionVertexCount();
			if( !prevPosCount)
			{
				for (int i=0; i<numPinnedVerts; ++i)
				{		
					clothData.SetVertexPosition( i, Transform(attachedToFrame, pinVerts[i]) );
				}
			}
			else
			{
				Vec3V* damageOffsets = clothData.GetVertexPrevPointer();
				Assert(damageOffsets);
				for (int i=0; i < numPinnedVerts; ++i )
				{
					clothData.SetVertexPosition( i, Transform(attachedToFrame, Add( pinVerts[i], damageOffsets[i]) ) );
				}
			}
		}
	}

} // namespace rage

#endif // __SPU
