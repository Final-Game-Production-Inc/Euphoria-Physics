#ifndef CLOTH_VERLET_SPU_H
#define CLOTH_VERLET_SPU_H

#define ENABLE_SPU_COMPARE		0
#define ENABLE_SPU_DEBUG_DRAW	(0 && __PS3 && !__FINAL)


#if ENABLE_SPU_COMPARE

namespace rage {
	void SpuCompare(const void* pData, int size, const char* msg, bool vector3 = false);
	template<class T>
	inline void SPU_COMPARE(const char* msg, const T* data, int count = 1)
	{
		SpuCompare(data, count * sizeof(T), msg);
	}
	template<>
	inline void SPU_COMPARE(const char* msg, const Vector3* data, int count)
	{
		SpuCompare(data, count * sizeof(Vector3), msg, true);
	}
	template<>
	inline void SPU_COMPARE(const char* msg, const Vec3V* data, int count)
	{
		SpuCompare(data, count * sizeof(Vec3V), msg, true);
	}
	template<>
	inline void SPU_COMPARE(const char* msg, const Matrix34* data, int count)
	{
		SpuCompare(data, count * sizeof(Matrix34), msg, true);
	}
	template<>
	inline void SPU_COMPARE(const char* msg, const Mat34V* data, int count)
	{
		SpuCompare(data, count * sizeof(Matrix34), msg, true);
	}
#if __PPU
	extern bool gEnableSpuCompare;
	extern u8	gCompareBuf[1024*1024] ;
	extern u8*	gpCompareBuf;
	extern int	gSpuCompareFailed ;
#endif
} // namespace rage

#else
	#define		SPU_COMPARE(...)
#endif // ENABLE_SPU_COMPARE


#if ENABLE_SPU_DEBUG_DRAW

#define		SPU_DEBUG_DRAW_ONLY(x)	x

namespace rage
{
	void SpuDebugSphere(const void* pData);
	void SpuDebugBox(const void* pV0, const void* pV1);
	void SpuDebugTriangle(const void* pV0, const void* pV1, const void* pV2);
	void SpuDebugCapsule(const void* pCol0, const void* pCol1, const void* pCol2, const void* pCol3, const void* pLen, const void* pRad);

	enum enDebugPrimType
	{
		SPU_DEBUG_SPHERE  = 0,
		SPU_DEBUG_BOX,
		SPU_DEBUG_TRIANGLE,
		SPU_DEBUG_CAPSULE,
		SPU_DEBUG_PRIM_COUNT,
	};	

	#if __PPU
		
		#define		MAX_VERLET_SPU_DEBUG		32
		#define		VERLET_DEBUG_BUFFER			(8 * 1024)
		#define		DEBUG_DRAW_BUFFER_SIZE		(MAX_VERLET_SPU_DEBUG * VERLET_DEBUG_BUFFER)

		extern u8	*gDebugDrawBuf;
		
		struct i128type
		{
			int c[4];
		};
		extern i128type *gDebugDrawPrimCount;

// NOTE: the following is used only on the SPU
		extern u8*	gpDebugDrawBuf;
		extern int	gSPUDebugDrawSpheresCount;
		extern int	gSPUDebugDrawTrianglesCount;
		extern int	gSPUDebugDrawBoxesCount;
		extern int	gSPUDebugDrawCapsulesCount;
	#endif
} // namespace rage
#else
	#define		SPU_DEBUG_DRAW_ONLY(x)
	#define		VERLET_DEBUG_BUFFER			(0)
#endif // ENABLE_SPU_DEBUG_DRAW



namespace rage
{
	extern bool gSPU_PauseClothSimulation;
}

#endif // CLOTH_VERLET_SPU_H
