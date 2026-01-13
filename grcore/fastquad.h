//
// grcore/fastquad.h
//
// Copyright (C) 2012 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_FASTQUAD_H
#define GRCORE_FASTQUAD_H

#include "grcore/config.h"
#include "grcore/device.h"
#include "grcore/AA_shared.h"
#include "fastquad_switch.h"
#include "vector/vector4.h"

namespace rage
{
#if FAST_QUAD_SUPPORT
	class grcDevice;

	class FastQuad
	{
		friend class grcDevice;
		static void Init();
		static void Shutdown();

	public:
		static void Draw(bool bFullScreen);

		static inline Vector4 Pack(float x0, float y0, float x1, float y1)
		{
			return Vector4( x1-x0, y1-y0, x0, y0 );
		}
	};
#endif // FAST_QUAD_SUPPORT

#if DEVICE_EQAA
	class grcRenderTarget;

	namespace eqaa
	{
		enum { CENTERED = RSG_ORBIS || RSG_DURANGO };	//use centered custom sample pattern

#if AA_SAMPLE_DEBUG
		struct Debug
		{
			Debug();
			bool	useDeprecatedPattern;
			float	patternRadius;
			float	patternOffset;
			float	resolveSideWeight;
			float	resolveDepthThreshold;
			bool	resolveSkipEdges;
		};
		Debug& GetDebug();
#endif //AA_SAMPLE_DEBUG
#if RSG_ORBIS
		struct Control
		{
			Control();
			bool incoherentEqaaReads;
			bool staticAnchorAssociations;
			bool interpolateCompressedZ;
			bool highQualityTileIntersections;
		};
		Control& GetControl();
#endif //RSG_ORBIS
		
		namespace pass
		{
			enum Clear
			{
				fast_clear_cs,
				fast_copy_cs,
				quad_clear,
				quad_dummy,
			};
			enum Resolve
			{
				copy_fmask,
				resolve_msaa,
				debug_frag_msaa,
				debug_mask_msaa,
				resolve_eqaa,
				debug_frag_eqaa,
				debug_sample_eqaa,
				debug_fmask,
				compress_depth,
				resolve_eqaa_extra,
				resolve_eqaa_quality_41,
#if AA_SAMPLE_DEBUG
				resolve_eqaa_quality_41_centered_old,
#endif
				resolve_eqaa_quality_41_dithered,
				resolve_eqaa_quality_42,
				resolve_eqaa_quality_82,
				resolve_eqaa_performance_104,	//hex
				resolve_eqaa_nandetect
			};
		}

		void SetEffectName(const char *effectName);
		void Init();
		const grcVertexDeclaration *GetVertexDeclaration();

		void ClearBuffer(const char debugName[], void *address, unsigned size, const u32 value);
		void CopyBuffer(const char debugName[], void *dest, void *source, unsigned size);

		class ResolveArea
		{
			ResolveArea(const ResolveArea&);
			void operator=(const ResolveArea&);
		public:
			static void Apply();
			ResolveArea(u32 x, u32 y, u32 w, u32 h);
			~ResolveArea();
		};
		
		void DrawClear(pass::Clear pass);
		void DrawResolve(pass::Resolve pass, bool toDepth = false);
		bool ResolveMeSoftly(grcRenderTarget *const destination, const grcRenderTarget *const source, const int destSliceIndex);

		u32 UntileCmask(void *const destination, const void *const rawCmask, unsigned width, unsigned height, bool isLinear, bool packTiles);
		u32 UntileHtile(void *const destination, const void *const rawHtile, unsigned width, unsigned height, bool isLinear, bool withStencil);
		void UntileFmask(grcRenderTarget *const destination, const grcTexture *const fmask, const grcDevice::MSAAMode &mode, unsigned sampleOffset);
	}
#endif // DEVICE_EQAA

}	//rage
#endif	//GRCORE_FASTQUAD_H