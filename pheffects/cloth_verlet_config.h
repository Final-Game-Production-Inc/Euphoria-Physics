//
// pheffects/cloth_verlet_config.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_CLOTH_VERLET_CONFIG_H
#define PHEFFECTS_CLOTH_VERLET_CONFIG_H

#include "clothdata.h"
#include "grprofile/drawcore.h"

#define QR_ROUND	31

namespace rage {

#if __PFDRAW
	void DrawClothWireframe( const Vec3V* RESTRICT vertexBuffer, const phVerletCloth& cloth, void* controllerAddress, const Vector3& offset = Vector3(Vector3::ZeroType), const Vector3& viewPt = Vector3(Vector3::ZeroType), const bool useNormals = false, const bool enableAlpha = false, const bool flipAlpha = false, const float sphereRad = 0.005f, const Vector3& vertsColor = Vector3(1.0f, 0.0f, 0.0f) );
	void DrawEdgesColor( const Vec3V* RESTRICT vertexBuffer, const phVerletCloth& cloth, const Vector3& offset = Vector3(Vector3::ZeroType) );
#endif

}

#endif
