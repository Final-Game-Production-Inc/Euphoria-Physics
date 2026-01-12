#include "cloth_verlet.h"
#include "cloth/clothcontroller.h"
#include "grcore/indexbuffer.h"
#include "grcore/vertexbuffer.h"
#include "grcore/vertexbuffereditor.h"
#include "grmodel/geometry.h"
#include "vectormath/classes.h"
#include "vectormath/classes_soa.h"
#include "vectormath/layoutconvert.h"
#include "profile/page.h"
#include "profile/group.h"
#include "profile/element.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "physics/simulator.h"
#include "tune.h"
#include "system/task.h"
#include "system/spuscratch.h"
#include "phbound/boundcomposite.h"
#include "cloth_verlet_spu.h"
#include "vector/colors.h"


DECLARE_TASK_INTERFACE(UpdateCharacterCloth);
DECLARE_TASK_INTERFACE(UpdateEnvironmentCloth);
DECLARE_TASK_INTERFACE(UpdateEnvironmentRope);


#if __WIN32
#pragma warning( disable:4100 )
#endif

//#pragma optimize("", off)

#define		DAMP_TIME_SCALE			40.0f


namespace rage {

EXT_PFD_DECLARE_ITEM(ClothEdges);
EXT_PFD_DECLARE_ITEM(SpuDebug);
EXT_PFD_DECLARE_ITEM(VerletBounds);
EXT_PFD_DECLARE_ITEM(ClothNormals);
EXT_PFD_DECLARE_ITEM(ClothDeltas);
EXT_PFD_DECLARE_ITEM(ClothFaceNormals);

bool g_PauseClothSimulation = false;
bool phVerletCloth::sm_SpuEnvUpdate = true;

namespace phClothStats
{
	PF_PAGE(Cloth,"Cloth");

	PF_GROUP(ClothUpdate);
	PF_LINK(Cloth,ClothUpdate);

	PF_TIMER(ClothManager,ClothUpdate);
	PF_TIMER(ClothIntegration,ClothUpdate);
	PF_TIMER(ClothCalculateDeltaPositions,ClothUpdate);
	PF_TIMER(ClothIteration,ClothUpdate);
	PF_TIMER(EnvClothCheckDistance,ClothUpdate);
	PF_TIMER(GetVertexNormalsFromDrawable,ClothUpdate);	
	PF_TIMER(GetVertexNormals,ClothUpdate);		
	PF_TIMER(ApplyAirResistance,ClothUpdate);
	PF_TIMER(ApplyAirResistanceWithCompression,ClothUpdate);
	PF_TIMER(ApplyAirResistanceNoCompression,ClothUpdate);
	PF_TIMER(RopeIntegration,ClothUpdate);
	PF_TIMER(EnvClothUpdate,ClothUpdate);
	PF_TIMER(EnvClothSimUpdate,ClothUpdate);
	PF_TIMER(EnvClothPostSimUpdate,ClothUpdate);
	PF_TIMER(EnvClothSimSpuUpdate,ClothUpdate);
	PF_TIMER(EnvClothPostSpuUpdate,ClothUpdate);	
	PF_TIMER(EnvRopeUpdate,ClothUpdate);
	PF_TIMER(UpdateClothEdges,ClothUpdate);
	PF_TIMER(UpdateRopeEdges,ClothUpdate);
	PF_TIMER(ClothCollisions,ClothUpdate);
	PF_TIMER(ClothCollisionsSphere,ClothUpdate);
	PF_TIMER(ClothCollisionsCapsule,ClothUpdate);
	PF_TIMER(ClothCollisionsTaperedCapsule,ClothUpdate);
	PF_TIMER(ClothCollisionsGeometry,ClothUpdate);
}

namespace phClothControllerStats
{
	EXT_PF_TIMER( ClothMapping );	
}


using namespace phClothStats;
using namespace phClothControllerStats;

void phVerletCloth::UpdateEdgesVec3V ( __vector4* XENON_ONLY(RESTRICT) clothVertexPositions, const phEdgeData* XENON_ONLY(RESTRICT) pEdgeData )
{
	PF_FUNC(UpdateRopeEdges);

	Vec3V_Ptr vertexPositions = Vec3V_Ptr(clothVertexPositions);
	/// ScalarV zero = ScalarV(V_ZERO);
	ScalarV one = ScalarV(V_ONE);
	ScalarV negOne = ScalarV(V_NEGONE);
	ScalarV two = ScalarV(V_TWO);

	const int firstEdge = GetNumLockedEdgesFront();
	const int lastEdge = m_NumEdges - GetNumLockedEdgesBack();

	for (register int edgeIndex = firstEdge; edgeIndex < lastEdge; ++edgeIndex )
	{
		const phEdgeData& edgeData = pEdgeData[edgeIndex];
		const u16 edgeVert0 = edgeData.m_vertIndices[0];
		const u16 edgeVert1 = edgeData.m_vertIndices[1];
		Vec3V vertexPos0 = vertexPositions[edgeVert0];
		Vec3V vertexPos1 = vertexPositions[edgeVert1];

		Vec3V edge1to0 = Subtract(vertexPos0,vertexPos1);
		ScalarV length2 = MagSquared(edge1to0);

		ScalarV restLength2 = ScalarVFromF32(edgeData.m_EdgeLength2);
		BoolV isCompressed = IsLessThan(length2,restLength2);
		ScalarV compressionWeight = SelectFT(isCompressed,one,ScalarVFromF32(edgeData.m_CompressionWeight));	

#if NO_ROPE_WEIGHTS
		ScalarV weight0 = ScalarVFromF32(edgeData.m_Weight0);
		ScalarV weight1 = Subtract( one, weight0 );

		weight0 = Scale( weight0, compressionWeight );
		weight1 = Scale( weight1, compressionWeight );
#else
		ScalarV weight0 = Scale(edgeData.GetRopeVertexWeight0(),compressionWeight);
		ScalarV weight1 = Scale(edgeData.GetRopeVertexWeight1(),compressionWeight);
#endif

		// Approximate 1-sqrt(restLength2/length2) with 1-2*restLength2/(length2+restLength2)
		ScalarV constraintViolation = SubtractScaled(one, two, Scale(restLength2,Invert(Add(length2,restLength2))));
		constraintViolation = Clamp(constraintViolation,negOne,one);

		vertexPositions[edgeVert0] = AddScaled(vertexPos0, edge1to0, Negate(Scale(weight0,constraintViolation)) );
		vertexPositions[edgeVert1] = AddScaled(vertexPos1, edge1to0,		Scale(weight1,constraintViolation) );
	}
}


void phVerletCloth::UpdateEdgesSoAVec3VBy8s (int numEdges, __vector4* XENON_ONLY(RESTRICT) clothVertexPositions, const phEdgeData* XENON_ONLY(RESTRICT) pEdgeData, float overRelaxationConstant	)
{
	PF_FUNC(UpdateClothEdges);

	Vec3V* RESTRICT vertexPositions = (Vec3V* RESTRICT)clothVertexPositions;

	ScalarV relaxation = ScalarVFromF32(overRelaxationConstant);
	ScalarV zero = ScalarV(V_ZERO);
	ScalarV one = ScalarV(V_ONE);
	ScalarV two = ScalarV(V_TWO);

	// In this SoA version:
	// 1) We are saving vector registers.
	// 2) We are saving instructions.
	// 3) We are better hiding latency.

	SoA_ScalarV soa_zero = SoA_ScalarV( zero.GetIntrin128() );
	SoA_ScalarV soa_one = SoA_ScalarV( one.GetIntrin128() );
	SoA_ScalarV soa_two = SoA_ScalarV( two.GetIntrin128() );

	Assert( numEdges > 7 );

	int numEdges8 = (numEdges >> 3) << 3;

	PrefetchDC( &pEdgeData[0] );				// 8 edges fit in one cache line
	int edgeIndex;
	for( edgeIndex=0; edgeIndex < numEdges8; edgeIndex+=8)
	{
		PrefetchDC( &pEdgeData[edgeIndex+8] );				// 8 edges fit in one cache line

		const int i0 = edgeIndex + 0;
		const int i1 = edgeIndex + 1;
		const int i2 = edgeIndex + 2;
		const int i3 = edgeIndex + 3;
		const int i4 = edgeIndex + 4;
		const int i5 = edgeIndex + 5;
		const int i6 = edgeIndex + 6;
		const int i7 = edgeIndex + 7;

		const phEdgeData* RESTRICT edge0 = &pEdgeData[i0];
		const phEdgeData* RESTRICT edge1 = &pEdgeData[i1];
		const phEdgeData* RESTRICT edge2 = &pEdgeData[i2];
		const phEdgeData* RESTRICT edge3 = &pEdgeData[i3];
		const phEdgeData* RESTRICT edge4 = &pEdgeData[i4];
		const phEdgeData* RESTRICT edge5 = &pEdgeData[i5];
		const phEdgeData* RESTRICT edge6 = &pEdgeData[i6];
		const phEdgeData* RESTRICT edge7 = &pEdgeData[i7];

		const u16 edge0Vert0 = edge0->m_vertIndices[0];
		const u16 edge0Vert1 = edge0->m_vertIndices[1];
		const u16 edge1Vert0 = edge1->m_vertIndices[0];
		const u16 edge1Vert1 = edge1->m_vertIndices[1];
		const u16 edge2Vert0 = edge2->m_vertIndices[0];
		const u16 edge2Vert1 = edge2->m_vertIndices[1];
		const u16 edge3Vert0 = edge3->m_vertIndices[0];
		const u16 edge3Vert1 = edge3->m_vertIndices[1];
		const u16 edge4Vert0 = edge4->m_vertIndices[0];
		const u16 edge4Vert1 = edge4->m_vertIndices[1];
		const u16 edge5Vert0 = edge5->m_vertIndices[0];
		const u16 edge5Vert1 = edge5->m_vertIndices[1];
		const u16 edge6Vert0 = edge6->m_vertIndices[0];
		const u16 edge6Vert1 = edge6->m_vertIndices[1];
		const u16 edge7Vert0 = edge7->m_vertIndices[0];
		const u16 edge7Vert1 = edge7->m_vertIndices[1];

		Vec4V vEdgeVars0 = edge0->GetEdgeDataAsVector();
		Vec4V vEdgeVars1 = edge1->GetEdgeDataAsVector();
		Vec4V vEdgeVars2 = edge2->GetEdgeDataAsVector();
		Vec4V vEdgeVars3 = edge3->GetEdgeDataAsVector();
		Vec4V vEdgeVars4 = edge4->GetEdgeDataAsVector();
		Vec4V vEdgeVars5 = edge5->GetEdgeDataAsVector();
		Vec4V vEdgeVars6 = edge6->GetEdgeDataAsVector();
		Vec4V vEdgeVars7 = edge7->GetEdgeDataAsVector();

		// Turn them on their side...
		// Y component = m_EdgeLength2 (x4)
		// Z component = m_Weight0 (x4)
		// W component = m_CompressionWeight (x4)
		SoA_Vec4V soa_vEdgeVars0123, soa_vEdgeVars4567;
		ToSoA( soa_vEdgeVars0123, vEdgeVars0, vEdgeVars1, vEdgeVars2, vEdgeVars3 );
		ToSoA( soa_vEdgeVars4567, vEdgeVars4, vEdgeVars5, vEdgeVars6, vEdgeVars7 );

		Vec3V vertex0Pos0 = vertexPositions[edge0Vert0];
		Vec3V vertex0Pos1 = vertexPositions[edge0Vert1];
		Vec3V vertex1Pos0 = vertexPositions[edge1Vert0];
		Vec3V vertex1Pos1 = vertexPositions[edge1Vert1];
		Vec3V vertex2Pos0 = vertexPositions[edge2Vert0];
		Vec3V vertex2Pos1 = vertexPositions[edge2Vert1];
		Vec3V vertex3Pos0 = vertexPositions[edge3Vert0];
		Vec3V vertex3Pos1 = vertexPositions[edge3Vert1];
		Vec3V vertex4Pos0 = vertexPositions[edge4Vert0];
		Vec3V vertex4Pos1 = vertexPositions[edge4Vert1];
		Vec3V vertex5Pos0 = vertexPositions[edge5Vert0];
		Vec3V vertex5Pos1 = vertexPositions[edge5Vert1];
		Vec3V vertex6Pos0 = vertexPositions[edge6Vert0];
		Vec3V vertex6Pos1 = vertexPositions[edge6Vert1];
		Vec3V vertex7Pos0 = vertexPositions[edge7Vert0];
		Vec3V vertex7Pos1 = vertexPositions[edge7Vert1];

		// Turn them on their side...
		SoA_Vec3V soa_vertex0Pos0_1Pos0_2Pos0_3Pos0;
		SoA_Vec3V soa_vertex4Pos0_5Pos0_6Pos0_7Pos0;
		SoA_Vec3V soa_vertex0Pos1_1Pos1_2Pos1_3Pos1;
		SoA_Vec3V soa_vertex4Pos1_5Pos1_6Pos1_7Pos1;
		ToSoA( soa_vertex0Pos0_1Pos0_2Pos0_3Pos0, vertex0Pos0, vertex1Pos0, vertex2Pos0, vertex3Pos0 );
		ToSoA( soa_vertex4Pos0_5Pos0_6Pos0_7Pos0, vertex4Pos0, vertex5Pos0, vertex6Pos0, vertex7Pos0 );
		ToSoA( soa_vertex0Pos1_1Pos1_2Pos1_3Pos1, vertex0Pos1, vertex1Pos1, vertex2Pos1, vertex3Pos1 );
		ToSoA( soa_vertex4Pos1_5Pos1_6Pos1_7Pos1, vertex4Pos1, vertex5Pos1, vertex6Pos1, vertex7Pos1 );

		SoA_ScalarV soa_restLengthSquared0123 = soa_vEdgeVars0123.GetY();
		SoA_ScalarV soa_restLengthSquared4567 = soa_vEdgeVars4567.GetY();

		SoA_ScalarV soa_weight00_10_20_30 = soa_vEdgeVars0123.GetZ();
		SoA_ScalarV soa_weight40_50_60_70 = soa_vEdgeVars4567.GetZ();

		SoA_Vec3V soa_edge01to0_11to0_21to0_31to0, soa_edge41to0_51to0_61to0_71to0;
		Subtract( soa_edge01to0_11to0_21to0_31to0, soa_vertex0Pos0_1Pos0_2Pos0_3Pos0, soa_vertex0Pos1_1Pos1_2Pos1_3Pos1 );
		Subtract( soa_edge41to0_51to0_61to0_71to0, soa_vertex4Pos0_5Pos0_6Pos0_7Pos0, soa_vertex4Pos1_5Pos1_6Pos1_7Pos1 );

		SoA_ScalarV soa_currLengthSquared0123 = MagSquared( soa_edge01to0_11to0_21to0_31to0 );
		SoA_ScalarV soa_currLengthSquared4567 = MagSquared( soa_edge41to0_51to0_61to0_71to0 );

		SoA_ScalarV soa_weight01_11_21_31 = Subtract( soa_one, soa_weight00_10_20_30 );
		SoA_ScalarV soa_weight41_51_61_71 = Subtract( soa_one, soa_weight40_50_60_70 );

		SoA_VecBool1V soa_isCompressed0123 = IsLessThan( soa_currLengthSquared0123, soa_restLengthSquared0123 );
		SoA_VecBool1V soa_isCompressed4567 = IsLessThan( soa_currLengthSquared4567, soa_restLengthSquared4567 );

		SoA_ScalarV soa_compressionWeight0123 = SelectFT( soa_isCompressed0123, soa_one, soa_vEdgeVars0123.GetW() );
		SoA_ScalarV soa_compressionWeight4567 = SelectFT( soa_isCompressed4567, soa_one, soa_vEdgeVars4567.GetW() );

		soa_weight00_10_20_30 = Scale( soa_weight00_10_20_30, soa_compressionWeight0123 );
		soa_weight40_50_60_70 = Scale( soa_weight40_50_60_70, soa_compressionWeight4567 );
		soa_weight01_11_21_31 = Scale( soa_weight01_11_21_31, soa_compressionWeight0123 );
		soa_weight41_51_61_71 = Scale( soa_weight41_51_61_71, soa_compressionWeight4567 );

		// Add() result. (Innermost)
		SoA_ScalarV soa_a_0123 = Add( soa_currLengthSquared0123, soa_restLengthSquared0123 );
		SoA_ScalarV soa_a_4567 = Add( soa_currLengthSquared4567, soa_restLengthSquared4567 );
		// InvertFast() result and Scale() result.
		SoA_ScalarV soa_c_0123 = Scale( InvertFast( soa_a_0123 ), soa_restLengthSquared0123 );
		SoA_ScalarV soa_c_4567 = Scale( InvertFast( soa_a_4567 ), soa_restLengthSquared4567 );
		// Subtract() result.
		SoA_ScalarV soa_e_0123 = SubtractScaled( soa_one, soa_two, soa_c_0123 );
		SoA_ScalarV soa_e_4567 = SubtractScaled( soa_one, soa_two, soa_c_4567 );

		// Scale() result. (Outermost)// Apply overrelaxation
		SoA_ScalarV soa_constraintViolation0123 = Scale( soa_e_0123, SoA_ScalarV(relaxation.GetIntrin128()) );
		SoA_ScalarV soa_constraintViolation4567 = Scale( soa_e_4567, SoA_ScalarV(relaxation.GetIntrin128()) );

		SoA_ScalarV s00_10_20_30 = SubtractScaled( soa_zero, soa_weight00_10_20_30, soa_constraintViolation0123 );
		SoA_ScalarV s40_50_60_70 = SubtractScaled( soa_zero, soa_weight40_50_60_70, soa_constraintViolation4567 );

		SoA_ScalarV s01_11_21_31 = Scale( soa_weight01_11_21_31, soa_constraintViolation0123 );
		SoA_ScalarV s41_51_61_71 = Scale( soa_weight41_51_61_71, soa_constraintViolation4567 );

		SoA_Vec3V soa_x;
		SoA_Vec3V soa_y;
		SoA_Vec3V soa_z;
		SoA_Vec3V soa_w;
		AddScaled( soa_x, soa_vertex0Pos0_1Pos0_2Pos0_3Pos0, soa_edge01to0_11to0_21to0_31to0, SoA_Vec3V(s00_10_20_30) );
		AddScaled( soa_y, soa_vertex0Pos1_1Pos1_2Pos1_3Pos1, soa_edge01to0_11to0_21to0_31to0, SoA_Vec3V(s01_11_21_31) );
		AddScaled( soa_z, soa_vertex4Pos0_5Pos0_6Pos0_7Pos0, soa_edge41to0_51to0_61to0_71to0, SoA_Vec3V(s40_50_60_70) );
		AddScaled( soa_w, soa_vertex4Pos1_5Pos1_6Pos1_7Pos1, soa_edge41to0_51to0_61to0_71to0, SoA_Vec3V(s41_51_61_71) );

		// Turn them back right-side-up again, and return.
		Vec3V e0v0, e0v1, e1v0, e1v1;
		Vec3V e2v0, e2v1, e3v0, e3v1;
		Vec3V e4v0, e4v1, e5v0, e5v1;
		Vec3V e6v0, e6v1, e7v0, e7v1;
		ToAoS( e0v0, e1v0, e2v0, e3v0, soa_x );
		ToAoS( e0v1, e1v1, e2v1, e3v1, soa_y );
		ToAoS( e4v0, e5v0, e6v0, e7v0, soa_z );
		ToAoS( e4v1, e5v1, e6v1, e7v1, soa_w );

		vertexPositions[edge0Vert0] = e0v0;
		vertexPositions[edge0Vert1] = e0v1;
		vertexPositions[edge1Vert0] = e1v0;
		vertexPositions[edge1Vert1] = e1v1;
		vertexPositions[edge2Vert0] = e2v0;
		vertexPositions[edge2Vert1] = e2v1;
		vertexPositions[edge3Vert0] = e3v0;
		vertexPositions[edge3Vert1] = e3v1;
		vertexPositions[edge4Vert0] = e4v0;
		vertexPositions[edge4Vert1] = e4v1;
		vertexPositions[edge5Vert0] = e5v0;
		vertexPositions[edge5Vert1] = e5v1;
		vertexPositions[edge6Vert0] = e6v0;
		vertexPositions[edge6Vert1] = e6v1;
		vertexPositions[edge7Vert0] = e7v0;
		vertexPositions[edge7Vert1] = e7v1;
	}

	// process the extra edges added from tearing
	for( ; edgeIndex < numEdges; ++edgeIndex )
	{
		const phEdgeData* RESTRICT edge = &pEdgeData[edgeIndex];
		u16 edgeVert0 = edge->m_vertIndices[0];
		u16 edgeVert1 = edge->m_vertIndices[1];

		Vec4V vEdgeVars0 = edge->GetEdgeDataAsVector();

		ScalarV restLenSqr	= vEdgeVars0.GetY();		// Y component = m_EdgeLength2
		ScalarV weight0		= vEdgeVars0.GetZ();		// Z component = m_Weight0
		ScalarV compWeight	= vEdgeVars0.GetW();		// W component = m_CompressionWeight

		Vec3V vertexPos0 = vertexPositions[edgeVert0];
		Vec3V vertexPos1 = vertexPositions[edgeVert1];

		Vec3V edge0to1 = Subtract(vertexPos0,vertexPos1);
		ScalarV currLenSqr = MagSquared( edge0to1 );
		ScalarV weight1 = Subtract( one, weight0 );

		BoolV isCompressed = IsLessThan(currLenSqr,restLenSqr);
		compWeight = SelectFT( isCompressed, one, compWeight );

		weight0 = Scale( weight0, compWeight );
		weight1 = Scale( weight1, compWeight );

		ScalarV lenSqr = Add( currLenSqr, restLenSqr );
		ScalarV invLenSqr = InvertFast( lenSqr );
		ScalarV stretch = Scale( invLenSqr, restLenSqr );
		ScalarV average = SubtractScaled( one, two, stretch );

		ScalarV constraintViolation = Scale( average, relaxation );
		ScalarV newWeight0 = SubtractScaled( zero, weight0, constraintViolation );
		ScalarV newWeight1 = Scale( weight1, constraintViolation );		

		vertexPositions[edgeVert0] = AddScaled( vertexPos0, edge0to1, newWeight0 );
		vertexPositions[edgeVert1] = AddScaled( vertexPos1, edge0to1, newWeight1 );
	}
}



#if __WIN32
#pragma warning( disable:4100 )
#endif


#if __ASSERT
void phVerletCloth::CheckVerts(const char *strCodeFile, const char* strCodeFunction, u32 nCodeLine)
{
	Vec3V* RESTRICT verts = m_ClothData.GetVertexPointer();
	for( int i = 0; i < GetNumVertices(); ++i )
	{
		Assertf(IsLessThanAll(Abs(verts[i]), Vec3V(V_FLT_LARGE_6)), "Vertex Index %d. %s %s %d.", i, strCodeFile, strCodeFunction, nCodeLine );
	}
}
#endif

void phVerletCloth::ComputeClothBoundingVolume()
{
	Vec3V minV(V_FLT_MAX);
	Vec3V maxV(V_NEG_FLT_MAX);

#if !NO_BOUND_CENTER_RADIUS
	Vec3V center(V_ZERO);
	int nVert = GetNumVertices();
	Assert( nVert );
#endif

	const Vec3V _fltMaxV(V_FLT_MAX);
	const Vec3V _zero(V_ZERO);

	const Vec3V* RESTRICT verts = m_ClothData.GetVertexPointer();
	const int firstVertex = GetNumLockedEdgesFront();
	const int lastVertex = GetNumVertices() - GetNumLockedEdgesBack();

	for( int i = firstVertex; i < lastVertex; ++i )
	{		
		Vec3V p = verts[i];
		VecBoolV nonZeroIfNewMin = IsLessThan( p, minV );
		VecBoolV nonZeroIfNewMax = IsGreaterThan( p, maxV );
		
		minV = SelectFT( nonZeroIfNewMin, minV, p ); // TODO -- minV = Min( p, minV ); ?
		maxV = SelectFT( nonZeroIfNewMax, maxV, p ); // TODO -- maxV = Max( p, maxV ); ?
#if !NO_BOUND_CENTER_RADIUS
		VecBoolV nonZeroIfFinite = IsLessThan( p, _fltMaxV );
		center = Add( center, SelectFT( nonZeroIfFinite, _zero, p ));
#endif
	}

	m_BBMin = minV;
	m_BBMax = maxV;

#if !NO_BOUND_CENTER_RADIUS
	center = Scale( center, InvertFast( ScalarVFromF32( (float)nVert) ) );
	m_BoundingCenterAndRadius.SetVector3( VEC3V_TO_VECTOR3(center) );

	Vec3V maxMinusCenter = Abs( Subtract( maxV, center ) );
	Vec3V centerMinusMin = Abs( Subtract( center, minV ) );

	Vec3V farthestCorner = SelectFT( IsGreaterThan(centerMinusMin,maxMinusCenter), maxV, minV );

	Vec3V tempV = Subtract( center, farthestCorner );
	ScalarV radiusSquare = Dot( tempV, tempV );

	ScalarV radius = SelectFT( IsLessThan( radiusSquare, ScalarVFromF32( MAX_CLOTH_RADIUS_SQR ) ), ScalarVFromF32(MAX_CLOTH_RADIUS), Sqrt(radiusSquare) );
	RC_VEC4V(m_BoundingCenterAndRadius).SetW( radius );
#endif
}

void phVerletCloth::ComputeBoundingVolume( phBound* bound, Vector3* worldCenter, float* radius )
{
	ComputeClothBoundingVolume();
	Vec3V center = GetCenter();
	float boundingRadius = GetRadius(center);

	// NOTE: only rope has composite bound
	if (bound && bound->GetType()==phBound::COMPOSITE)
	{
		// Compute the composite extents.
		phBoundComposite& compositeBound = *static_cast<phBoundComposite*>(bound);
		compositeBound.CalculateCompositeExtents();

		if (GetFlag(FLAG_IS_ROPE))
		{
			// Add the rope radius to the bounding radius, and set it again.
			// TODO: fix the hardcoded number
			boundingRadius += 0.025f;

#if !NO_BOUND_CENTER_RADIUS
			m_BoundingCenterAndRadius.SetW( boundingRadius );
#endif

			// Keep the composites computed box, but make it have the computed rope's bounding radius.
			compositeBound.SetRadiusAroundCentroid(ScalarVFromF32(boundingRadius));

#if !NO_CLOTH_BOUND_CLAMP
			// Clamp the bound size.
			if (ClampCompositeExtents(compositeBound,MAX_CLOTH_RADIUS))
			{
	#if !NO_BOUND_CENTER_RADIUS
				// The composite extents were clamped, so clamp the cloth's bounding radius too.
				m_BoundingCenterAndRadius.SetW( MAX_CLOTH_RADIUS );
	#endif
			}
#endif
		}
	}

	if (worldCenter)
	{
#if NO_BOUND_CENTER_RADIUS
		*worldCenter = VEC3V_TO_VECTOR3(center);
#else
		worldCenter->Set( m_BoundingCenterAndRadius[0], m_BoundingCenterAndRadius[1], m_BoundingCenterAndRadius[2] );
#endif
	}

	if (radius)
	{
#if NO_BOUND_CENTER_RADIUS
		*radius = boundingRadius;
#else
		*radius = m_BoundingCenterAndRadius[3];
#endif
	}
}


#if !NO_BOUND_CENTER_RADIUS
// [SPHERE-OPTIMISE]
void phVerletCloth::GetBoundingSphere (Vector3& centre, float& radius) const
{
	centre = m_BoundingCenterAndRadius.GetVector3(); 
	radius = m_BoundingCenterAndRadius.GetW(); 
}
#endif

void phVerletCloth::UpdateCharacterCloth( float timeScale, Mat34V_In gravityTransform, Mat34V_In attachedFrame, float timeStep, Vec3V_In gravityV, ScalarV_In gravityScale, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset)
{
#if NO_PIN_VERTS_IN_VERLET
	const int numPinnedVerts = GetClothData().GetNumPinVerts();
#else
	const int numPinnedVerts = GetPinCount();
#endif

	float dampTimeScale = DAMP_TIME_SCALE;
	float damping = DEFAULT_VERLET_DAMPING * Selectf( timeScale - 1.0f, 1.0f, timeScale * dampTimeScale );		// increase the damp value x times if time is scaled
	ScalarV scaledDamping = ScalarVFromF32(damping);

	scaledDamping = -scaledDamping;
	ScalarV one(V_ONE);
	scaledDamping = scaledDamping + one;

	ScalarV timeStepV = ScalarVFromF32(timeStep);
	Vec3V gravity = Transform( gravityTransform, gravityV/*phSimulator::GetGravityV()*/ );

	Vec3V gravityDelVel = Scale( Scale(gravity,timeStepV), gravityScale );
	Vec3V gravityDisplacement = Scale(gravityDelVel,timeStepV);
	gravityDisplacement.SetW( ScalarV(V_ZERO) );

#if __SPU
	if( !gSPU_PauseClothSimulation )
#else
	if( !g_PauseClothSimulation )
#endif
	{
		UpdateCloth( attachedFrame, m_ClothData.GetVertexPointer(), m_CollisionInst, numPinnedVerts, gravityDisplacement, scaledDamping, customMatrices, parentOffset );
	}
}


void phVerletCloth::UpdateCharacterClothAgainstCollisionInsts( Mat34V_In attachedFrame, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset)
{
#if NO_PIN_VERTS_IN_VERLET
	const int numPinnedVerts = GetClothData().GetNumPinVerts();
#else
	const int numPinnedVerts = GetPinCount();
#endif

	UpdateClothAgainstCollisionInsts( attachedFrame, m_ClothData.GetVertexPointer(), m_CollisionInst, numPinnedVerts, customMatrices, parentOffset );
}



#if __PPU && ENABLE_SPU_DEBUG_DRAW

u8	*gDebugDrawBuf;
i128type *gDebugDrawPrimCount;

#endif

#if ENABLE_SPU_DEBUG_DRAW
u8*	gpDebugDrawBuf;
int	gSPUDebugDrawSpheresCount;
int	gSPUDebugDrawTrianglesCount;
int	gSPUDebugDrawBoxesCount;
int gSPUDebugDrawCapsulesCount;
#endif

bool gSPU_PauseClothSimulation = false;	

#if __PPU && ENABLE_SPU_COMPARE
u8 gCompareBuf[1024*1024] ;
#endif

#if ENABLE_SPU_COMPARE
u8* gpCompareBuf;
int gSpuCompareFailed = 0;
bool gEnableSpuCompare = true;
#endif

sysTaskHandle phVerletCloth::UpdateEnvironmentClothSpu(	
	Mat34V_In attachedFrame, ScalarV_In timeStep, phInstDatRefArray& collisionInst, 
	ScalarV_In gravityScale, grmGeometryQB* pGeom, void* pClothController, int sizeOfClothController, Vec3V_In force,
	int simLodIndex, phVerletCloth* drwCloth, 
	bool 
#if CLOTH_SIM_MESH_ONLY
	bClothSimMeshOnly
#endif
	,
	const bool separateMotion, const u32 userDataAddress, phVerletSPUDebug* SPU_DEBUG_DRAW_ONLY(verletSpuDebug) )
{
#if (__PPU)
	if (sm_SpuEnvUpdate)
	{
		Assert( simLodIndex > -1 && simLodIndex < 4 );
//		Assert( drwLodIndex > -1 && drwLodIndex < 4 );

		int inputsize = ((sizeof(*this)+15)&~15) + ((sizeOfClothController+15)&~15) + 16;
#if CLOTH_SIM_MESH_ONLY
		if( !bClothSimMeshOnly )
#endif
		{
			Assert( pGeom );
			inputsize += sizeof(grmGeometryQB);
		}
		Assert( pClothController );

		
		// stack used in DetectAndEnforceInstance and other physics stuff ?!
		int stacksize = 10 * 1024 ;
		int reduceScratchSize = 2 * 1024;
		if( 0/*drwLodIndex*/ != simLodIndex )
		{
			inputsize += ((sizeof(*this)+15)&~15);		// the second cloth goes in too
		}
#if CLOTH_SIM_MESH_ONLY	
		if( !bClothSimMeshOnly )
#endif
		{
			grcVertexBuffer* vb = pGeom->GetVertexBuffer();
			Assert( vb );
			vb->Lock();
		}

		int scratchSize = kMaxSPUJobSize - stacksize - inputsize - (int)TASK_INTERFACE_SIZE(UpdateEnvironmentCloth) - reduceScratchSize;	
		sysTaskContext c(TASK_INTERFACE(UpdateEnvironmentCloth), 0, scratchSize, stacksize);

		c.SetInputOutput();
		c.AddInput(this,((sizeof(*this)+15)&~15) );

		if( 0/*drwLodIndex*/ != simLodIndex )
		{
			Assert( drwCloth );
			c.AddInput(drwCloth,((sizeof(*this)+15)&~15) );
		}

		c.AddInput(pClothController, ((sizeOfClothController+15)&~15) );
#if CLOTH_SIM_MESH_ONLY
		if( !bClothSimMeshOnly )
#endif
		{
			c.AddInput(pGeom, sizeof(grmGeometryQB));
		}

		phVerletClothUpdate& u = *c.AllocUserDataAs<phVerletClothUpdate>();
		u.m_Gravity = VEC3V_TO_VECTOR3(Scale(phSimulator::GetGravityV(), gravityScale));
		StoreScalar32FromScalarV(u.m_Gravity.w, timeStep);

#if NO_BOUND_CENTER_RADIUS
		u.boundingCenterAndRadiusAddress = (u32)&m_BBMin;
#else
		u.boundingCenterAndRadiusAddress = (u32)&m_BoundingCenterAndRadius;
#endif
		u.instLastMtxIdxAddrMM	= PHLEVEL->GetLastInstanceMatrixIndexMapBaseAddr();
		u.instLastMatricsAddrMM = PHLEVEL->GetLastInstanceMatricesBaseAddr();
		u.m_Frame				= attachedFrame;
		u.m_Force				= VEC3V_TO_VECTOR3(force);
		u.simLodIndex			= simLodIndex;
		u.drwLodIndex			= 0/*drwLodIndex*/;
		u.separateMotion		= separateMotion;
		u.userDataAddress		= userDataAddress;
#if CLOTH_SIM_MESH_ONLY
		u.bClothSimMeshOnly		= bClothSimMeshOnly;
#else
		u.bClothSimMeshOnly		= false;
#endif
		u.pauseSimulation		= g_PauseClothSimulation;

		Assign(u.vertexBufferIdx, grmGeometry::GetVertexBufferIndex());
#if ENABLE_SPU_COMPARE
		if (gEnableSpuCompare)
			gpCompareBuf = gCompareBuf;

		gSpuCompareFailed = 0;
		u.compareBuf			= gpCompareBuf;
#endif

#if ENABLE_SPU_DEBUG_DRAW
		if( verletSpuDebug )
		{
			Assert( verletSpuDebug->debugIdx < MAX_VERLET_SPU_DEBUG );
			Vec4V* v = (Vec4V*)&gDebugDrawPrimCount[verletSpuDebug->debugIdx]; 
			v->ZeroComponents();

			verletSpuDebug->debugDrawCountOut	= (u32)v;
			verletSpuDebug->debugDrawBuf = gDebugDrawBuf + verletSpuDebug->debugIdx * VERLET_DEBUG_BUFFER;

			u.verletSpuDebugAddr = (u32)verletSpuDebug;
		}
		else
		{
			u.verletSpuDebugAddr = 0;
		}
#endif

		SetCollisionInst( collisionInst );

		sysTaskHandle h = c.Start();
#if ENABLE_SPU_COMPARE
		sysTaskManager::Wait(h);
		UpdateEnvironmentCloth( force.GetWf(), timeStep, collisionInst, VECTOR3_TO_VEC3V(u.m_Gravity), gravityScale);
		Assert(gSpuCompareFailed < 64);
		gpCompareBuf = 0;
		return 0;
#else
		return h;
#endif
	}
	else
#endif // (__PPU)
	{
		Assert(0); // TODO: need parent offset , not V_ZERO
		UpdateEnvironmentCloth( Vec3V(V_ZERO), force.GetWf(), timeStep, collisionInst, phSimulator::GetGravityV(), gravityScale );
		return 0;
	}
}

#if ENABLE_SPU_DEBUG_DRAW

#define		VERLET_BUFFER_TYPE_SIZE		1600

#endif

#if __PPU 

void phVerletCloth::InitDebugSPU()
{
#if ENABLE_SPU_DEBUG_DRAW
	{
		USE_DEBUG_MEMORY();
		Assert(!gDebugDrawBuf && !gDebugDrawPrimCount);
		gDebugDrawBuf = rage_aligned_new(16) u8[DEBUG_DRAW_BUFFER_SIZE];
		gDebugDrawPrimCount = rage_aligned_new(16) i128type[MAX_VERLET_SPU_DEBUG];
	}

	for( int k = 0; k < MAX_VERLET_SPU_DEBUG; ++k )
	{
		Vec4V* v = (Vec4V*)&gDebugDrawPrimCount[k]; 
		v->ZeroComponents();
	}
#endif
}

void phVerletCloth::ShutdownDebugSPU()
{
#if ENABLE_SPU_DEBUG_DRAW
	USE_DEBUG_MEMORY();
	delete[] gDebugDrawBuf;
	delete[] gDebugDrawPrimCount;
#endif
}


void phVerletCloth::DebugDrawSPU()
{
#if ENABLE_SPU_DEBUG_DRAW
#if __PFDRAW
	if( PFD_SpuDebug.Begin(true) )
	{
		Color32 color = Color_white;
		for( int k = 0; k < MAX_VERLET_SPU_DEBUG; ++k )
		{
			const u8* RESTRICT drawBufferStart = (gDebugDrawBuf + k*VERLET_DEBUG_BUFFER);
			const Vec3V* drawBuf = (const Vec3V*)( drawBufferStart + VERLET_BUFFER_TYPE_SIZE * SPU_DEBUG_SPHERE );
			const int* RESTRICT counters = (const int*)&gDebugDrawPrimCount[k];
			grcColor( Color_red );
			for( int i = 0; i < counters[SPU_DEBUG_SPHERE]; ++i )
			{
				grcDrawSphere( 0.1f, VEC3V_TO_VECTOR3(drawBuf[i]) );
			}

			grcColor( Color_white );
			drawBuf = (const Vec3V*)( drawBufferStart + VERLET_BUFFER_TYPE_SIZE * SPU_DEBUG_BOX );
			for( int i = 0; i < counters[SPU_DEBUG_BOX]; ++i )
			{
				const int offset = i*2;			
				grcDrawBox( VEC3V_TO_VECTOR3(drawBuf[offset]), VEC3V_TO_VECTOR3(drawBuf[offset+1]), Color_white );
			}

			drawBuf = (const Vec3V*)( drawBufferStart + VERLET_BUFFER_TYPE_SIZE * SPU_DEBUG_TRIANGLE );
			for( int i = 0; i < counters[SPU_DEBUG_TRIANGLE]; ++i )
			{
				const int offset = i*3;
				Vec3V v0 = drawBuf[offset];
				Vec3V v1 = drawBuf[offset+1];
				Vec3V v2 = drawBuf[offset+2];

				grcDrawLine( VEC3V_TO_VECTOR3(v0), VEC3V_TO_VECTOR3(v1), color );
				grcDrawLine( VEC3V_TO_VECTOR3(v1), VEC3V_TO_VECTOR3(v2), color );
				grcDrawLine( VEC3V_TO_VECTOR3(v2), VEC3V_TO_VECTOR3(v0), color );
			}

			grcColor( Color_white );
			drawBuf = (const Vec3V*)( drawBufferStart + VERLET_BUFFER_TYPE_SIZE * SPU_DEBUG_CAPSULE );
			for( int i = 0; i < counters[SPU_DEBUG_CAPSULE]; ++i )
			{
				const int offset = i*6;
				Vec3V col0 = drawBuf[offset];
				Vec3V col1 = drawBuf[offset+1];
				Vec3V col2 = drawBuf[offset+2];
				Vec3V col3 = drawBuf[offset+3];
				ScalarV len( drawBuf[offset+4].GetIntrin128() );
				ScalarV rad( drawBuf[offset+5].GetIntrin128() );

				Mat34V capsuleMat;
				capsuleMat.SetCol0( col0 );
				capsuleMat.SetCol1( col1 );
				capsuleMat.SetCol2( col2 );
				capsuleMat.SetCol3( col3 );

				grcDrawCapsule( len.Getf(), rad.Getf(), MAT34V_TO_MATRIX34(capsuleMat), 8 );

				// 				Displayf("Draw capsule col0: %f, %f, %f", col0.GetXf(), col0.GetYf(), col0.GetZf() );
				// 				Displayf("Draw capsule col1: %f, %f, %f", col1.GetXf(), col1.GetYf(), col1.GetZf() );
				// 				Displayf("Draw capsule col2: %f, %f, %f", col2.GetXf(), col2.GetYf(), col2.GetZf() );
				// 				Displayf("Draw capsule col3: %f, %f, %f", col3.GetXf(), col3.GetYf(), col3.GetZf() );
			}

			Vec4V* v = (Vec4V*)&gDebugDrawPrimCount[k]; 
			v->ZeroComponents();
		} // end for k

		PFD_SpuDebug.End();
	}
#endif
#endif // ENABLE_SPU_DEBUG_DRAW
}
#endif

#if (__PPU)
void phVerletCloth::SetCollisionInst( phInstDatRefArray& collisionInst )
{
	int collisionInstCount = collisionInst.GetCount();

	m_CollisionInst.ResetCount();
	const int arrayCapacity = m_CollisionInst.GetCapacity();
	if( collisionInstCount > arrayCapacity )
	{
		Displayf("Collision instances %d are more than the capacity %d", collisionInstCount, arrayCapacity );
		collisionInstCount = arrayCapacity;
	}

	for(int i=0; i < collisionInstCount; ++i)
	{
		u32 fatID = collisionInst[i];
		int levelIndex = fatID & 0xffff;
#if LEVELNEW_GENERATION_IDS
		int genID = (fatID >> 16) & 0xffff;
		u32 collisionInstEA = (u32)
			((levelIndex != phInst::INVALID_INDEX &&
			PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndex, genID)) 
			? PHLEVEL->GetInstance(levelIndex) : NULL);
#else
		u32 collisionInstEA = (u32)
			((levelIndex != phInst::INVALID_INDEX) 
			? PHLEVEL->GetInstance(levelIndex) : NULL);
#endif
		if( collisionInstEA )		
			m_CollisionInst.Append() = collisionInstEA;
	}
}

void phVerletCloth::AddCollisionInst( u32 fatID )
{
	int levelIndex = fatID & 0xffff;
#if LEVELNEW_GENERATION_IDS
	int genID = (fatID >> 16) & 0xffff;
	u32 collisionInstEA = (u32)
		((levelIndex != phInst::INVALID_INDEX &&
		PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndex, genID)) 
		? PHLEVEL->GetInstance(levelIndex) : NULL);
#else
	u32 collisionInstEA = (u32)
		((levelIndex != phInst::INVALID_INDEX) 
		? PHLEVEL->GetInstance(levelIndex) : NULL);
#endif
	if( collisionInstEA && (m_CollisionInst.GetCount() < m_CollisionInst.GetCapacity()) )
		m_CollisionInst.Append() = collisionInstEA;
}
#endif // (__PPU)

void phVerletCloth::ResetCollisionInst()
{
	m_CollisionInst.Resize(0);
}

sysTaskHandle phVerletCloth::UpdateEnvironmentRopeSpu( ScalarV_In timeStep, phInstDatRefArray& collisionInst, 
	ScalarV_In gravityScale, void* pClothController, int sizeOfClothController )
{
#if (__PPU)
	if (sm_SpuEnvUpdate)
	{
		const int allignedSizeOfCloth = ((sizeof(*this)+15)&~15);
		const int allignedSizeOfController = ((sizeOfClothController+15)&~15);

		int inputsize = allignedSizeOfCloth + allignedSizeOfController + 16;		
		int stacksize = 24* 1024;		// BVH uses walkStacklessTree which require alot of stacksize
		int scratchSize = kMaxSPUJobSize - stacksize - inputsize - (int)TASK_INTERFACE_SIZE(UpdateEnvironmentRope) - 1024;	

		sysTaskContext c( TASK_INTERFACE(UpdateEnvironmentRope), 0, scratchSize, stacksize);
		c.SetInputOutput();

		c.AddInput(this, allignedSizeOfCloth );

		Assert( pClothController );
		c.AddInput(pClothController, allignedSizeOfController );

		phVerletClothUpdate& u = *c.AllocUserDataAs<phVerletClothUpdate>();
		Vec3V gravitySpu = Scale(phSimulator::GetGravityV(), gravityScale);
		u.m_Gravity = VEC3V_TO_VECTOR3( gravitySpu );
		StoreScalar32FromScalarV(u.m_Gravity.w, timeStep);

#if NO_BOUND_CENTER_RADIUS
		u.boundingCenterAndRadiusAddress = (u32)&m_BBMin;
#else
		u.boundingCenterAndRadiusAddress = (u32)&m_BoundingCenterAndRadius;
#endif
		u.instLastMtxIdxAddrMM = PHLEVEL->GetLastInstanceMatrixIndexMapBaseAddr();
		u.instLastMatricsAddrMM = PHLEVEL->GetLastInstanceMatricesBaseAddr();

#if ENABLE_SPU_COMPARE
		if (gEnableSpuCompare)
			gpCompareBuf = gCompareBuf;
		gSpuCompareFailed = 0;
		u.compareBuf = gpCompareBuf;
#endif

		SetCollisionInst( collisionInst );

		sysTaskHandle h = c.Start();
#if ENABLE_SPU_COMPARE
		sysTaskManager::Wait(h);
		UpdateEnvironmentRope( gravitySpu, timeStep, collisionInst, gravityScale);
		Assert(gSpuCompareFailed < 64);
		gpCompareBuf = 0;
		return 0;
#else
		return h;
#endif
	}
	else
#endif // (__PPU)
	{
		UpdateEnvironmentRope( phSimulator::GetGravityV(), timeStep, collisionInst, gravityScale);
		return 0;
	}
}


#if ENABLE_SPU_DEBUG_DRAW	
void SpuDebugSphere(const void* SPU_ONLY(pData))
{
	if( gpDebugDrawBuf )
	{
#if __SPU
		const int sizeBuf = sizeof(Vec3V);
		const Vec3V* baseAddr = ((Vec3V*)gpDebugDrawBuf + VERLET_BUFFER_TYPE_SIZE * rage::SPU_DEBUG_SPHERE ) + gSPUDebugDrawSpheresCount;
		sysDmaLargePutAndWait((void*)pData, (u32)(baseAddr), sizeBuf, 0);

		++gSPUDebugDrawSpheresCount;
#endif
	}
}

void SpuDebugBox(const void* SPU_ONLY(pV0), const void* SPU_ONLY(pV1))
{
	if( gpDebugDrawBuf )
	{
#if __SPU
		const int sizeBuf = sizeof(Vec3V);
		const Vec3V* baseAddr = ((Vec3V*)(gpDebugDrawBuf + VERLET_BUFFER_TYPE_SIZE * rage::SPU_DEBUG_BOX)) + gSPUDebugDrawBoxesCount*2;
		sysDmaLargePut((void*)pV0, (u32)(baseAddr+0), sizeBuf, 0);
		sysDmaLargePut((void*)pV1, (u32)(baseAddr+1), sizeBuf, 0);

		sysDmaWait(1<<0);

		++gSPUDebugDrawBoxesCount;
#endif
	}
}

void SpuDebugTriangle(const void* SPU_ONLY(pV0), const void* SPU_ONLY(pV1), const void* SPU_ONLY(pV2))
{
	if( gpDebugDrawBuf )
	{
#if __SPU

		const int sizeBuf = sizeof(Vec3V);
		const Vec3V* baseAddr = ((Vec3V*)(gpDebugDrawBuf + VERLET_BUFFER_TYPE_SIZE * rage::SPU_DEBUG_TRIANGLE)) + gSPUDebugDrawTrianglesCount*3;
		sysDmaLargePut((void*)pV0, (u32)(baseAddr+0), sizeBuf, 0);
		sysDmaLargePut((void*)pV1, (u32)(baseAddr+1), sizeBuf, 0);
		sysDmaLargePut((void*)pV2, (u32)(baseAddr+2), sizeBuf, 0);

		sysDmaWait(1<<0);

		++gSPUDebugDrawTrianglesCount;
#endif
	}
}

void SpuDebugCapsule(const void* SPU_ONLY(pCol0), 
	const void* SPU_ONLY(pCol1), 
	const void* SPU_ONLY(pCol2), 
	const void* SPU_ONLY(pCol3), 
	const void* SPU_ONLY(pLen), const void* SPU_ONLY(pRad))
{
	if( gpDebugDrawBuf )
	{
#if __SPU
		const int sizeBuf = sizeof(Vec3V);
		const Vec3V* baseAddr = ((Vec3V*)(gpDebugDrawBuf + VERLET_BUFFER_TYPE_SIZE * rage::SPU_DEBUG_CAPSULE)) + gSPUDebugDrawCapsulesCount*6;			// 4 vectors for the matrix, 1 vector for length, 1 vector for radius
		sysDmaLargePut((void*)pCol0, (u32)(baseAddr+0), sizeBuf, 0);
		sysDmaLargePut((void*)pCol1, (u32)(baseAddr+1), sizeBuf, 0);
		sysDmaLargePut((void*)pCol2, (u32)(baseAddr+2), sizeBuf, 0);
		sysDmaLargePut((void*)pCol3, (u32)(baseAddr+3), sizeBuf, 0);
		sysDmaLargePut((void*)pLen,  (u32)(baseAddr+4), sizeBuf, 0);
		sysDmaLargePut((void*)pRad,  (u32)(baseAddr+5), sizeBuf, 0);

		sysDmaWait(1<<0);

		++gSPUDebugDrawCapsulesCount;
#endif
	}
}


#endif // ENABLE_SPU_DEBUG_DRAW


#if ENABLE_SPU_COMPARE
void SpuCompare(const void* pData, int size, const char* msg, bool)
{
	if (!gpCompareBuf)
		return;
#if __SPU
	sysDmaLargePutAndWait((void*)pData, (u32)gpCompareBuf, size, 0);
#else
	int count = size/4;
	const float* i1 = (const float*)pData;
	const float* i2 = (const float*)gpCompareBuf;	
	for(int i=0; i<count; ++i)
	{
		if (i1[i] == i1[i] && Abs(i1[i] - i2[i]) > 1e-3f)
		{
			Displayf("%s [%i] : %f != %f", msg, i, i1[i], i2[i]);
			++gSpuCompareFailed;
		}
	}
#endif
	gpCompareBuf += (size+15)&~15;
}
#endif


void phVerletCloth::UpdateEnvironmentCloth ( Vec3V_In parentOffset, float timeScale, ScalarV_In timeStep, phInstDatRefArray& collisionInst, Vec3V_In gravityV, ScalarV_In gravityScale)
{
	PF_FUNC(EnvClothUpdate);
	float dampTimeScale = DAMP_TIME_SCALE;
	float damping = DEFAULT_VERLET_DAMPING * Selectf( timeScale - 1.0f, 1.0f, timeScale * dampTimeScale );		// increase the damp value x times if time is scaled
	ScalarV scaledDamping = Add( ScalarVFromF32(-damping), ScalarV(V_ONE) );

	Vec3V gravityDelVel = Scale( Scale(gravityV/*phSimulator::GetGravityV()*/,timeStep), gravityScale );
	Vec3V gravityDisplacement = Scale(gravityDelVel,timeStep);
	gravityDisplacement.SetW( ScalarV(V_ZERO) );

#if __SPU
	if( !gSPU_PauseClothSimulation )
#else
	if( !g_PauseClothSimulation )
#endif
	{

#if NO_PIN_VERTS_IN_VERLET
		int numPinnedVerts = GetClothData().GetNumPinVerts();
#else
		int numPinnedVerts = GetPinCount();
#endif
		UpdateCloth( Mat34V(V_IDENTITY), m_ClothData.GetVertexPointer(), collisionInst, numPinnedVerts, gravityDisplacement, scaledDamping, NULL, parentOffset );
	}
}


void phVerletCloth::UpdateEnvironmentClothAgainstCollisionInsts( Vec3V_In parentOffset, phInstDatRefArray& collisionInst)
{
#if NO_PIN_VERTS_IN_VERLET
	int numPinnedVerts = GetClothData().GetNumPinVerts();
#else
	int numPinnedVerts = GetPinCount();
#endif


	UpdateClothAgainstCollisionInsts( Mat34V(V_IDENTITY), m_ClothData.GetVertexPointer(), collisionInst, numPinnedVerts, NULL, parentOffset );
}


void phVerletCloth::UpdateEnvironmentRope( Vec3V_In gravity, ScalarV_In timeStep, phInstDatRefArray& collisionInst, ScalarV_In gravityScale )
{
	PF_FUNC(EnvRopeUpdate);

	ScalarV scaledDamping = Add( ScalarVFromF32( -DEFAULT_VERLET_DAMPING ), ScalarV(V_ONE) );
	UpdateRope( gravity, timeStep, gravityScale, m_ClothData.GetVertexPointer(), collisionInst, scaledDamping );
}


#if __SPU
#define PARAM_SPU_ONLY(x) x
#else
#define PARAM_SPU_ONLY(x)
#endif

#if !__SPU
inline float Project( const Vector3 &projectionAxis, const Vector3 &projectIn, Vector3 &projectOut )
{
	const float d = projectionAxis.Dot( projectIn );
	projectOut.SetScaled( projectionAxis, d );
	return d;
}
#endif

inline Vec3V_Out ProjectV( Vec3V_In projectionAxis, Vec3V_In projectIn )
{
	ScalarV d = Dot( projectionAxis, projectIn );
	return Scale( projectionAxis, Vec3V(d) );
}


#define CUSTOM_PARAM_SPU_ONLY(x)

inline void phVerletCloth::ApplyForce_WithCompression( Vec3V* RESTRICT vertNormals, int CUSTOM_PARAM_SPU_ONLY(vertNormalStride), const float* RESTRICT vertWeights, Vec3V_In forceV, int nPin, int nVert, const float* RESTRICT inflationScale )
{
	Vec3V* RESTRICT clothVertexPositions = (Vec3V*)m_ClothData.GetVertexPointer();
	Assert( clothVertexPositions );
	Assert( vertNormals );
	Assert( vertWeights );

#if !__SPU
	PrefetchDC( &clothVertexPositions[nPin] );
	PrefetchDC( vertNormals );
	PrefetchDC( vertWeights );
#endif

	Assert( inflationScale );
	Assert( nVert );

	const int numVerts8 = (((nVert-nPin) >> 3 ) << 3) + nPin;
	int i;
	for( i = nPin; i < numVerts8; i += 8 )
	{
#if !__SPU
		PrefetchDC( &clothVertexPositions[i+8] );
		PrefetchDC( &vertNormals[i+8] );
		PrefetchDC( &vertWeights[i+8] );
#endif
		const int	i0 = i,
			i1 = i + 1,
			i2 = i + 2,
			i3 = i + 3,
			i4 = i + 4,
			i5 = i + 5,
			i6 = i + 6,
			i7 = i + 7;

		Vec3V v0( UnpackV1010102( clothVertexPositions[i0].GetIntrin128() ) );
		Vec3V v1( UnpackV1010102( clothVertexPositions[i1].GetIntrin128() ) );
		Vec3V v2( UnpackV1010102( clothVertexPositions[i2].GetIntrin128() ) );
		Vec3V v3( UnpackV1010102( clothVertexPositions[i3].GetIntrin128() ) );
		Vec3V v4( UnpackV1010102( clothVertexPositions[i4].GetIntrin128() ) );
		Vec3V v5( UnpackV1010102( clothVertexPositions[i5].GetIntrin128() ) );
		Vec3V v6( UnpackV1010102( clothVertexPositions[i6].GetIntrin128() ) );
		Vec3V v7( UnpackV1010102( clothVertexPositions[i7].GetIntrin128() ) );

		Vec3V oldV0 = v0;
		Vec3V oldV1 = v1;
		Vec3V oldV2 = v2;
		Vec3V oldV3 = v3;
		Vec3V oldV4 = v4;
		Vec3V oldV5 = v5;
		Vec3V oldV6 = v6;
		Vec3V oldV7 = v7;

		v0 = Subtract( v0, forceV );
		v1 = Subtract( v1, forceV );
		v2 = Subtract( v2, forceV );
		v3 = Subtract( v3, forceV );
		v4 = Subtract( v4, forceV );
		v5 = Subtract( v5, forceV );
		v6 = Subtract( v6, forceV );
		v7 = Subtract( v7, forceV );

		Vec3V n0 = vertNormals[ i0 ];
		Vec3V n1 = vertNormals[ i1 ];
		Vec3V n2 = vertNormals[ i2 ];
		Vec3V n3 = vertNormals[ i3 ];
		Vec3V n4 = vertNormals[ i4 ];
		Vec3V n5 = vertNormals[ i5 ];
		Vec3V n6 = vertNormals[ i6 ];
		Vec3V n7 = vertNormals[ i7 ];

		v0 = Scale( ProjectV(n0, v0), ScalarVFromF32(inflationScale[i0]) );
		v1 = Scale( ProjectV(n1, v1), ScalarVFromF32(inflationScale[i1]) );
		v2 = Scale( ProjectV(n2, v2), ScalarVFromF32(inflationScale[i2]) );
		v3 = Scale( ProjectV(n3, v3), ScalarVFromF32(inflationScale[i3]) );
		v4 = Scale( ProjectV(n4, v4), ScalarVFromF32(inflationScale[i4]) );
		v5 = Scale( ProjectV(n5, v5), ScalarVFromF32(inflationScale[i5]) );
		v6 = Scale( ProjectV(n6, v6), ScalarVFromF32(inflationScale[i6]) );
		v7 = Scale( ProjectV(n7, v7), ScalarVFromF32(inflationScale[i7]) );

		ScalarV vertWeight0 = ScalarVFromF32(vertWeights[i0]);
		ScalarV vertWeight1 = ScalarVFromF32(vertWeights[i1]);
		ScalarV vertWeight2 = ScalarVFromF32(vertWeights[i2]);
		ScalarV vertWeight3 = ScalarVFromF32(vertWeights[i3]);
		ScalarV vertWeight4 = ScalarVFromF32(vertWeights[i4]);
		ScalarV vertWeight5 = ScalarVFromF32(vertWeights[i5]);
		ScalarV vertWeight6 = ScalarVFromF32(vertWeights[i6]);
		ScalarV vertWeight7 = ScalarVFromF32(vertWeights[i7]);

		oldV0 = SubtractScaled( oldV0, v0, vertWeight0 );
		oldV1 = SubtractScaled( oldV1, v1, vertWeight1 );
		oldV2 = SubtractScaled( oldV2, v2, vertWeight2 );
		oldV3 = SubtractScaled( oldV3, v3, vertWeight3 );
		oldV4 = SubtractScaled( oldV4, v4, vertWeight4 );
		oldV5 = SubtractScaled( oldV5, v5, vertWeight5 );
		oldV6 = SubtractScaled( oldV6, v6, vertWeight6 );
		oldV7 = SubtractScaled( oldV7, v7, vertWeight7 );

		clothVertexPositions[i0] = SubtractScaled( clothVertexPositions[i0], v0, vertWeight0 );
		clothVertexPositions[i1] = SubtractScaled( clothVertexPositions[i1], v1, vertWeight1 );
		clothVertexPositions[i2] = SubtractScaled( clothVertexPositions[i2], v2, vertWeight2 );
		clothVertexPositions[i3] = SubtractScaled( clothVertexPositions[i3], v3, vertWeight3 );
		clothVertexPositions[i4] = SubtractScaled( clothVertexPositions[i4], v4, vertWeight4 );
		clothVertexPositions[i5] = SubtractScaled( clothVertexPositions[i5], v5, vertWeight5 );
		clothVertexPositions[i6] = SubtractScaled( clothVertexPositions[i6], v6, vertWeight6 );
		clothVertexPositions[i7] = SubtractScaled( clothVertexPositions[i7], v7, vertWeight7 );

		clothVertexPositions[i0].SetIntrin128( PackV1010102( clothVertexPositions[i0].GetIntrin128Ref(), oldV0.GetIntrin128() ) );
		clothVertexPositions[i1].SetIntrin128( PackV1010102( clothVertexPositions[i1].GetIntrin128Ref(), oldV1.GetIntrin128() ) );
		clothVertexPositions[i2].SetIntrin128( PackV1010102( clothVertexPositions[i2].GetIntrin128Ref(), oldV2.GetIntrin128() ) );
		clothVertexPositions[i3].SetIntrin128( PackV1010102( clothVertexPositions[i3].GetIntrin128Ref(), oldV3.GetIntrin128() ) );
		clothVertexPositions[i4].SetIntrin128( PackV1010102( clothVertexPositions[i4].GetIntrin128Ref(), oldV4.GetIntrin128() ) );
		clothVertexPositions[i5].SetIntrin128( PackV1010102( clothVertexPositions[i5].GetIntrin128Ref(), oldV5.GetIntrin128() ) );
		clothVertexPositions[i6].SetIntrin128( PackV1010102( clothVertexPositions[i6].GetIntrin128Ref(), oldV6.GetIntrin128() ) );
		clothVertexPositions[i7].SetIntrin128( PackV1010102( clothVertexPositions[i7].GetIntrin128Ref(), oldV7.GetIntrin128() ) );		
	}
	// TODO: make sure vert count is aligned to 8 ?
	for( ; i < nVert; ++i )
	{
		Vec3V v( UnpackV1010102( clothVertexPositions[i].GetIntrin128() ) );
		Vec3V oldV = v;
		v = Subtract( v, forceV );

		Vec3V n = vertNormals[ i ];

		v = Scale( ProjectV(n, v), ScalarVFromF32(inflationScale[i]) );

		ScalarV vertWeight = ScalarVFromF32(vertWeights[i]);

		oldV = SubtractScaled( oldV, v, vertWeight );
		clothVertexPositions[i] = SubtractScaled( clothVertexPositions[i], v, vertWeight );
		clothVertexPositions[i].SetIntrin128( PackV1010102( clothVertexPositions[i].GetIntrin128Ref(), oldV.GetIntrin128() ) );
	}
}


inline void phVerletCloth::ApplyForceTransform_WithCompression( const int* RESTRICT matIndices, const Mat34V* RESTRICT matrices, Vec3V* RESTRICT vertNormals, int CUSTOM_PARAM_SPU_ONLY(vertNormalStride), const float* RESTRICT vertWeights, Vec3V_In forceV, int nPin, int nVert, const float* RESTRICT inflationScale )
{
	if( IsLessThanAll(Dot(forceV,forceV), ScalarV(V_FLT_SMALL_6)) != 0 )
		return;

	Assert( matIndices );
	Assert( matrices );

	Vec3V* RESTRICT clothVertexPositions = (Vec3V*)m_ClothData.GetVertexPointer();

#if !__SPU
	PrefetchDC( &clothVertexPositions[nPin] );
	PrefetchDC( vertNormals );
	PrefetchDC( vertWeights );
#endif

	Assert( inflationScale );
	Assert( nVert );

	const int numVerts8 = (((nVert-nPin) >> 3 ) << 3) + nPin;
	int i;
	for( i = nPin; i < numVerts8; i += 8 )
	{
#if !__SPU
		PrefetchDC( &clothVertexPositions[i+8] );
		PrefetchDC( &vertNormals[i+8] );
		PrefetchDC( &vertWeights[i+8] );
#endif
		const int	i0 = i,
			i1 = i + 1,
			i2 = i + 2,
			i3 = i + 3,
			i4 = i + 4,
			i5 = i + 5,
			i6 = i + 6,
			i7 = i + 7;

		Vec3V v0( UnpackV1010102( clothVertexPositions[i0].GetIntrin128() ) );
		Vec3V v1( UnpackV1010102( clothVertexPositions[i1].GetIntrin128() ) );
		Vec3V v2( UnpackV1010102( clothVertexPositions[i2].GetIntrin128() ) );
		Vec3V v3( UnpackV1010102( clothVertexPositions[i3].GetIntrin128() ) );
		Vec3V v4( UnpackV1010102( clothVertexPositions[i4].GetIntrin128() ) );
		Vec3V v5( UnpackV1010102( clothVertexPositions[i5].GetIntrin128() ) );
		Vec3V v6( UnpackV1010102( clothVertexPositions[i6].GetIntrin128() ) );
		Vec3V v7( UnpackV1010102( clothVertexPositions[i7].GetIntrin128() ) );

		Vec3V oldV0 = v0;
		Vec3V oldV1 = v1;
		Vec3V oldV2 = v2;
		Vec3V oldV3 = v3;
		Vec3V oldV4 = v4;
		Vec3V oldV5 = v5;
		Vec3V oldV6 = v6;
		Vec3V oldV7 = v7;

		Vec3V forceV0 = Transform(matrices[matIndices[i0]], forceV);
		Vec3V forceV1 = Transform(matrices[matIndices[i1]], forceV);
		Vec3V forceV2 = Transform(matrices[matIndices[i2]], forceV);
		Vec3V forceV3 = Transform(matrices[matIndices[i3]], forceV);
		Vec3V forceV4 = Transform(matrices[matIndices[i4]], forceV);
		Vec3V forceV5 = Transform(matrices[matIndices[i5]], forceV);
		Vec3V forceV6 = Transform(matrices[matIndices[i6]], forceV);
		Vec3V forceV7 = Transform(matrices[matIndices[i7]], forceV);

		v0 = Subtract( v0, forceV0 );
		v1 = Subtract( v1, forceV1 );
		v2 = Subtract( v2, forceV2 );
		v3 = Subtract( v3, forceV3 );
		v4 = Subtract( v4, forceV4 );
		v5 = Subtract( v5, forceV5 );
		v6 = Subtract( v6, forceV6 );
		v7 = Subtract( v7, forceV7 );

		Vec3V n0 = vertNormals[ i0 ];
		Vec3V n1 = vertNormals[ i1 ];
		Vec3V n2 = vertNormals[ i2 ];
		Vec3V n3 = vertNormals[ i3 ];
		Vec3V n4 = vertNormals[ i4 ];
		Vec3V n5 = vertNormals[ i5 ];
		Vec3V n6 = vertNormals[ i6 ];
		Vec3V n7 = vertNormals[ i7 ];

		v0 = Scale( ProjectV(n0, v0), ScalarVFromF32(inflationScale[i0]) );
		v1 = Scale( ProjectV(n1, v1), ScalarVFromF32(inflationScale[i1]) );
		v2 = Scale( ProjectV(n2, v2), ScalarVFromF32(inflationScale[i2]) );
		v3 = Scale( ProjectV(n3, v3), ScalarVFromF32(inflationScale[i3]) );
		v4 = Scale( ProjectV(n4, v4), ScalarVFromF32(inflationScale[i4]) );
		v5 = Scale( ProjectV(n5, v5), ScalarVFromF32(inflationScale[i5]) );
		v6 = Scale( ProjectV(n6, v6), ScalarVFromF32(inflationScale[i6]) );
		v7 = Scale( ProjectV(n7, v7), ScalarVFromF32(inflationScale[i7]) );

		ScalarV vertWeight0 = ScalarVFromF32(vertWeights[i0]);
		ScalarV vertWeight1 = ScalarVFromF32(vertWeights[i1]);
		ScalarV vertWeight2 = ScalarVFromF32(vertWeights[i2]);
		ScalarV vertWeight3 = ScalarVFromF32(vertWeights[i3]);
		ScalarV vertWeight4 = ScalarVFromF32(vertWeights[i4]);
		ScalarV vertWeight5 = ScalarVFromF32(vertWeights[i5]);
		ScalarV vertWeight6 = ScalarVFromF32(vertWeights[i6]);
		ScalarV vertWeight7 = ScalarVFromF32(vertWeights[i7]);

		oldV0 = SubtractScaled( oldV0, v0, vertWeight0 );
		oldV1 = SubtractScaled( oldV1, v1, vertWeight1 );
		oldV2 = SubtractScaled( oldV2, v2, vertWeight2 );
		oldV3 = SubtractScaled( oldV3, v3, vertWeight3 );
		oldV4 = SubtractScaled( oldV4, v4, vertWeight4 );
		oldV5 = SubtractScaled( oldV5, v5, vertWeight5 );
		oldV6 = SubtractScaled( oldV6, v6, vertWeight6 );
		oldV7 = SubtractScaled( oldV7, v7, vertWeight7 );

		clothVertexPositions[i0] = SubtractScaled( clothVertexPositions[i0], v0, vertWeight0 );
		clothVertexPositions[i1] = SubtractScaled( clothVertexPositions[i1], v1, vertWeight1 );
		clothVertexPositions[i2] = SubtractScaled( clothVertexPositions[i2], v2, vertWeight2 );
		clothVertexPositions[i3] = SubtractScaled( clothVertexPositions[i3], v3, vertWeight3 );
		clothVertexPositions[i4] = SubtractScaled( clothVertexPositions[i4], v4, vertWeight4 );
		clothVertexPositions[i5] = SubtractScaled( clothVertexPositions[i5], v5, vertWeight5 );
		clothVertexPositions[i6] = SubtractScaled( clothVertexPositions[i6], v6, vertWeight6 );
		clothVertexPositions[i7] = SubtractScaled( clothVertexPositions[i7], v7, vertWeight7 );

		clothVertexPositions[i0].SetIntrin128( PackV1010102( clothVertexPositions[i0].GetIntrin128Ref(), oldV0.GetIntrin128() ) );
		clothVertexPositions[i1].SetIntrin128( PackV1010102( clothVertexPositions[i1].GetIntrin128Ref(), oldV1.GetIntrin128() ) );
		clothVertexPositions[i2].SetIntrin128( PackV1010102( clothVertexPositions[i2].GetIntrin128Ref(), oldV2.GetIntrin128() ) );
		clothVertexPositions[i3].SetIntrin128( PackV1010102( clothVertexPositions[i3].GetIntrin128Ref(), oldV3.GetIntrin128() ) );
		clothVertexPositions[i4].SetIntrin128( PackV1010102( clothVertexPositions[i4].GetIntrin128Ref(), oldV4.GetIntrin128() ) );
		clothVertexPositions[i5].SetIntrin128( PackV1010102( clothVertexPositions[i5].GetIntrin128Ref(), oldV5.GetIntrin128() ) );
		clothVertexPositions[i6].SetIntrin128( PackV1010102( clothVertexPositions[i6].GetIntrin128Ref(), oldV6.GetIntrin128() ) );
		clothVertexPositions[i7].SetIntrin128( PackV1010102( clothVertexPositions[i7].GetIntrin128Ref(), oldV7.GetIntrin128() ) );
	}

	// TODO: make sure vert count is alligned to 8 ?
	for( ; i < nVert; ++i )
	{
		Vec3V v( UnpackV1010102( clothVertexPositions[i].GetIntrin128() ) );
		Vec3V oldV = v;

		Vec3V forceVt = Transform(matrices[matIndices[i]],forceV);
		v = Subtract( v, forceVt );

		Vec3V n = vertNormals[ i ];

		v = Scale( ProjectV(n, v), ScalarVFromF32(inflationScale[i]) );
		ScalarV vertWeight = ScalarVFromF32(vertWeights[i]);

		oldV = SubtractScaled( oldV, v, vertWeight );
		clothVertexPositions[i] = SubtractScaled( clothVertexPositions[i], v, vertWeight );
		clothVertexPositions[i].SetIntrin128( PackV1010102( clothVertexPositions[i].GetIntrin128Ref(), oldV.GetIntrin128() ) );
	}
}



inline void phVerletCloth::ApplyForce_NoCompression( Vec3V_In v_dragCoef, const Vector3& force, int nVert )
{
	// ONLY ROPE IS USING THIS and rope doesn't use compressed deltas for now

	Vec3V f = VECTOR3_TO_VEC3V(force);
	if( IsLessThanAll(Dot(f,f), ScalarV(V_FLT_SMALL_6)) != 0 )
		return;

	Vector3* RESTRICT clothVertexPositions = const_cast<Vector3*>( (Vector3*)m_ClothData.GetVertexPointer());
	Vector3* RESTRICT clothVertexPrevPositions = (Vector3*)m_ClothData.GetVertexPrevPointer();

	Vec3V normal = NormalizeFast( f );
	PrefetchDC( &clothVertexPositions[0] );

	for( int i = 0; i < nVert; ++i )
	{
		if( !IsDynamicPinned(i) )
		{
			Vec3V v = RCC_VEC3V(clothVertexPositions[i]) - RCC_VEC3V(clothVertexPrevPositions[i]);

			v -= RCC_VEC3V(force);
			v = ProjectV( normal, v);		
			v = Scale( v, v_dragCoef );

			RC_VEC3V(clothVertexPositions[i]) -= v;
		}
	}
}

void phVerletCloth::ApplyAirResistance( const float* RESTRICT inflationScale, const float* RESTRICT vertWeights, Vec3V* RESTRICT vertNormals, int vertNormalStride, const Vector3& windVector )
{
#if __SPU
	if( gSPU_PauseClothSimulation )
		return;
#else
	if( g_PauseClothSimulation )
		return;
#endif

	PF_FUNC( ApplyAirResistanceWithCompression );
#if NO_PIN_VERTS_IN_VERLET
	int numPinnedVerts = GetClothData().GetNumPinVerts();
#else
	int numPinnedVerts = GetPinCount();
#endif
	ApplyForce_WithCompression( vertNormals, vertNormalStride, vertWeights, VECTOR3_TO_VEC3V(windVector), numPinnedVerts, GetNumVertices(), inflationScale );
}


void phVerletCloth::ApplyAirResistanceTransform( const int* RESTRICT matIndices, const Mat34V* RESTRICT tMat, const float* RESTRICT inflationScale, const float* RESTRICT vertWeights, Vec3V* RESTRICT vertNormals, int vertNormalStride, const Vector3& windVector )
{
#if __SPU
	if( gSPU_PauseClothSimulation )
		return;
#else
	if( g_PauseClothSimulation )
		return;
#endif

	PF_FUNC( ApplyAirResistanceWithCompression );
#if NO_PIN_VERTS_IN_VERLET
	int numPinnedVerts = GetClothData().GetNumPinVerts();
#else
	int numPinnedVerts = GetPinCount();
#endif
	ApplyForceTransform_WithCompression( matIndices, tMat, vertNormals, vertNormalStride, vertWeights, VECTOR3_TO_VEC3V(windVector), numPinnedVerts, GetNumVertices(), inflationScale );
}


void phVerletCloth::ApplyAirResistanceRope( float dragCoef, const Vector3& windVector)
{
	PF_FUNC( ApplyAirResistanceNoCompression );

	int nVert = GetNumVertices();
	Assert( nVert );
	Vec3V v_dragCoef = Vec3VFromF32(dragCoef);
	ApplyForce_NoCompression( v_dragCoef, windVector, nVert );
}

#if __ASSERT
bool phVerletCloth::TestVertexToBox( Vec3V_In vtx, Vec3V_In boxMax, Vec3V_In boxMin, ScalarV_In threshold )
{
	const Vec3V clampedPos = Clamp(vtx, boxMin, boxMax);
	return (IsLessThanOrEqualAll(DistSquared(vtx, clampedPos), Scale(threshold, threshold)) != 0)? true: false;
}
#endif

void phVerletCloth::UpdateRope( Vec3V_In gravity, ScalarV_In timeStep, ScalarV_In gravityScale, Vec3V* RESTRICT clothVertexPositions, phInstDatRefArray& collisionInst, ScalarV_In scaledDamping )
{
	Vec3V* RESTRICT clothPrevVertexPositions = m_ClothData.GetVertexPrevPointer();

	Assert( clothVertexPositions );
	Assert( clothPrevVertexPositions );

	PrefetchDC( clothVertexPositions );
	PrefetchDC( clothPrevVertexPositions );

#if (__SPU || ENABLE_SPU_COMPARE)			// spu precision is bad when far from the origin
	const int firstVertex = GetNumLockedEdgesFront();
	const int lastVertex = GetNumVertices() - GetNumLockedEdgesBack();
	Vec3V org = clothVertexPositions[firstVertex];
	org.SetW(ScalarV(V_ZERO));
	for( int i = firstVertex; i < lastVertex; ++i )
	{
		clothVertexPositions[i] = Subtract( clothVertexPositions[i], org);
		clothPrevVertexPositions[i] = Subtract( clothPrevVertexPositions[i], org);
	}
#else
	Vec3V org(V_ZERO);
#endif

	IntegrationRope( gravity, timeStep, gravityScale, clothVertexPositions, clothPrevVertexPositions, scaledDamping );

	IterationRope( &(m_EdgeData[0]), clothVertexPositions, clothPrevVertexPositions, collisionInst, org );


#if (__SPU || ENABLE_SPU_COMPARE)
	for( int i = firstVertex; i < lastVertex; ++i )
	{
		clothVertexPositions[i] = Add( clothVertexPositions[i], org );
		clothPrevVertexPositions[i] = Add( clothPrevVertexPositions[i], org);
	}
#endif

	// TODO: add concicentcy check
#if 0 && __ASSERT
	ScalarV threshold(V_TWO);
	for( int i = firstVertex; i < lastVertex; ++i )
	{
		Assert( TestVertexToBox(clothVertexPositions[i], m_BBMax, m_BBMin, threshold) );
	}
#endif

}


#define		CONST_INT_I0_I7(i)	const int	i0 = i,		\
											i1 = i + 1,	\
											i2 = i + 2,	\
											i3 = i + 3,	\
											i4 = i + 4,	\
											i5 = i + 5,	\
											i6 = i + 6,	\
											i7 = i + 7


#define		MAX_VERTS		(1024+256)


inline void phVerletCloth::UpdateCloth( Mat34V_In attachedFrame, Vec3V* RESTRICT clothVertexPositions, phInstDatRefArray& collisionInst, int numPinnedVerts, Vec3V_In gravityDisplacement, ScalarV_In scaledDamping, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset )
{
	Vec3V org(V_ZERO);
	const int numVerts = GetNumVertices();
#if (__SPU || ENABLE_SPU_COMPARE)			// spu precision is bad when far from the origin
	if( ENVIRONMENT_CLOTH )
	{
		org = clothVertexPositions[0];			
		org.SetW(ScalarV(V_ZERO));
		const int numVerts8 = (numVerts >> 3) << 3;
		int i;
		for(i=0; i < numVerts8; i+=8 )
		{
			CONST_INT_I0_I7(i);

			clothVertexPositions[i0] = Subtract( clothVertexPositions[i0], org );
			clothVertexPositions[i1] = Subtract( clothVertexPositions[i1], org );
			clothVertexPositions[i2] = Subtract( clothVertexPositions[i2], org );
			clothVertexPositions[i3] = Subtract( clothVertexPositions[i3], org );
			clothVertexPositions[i4] = Subtract( clothVertexPositions[i4], org );
			clothVertexPositions[i5] = Subtract( clothVertexPositions[i5], org );
			clothVertexPositions[i6] = Subtract( clothVertexPositions[i6], org );
			clothVertexPositions[i7] = Subtract( clothVertexPositions[i7], org );
		}
		for( ; i < numVerts; ++i )
		{
			clothVertexPositions[i] = Subtract( clothVertexPositions[i], org );
		}
	}
#endif

	Assert(numVerts<MAX_VERTS);
#if __SPU
	sysScratchScope s;
	Vec3V* prevVertexPositions = sysScratchAllocObj<Vec3V>(numVerts);
	s.Report("UpdateCloth: prevVertexPositions");
#else
	Vec3V* prevVertexPositions = Alloca( Vec3V, numVerts );
#endif

	IntegrationCloth_WithCompression( clothVertexPositions, prevVertexPositions, numPinnedVerts, gravityDisplacement, scaledDamping );

	IterationCloth( attachedFrame, clothVertexPositions, collisionInst, org, customMatrices, parentOffset );

	CalculateDeltaPositions_WithCompression( clothVertexPositions, prevVertexPositions, numVerts );

#if __PFDRAW && !__SPU
	if( GetFlag(phVerletCloth::FLAG_ENABLE_DEBUG_DRAW) )
	{
		if( PFD_ClothDeltas.Begin() )
		{
			float scaleFF = 5.0f;
			ScalarV scaleV = ScalarVFromF32( scaleFF );

			for(int i = numPinnedVerts; i < numVerts; i++ )
			{
				Vec3V posNew = Add( clothVertexPositions[i], parentOffset );
				Vec3V posOld = Add( prevVertexPositions[i], parentOffset );

				Vector3 pos0 = VEC3V_TO_VECTOR3(posNew);
				Vector3 pos1 = VEC3V_TO_VECTOR3( AddScaled(posNew, Subtract(posNew, posOld), scaleV) );
				grcDrawLine( pos0, pos1, Color_green );
			}
			PFD_ClothDeltas.End();
		}
	}
#endif

#if (__SPU || ENABLE_SPU_COMPARE)
	if( ENVIRONMENT_CLOTH )
	{
		const int numVerts8 = (numVerts >> 3) << 3;
		int i;
		for(i=0; i < numVerts8; i+=8 )
		{
			CONST_INT_I0_I7(i);

			clothVertexPositions[i0] = Add( clothVertexPositions[i0], org );
			clothVertexPositions[i1] = Add( clothVertexPositions[i1], org );
			clothVertexPositions[i2] = Add( clothVertexPositions[i2], org );
			clothVertexPositions[i3] = Add( clothVertexPositions[i3], org );
			clothVertexPositions[i4] = Add( clothVertexPositions[i4], org );
			clothVertexPositions[i5] = Add( clothVertexPositions[i5], org );
			clothVertexPositions[i6] = Add( clothVertexPositions[i6], org );
			clothVertexPositions[i7] = Add( clothVertexPositions[i7], org );
		}
		for(; i<numVerts; ++i)
			clothVertexPositions[i] = Add( clothVertexPositions[i], org );
	}
#endif

}


inline void phVerletCloth::UpdateClothAgainstCollisionInsts( Mat34V_In attachedFrame, Vec3V* RESTRICT clothVertexPositions, phInstDatRefArray& collisionInst, int numPinnedVerts, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset )
{
	Vec3V org(V_ZERO);
	IterationCloth( attachedFrame, clothVertexPositions, collisionInst, org, customMatrices, parentOffset );
}


inline void phVerletCloth::CalculateDeltaPositions_WithCompression(Vec3V* RESTRICT newVertexPositions, Vec3V* RESTRICT prevVertexPositions, const int numVerts)
{
	PF_FUNC( ClothCalculateDeltaPositions );

	const int numVerts8 = ((numVerts >> 3) << 3);

	PrefetchDC( &newVertexPositions[0] );
	PrefetchDC( &prevVertexPositions[0] );

	int i;
	for(i = 0; i < numVerts8; i += 8 )
	{
		PrefetchDC( &newVertexPositions[i+8] );
		PrefetchDC( &prevVertexPositions[i+8] );

		CONST_INT_I0_I7(i);

		const Vec3V disp0 = Subtract( newVertexPositions[i0], prevVertexPositions[i0] );
		const Vec3V disp1 = Subtract( newVertexPositions[i1], prevVertexPositions[i1] );
		const Vec3V disp2 = Subtract( newVertexPositions[i2], prevVertexPositions[i2] );
		const Vec3V disp3 = Subtract( newVertexPositions[i3], prevVertexPositions[i3] );
		const Vec3V disp4 = Subtract( newVertexPositions[i4], prevVertexPositions[i4] );
		const Vec3V disp5 = Subtract( newVertexPositions[i5], prevVertexPositions[i5] );
		const Vec3V disp6 = Subtract( newVertexPositions[i6], prevVertexPositions[i6] );
		const Vec3V disp7 = Subtract( newVertexPositions[i7], prevVertexPositions[i7] );

		newVertexPositions[i0].SetIntrin128( PackV1010102( newVertexPositions[i0].GetIntrin128Ref(), disp0.GetIntrin128() ) );
		newVertexPositions[i1].SetIntrin128( PackV1010102( newVertexPositions[i1].GetIntrin128Ref(), disp1.GetIntrin128() ) );
		newVertexPositions[i2].SetIntrin128( PackV1010102( newVertexPositions[i2].GetIntrin128Ref(), disp2.GetIntrin128() ) );
		newVertexPositions[i3].SetIntrin128( PackV1010102( newVertexPositions[i3].GetIntrin128Ref(), disp3.GetIntrin128() ) );
		newVertexPositions[i4].SetIntrin128( PackV1010102( newVertexPositions[i4].GetIntrin128Ref(), disp4.GetIntrin128() ) );
		newVertexPositions[i5].SetIntrin128( PackV1010102( newVertexPositions[i5].GetIntrin128Ref(), disp5.GetIntrin128() ) );
		newVertexPositions[i6].SetIntrin128( PackV1010102( newVertexPositions[i6].GetIntrin128Ref(), disp6.GetIntrin128() ) );
		newVertexPositions[i7].SetIntrin128( PackV1010102( newVertexPositions[i7].GetIntrin128Ref(), disp7.GetIntrin128() ) );
	}

	for( ; i < numVerts; ++i )
	{
		const Vec3V disp = Subtract( newVertexPositions[i], prevVertexPositions[i] );
		newVertexPositions[i].SetIntrin128( PackV1010102( newVertexPositions[i].GetIntrin128Ref(), disp.GetIntrin128() ) );
	}
}


void phVerletCloth::ZeroDeltaPositions_WithCompression()
{
	Vec3V* RESTRICT newVertexPositions = GetClothData().GetVertexPointer();
	const int numVerts = GetClothData().GetNumVerts();
	const int numVerts8 = ((numVerts >> 3) << 3);
	PrefetchDC( &newVertexPositions[0] );

	int i;
	const Vec3V zero(V_ZERO);

	for(i = 0; i < numVerts8; i += 8 )
	{
		PrefetchDC( &newVertexPositions[i+8] );

		CONST_INT_I0_I7(i);

		newVertexPositions[i0].SetIntrin128( PackV1010102( newVertexPositions[i0].GetIntrin128Ref(), zero.GetIntrin128() ) );
		newVertexPositions[i1].SetIntrin128( PackV1010102( newVertexPositions[i1].GetIntrin128Ref(), zero.GetIntrin128() ) );
		newVertexPositions[i2].SetIntrin128( PackV1010102( newVertexPositions[i2].GetIntrin128Ref(), zero.GetIntrin128() ) );
		newVertexPositions[i3].SetIntrin128( PackV1010102( newVertexPositions[i3].GetIntrin128Ref(), zero.GetIntrin128() ) );
		newVertexPositions[i4].SetIntrin128( PackV1010102( newVertexPositions[i4].GetIntrin128Ref(), zero.GetIntrin128() ) );
		newVertexPositions[i5].SetIntrin128( PackV1010102( newVertexPositions[i5].GetIntrin128Ref(), zero.GetIntrin128() ) );
		newVertexPositions[i6].SetIntrin128( PackV1010102( newVertexPositions[i6].GetIntrin128Ref(), zero.GetIntrin128() ) );
		newVertexPositions[i7].SetIntrin128( PackV1010102( newVertexPositions[i7].GetIntrin128Ref(), zero.GetIntrin128() ) );
	}

	for( ; i < numVerts; ++i )
	{
		newVertexPositions[i].SetIntrin128( PackV1010102( newVertexPositions[i].GetIntrin128Ref(), zero.GetIntrin128() ) );
	}
}

#if __SPU || ENABLE_SPU_COMPARE
#define ONLY_ON_SPU(x) x
#else
#define ONLY_ON_SPU(x)
#endif


inline void phVerletCloth::IterationRope( const phEdgeData* RESTRICT pEdgeDataPtr, Vec3V* RESTRICT clothVertexPositions, Vec3V* RESTRICT SPU_ONLY(clothVertexPrevPositions), phInstDatRefArray& collisionInst, Vec3V_In ONLY_ON_SPU(org) )
{	
	// TODO: fix it
	//	m_TestedForCulledPrimitves = false;
	//	m_CulledTrianglesCount = 0;
	// #if __SPU
	// 	m_CulledTriangles = sysScratchAllocObj<TriangleTransformed>(MAX_NUM_CULLED_POLYGIONS);
	// 	m_TriangleEdgeMap = sysScratchAllocObj<EdgeTriangleMap>(m_NumEdges);
	// #else
	// 	m_CulledTriangles = Alloca(TriangleTransformed,MAX_NUM_CULLED_POLYGIONS);
	// 	m_TriangleEdgeMap = Alloca(EdgeTriangleMap,m_NumEdges);
	// 
	// 	PrefetchDC(m_CulledTriangles);
	// 	PrefetchDC(m_TriangleEdgeMap);
	// #endif

	for( int iterationCount = 0 ; iterationCount < m_nIterations; ++iterationCount )
	{
		UpdateEdgesVec3V( (__vector4*)clothVertexPositions, pEdgeDataPtr );

#if (__SPU || ENABLE_SPU_COMPARE)
		const int firstVertex = GetNumLockedEdgesFront();
		const int lastVertex = GetNumVertices() - GetNumLockedEdgesBack();
		for(int i=firstVertex; i < lastVertex; ++i)
		{
			clothVertexPositions[i] = Add( clothVertexPositions[i], org );
			clothVertexPrevPositions[i] = Add( clothVertexPrevPositions[i], org );
		}
#endif

		DetectAndEnforceList( Mat34V(V_IDENTITY), collisionInst, NULL, Vec3V(V_ZERO) );

		// TODO: fix it
		//		m_TestedForCulledPrimitves = true;
#if (__SPU || ENABLE_SPU_COMPARE)
		for(int i=firstVertex; i < lastVertex; ++i)
		{
			clothVertexPositions[i] = Subtract( clothVertexPositions[i], org);
			clothVertexPrevPositions[i] = Subtract( clothVertexPrevPositions[i], org);
		}
#endif
	}	
}


inline void phVerletCloth::IterationCloth( Mat34V_In attachedFrame, Vec3V* RESTRICT clothVertexPositions, phInstDatRefArray& collisionInst, Vec3V_In ONLY_ON_SPU(org), const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset )
{
	PF_FUNC( ClothIteration );

	const phEdgeData* pEdgeDataPtr = &(GetEdge(0));
	Assert( pEdgeDataPtr );	

	const int numCustomEdgesType = GetCustomEdgeList().GetCount();
	const int numCustomEdgesInst = GetInstanceCustomEdgeList().GetCount();	

	for( int iterationCount = 0; iterationCount < m_nIterations; ++iterationCount )
	{
		// TODO: custom edges should be merged into regular edges ( once cloth modeling pipeline is ready )
		if( 
#if __BANK
			m_Pad0[0] &&
#endif
			(numCustomEdgesInst > 0 || numCustomEdgesType > 0 )
			)
		{
			// NOTE: give precedence to instance's custom edges
			// TODO: test if is possible to update custom edges every other frame ( for speed )
			const atArray<phEdgeData>& customEdges = (numCustomEdgesInst > 0 ? GetInstanceCustomEdgeList(): GetCustomEdgeList() ); 
			const int numCustomEdges = (numCustomEdgesInst > 0 ? numCustomEdgesInst: numCustomEdgesType);
			UpdateEdgesSoAVec3VBy8s( numCustomEdges,(__vector4*)clothVertexPositions, &customEdges[0], DEFAULT_VERLET_OVERRELAXATION_CONSTANT );
		}

		UpdateEdgesSoAVec3VBy8s( m_NumEdges, (__vector4*)clothVertexPositions, pEdgeDataPtr, DEFAULT_VERLET_OVERRELAXATION_CONSTANT );

		{

#if (__SPU || ENABLE_SPU_COMPARE)
			const int numVerts = GetNumVertices();
			const phBound* pCustomBound = GetCustomBound();
			if( ENVIRONMENT_CLOTH && (m_VirtualBound || pCustomBound || m_PedBound0 || m_PedBound1 ) )
			{
				const int numVerts8 = (numVerts >> 3) << 3;
				int i;
				for(i=0; i < numVerts8; i+=8 )
				{
					CONST_INT_I0_I7(i);

					clothVertexPositions[i0] = Add( clothVertexPositions[i0], org );
					clothVertexPositions[i1] = Add( clothVertexPositions[i1], org );
					clothVertexPositions[i2] = Add( clothVertexPositions[i2], org );
					clothVertexPositions[i3] = Add( clothVertexPositions[i3], org );
					clothVertexPositions[i4] = Add( clothVertexPositions[i4], org );
					clothVertexPositions[i5] = Add( clothVertexPositions[i5], org );
					clothVertexPositions[i6] = Add( clothVertexPositions[i6], org );
					clothVertexPositions[i7] = Add( clothVertexPositions[i7], org );
				}
				for( ; i<numVerts; ++i )
					clothVertexPositions[i] = Add( clothVertexPositions[i], org );
			}
#endif

			DetectAndEnforceList( attachedFrame, collisionInst, customMatrices, parentOffset );

#if (__SPU || ENABLE_SPU_COMPARE)
			if( ENVIRONMENT_CLOTH && (m_VirtualBound || pCustomBound || m_PedBound0 || m_PedBound1) )
			{
				const int numVerts8 = (numVerts >> 3) << 3;
				int i;
				for(i=0; i < numVerts8; i+=8 )
				{
					CONST_INT_I0_I7(i);

					clothVertexPositions[i0] = Subtract( clothVertexPositions[i0], org );
					clothVertexPositions[i1] = Subtract( clothVertexPositions[i1], org );
					clothVertexPositions[i2] = Subtract( clothVertexPositions[i2], org );
					clothVertexPositions[i3] = Subtract( clothVertexPositions[i3], org );
					clothVertexPositions[i4] = Subtract( clothVertexPositions[i4], org );
					clothVertexPositions[i5] = Subtract( clothVertexPositions[i5], org );
					clothVertexPositions[i6] = Subtract( clothVertexPositions[i6], org );
					clothVertexPositions[i7] = Subtract( clothVertexPositions[i7], org );
				}
				for( ; i<numVerts; ++i )
					clothVertexPositions[i] = Subtract( clothVertexPositions[i], org );
			}
#endif

		}
	}
}


inline void phVerletCloth::IntegrationCloth_WithCompression( Vec3V* RESTRICT clothVertexPositions, Vec3V* RESTRICT clothPrevVertexPositions,
	const int numPinnedVerts, Vec3V_In gravityDisplacement, ScalarV_In scaledDamping	)
{
	PF_FUNC( ClothIntegration );

	PrefetchDC( clothPrevVertexPositions );
	PrefetchDC( clothVertexPositions );

	int i;
	for (i=0; i<numPinnedVerts; i++)
	{
		clothPrevVertexPositions[i] = clothVertexPositions[i];
	}

	const int numVerts = GetNumVertices();
	const int numVerts8 = (((numVerts-numPinnedVerts) >> 3) << 3) + numPinnedVerts;
	for ( i = numPinnedVerts; i < numVerts8 ; i += 8 )
	{
		PrefetchDC( &clothPrevVertexPositions[i + 8] );
		PrefetchDC( &clothVertexPositions[i + 8] );

		CONST_INT_I0_I7(i);

		const Vec3V vertexPos0 = clothVertexPositions[i0];
		const Vec3V vertexPos1 = clothVertexPositions[i1];
		const Vec3V vertexPos2 = clothVertexPositions[i2];
		const Vec3V vertexPos3 = clothVertexPositions[i3];
		const Vec3V vertexPos4 = clothVertexPositions[i4];
		const Vec3V vertexPos5 = clothVertexPositions[i5];
		const Vec3V vertexPos6 = clothVertexPositions[i6];
		const Vec3V vertexPos7 = clothVertexPositions[i7];

		clothPrevVertexPositions[i0] = vertexPos0;
		clothPrevVertexPositions[i1] = vertexPos1;
		clothPrevVertexPositions[i2] = vertexPos2;
		clothPrevVertexPositions[i3] = vertexPos3;
		clothPrevVertexPositions[i4] = vertexPos4;
		clothPrevVertexPositions[i5] = vertexPos5;
		clothPrevVertexPositions[i6] = vertexPos6;
		clothPrevVertexPositions[i7] = vertexPos7;

		const Vec3V prevDisp0( UnpackV1010102( vertexPos0.GetIntrin128() ) );
		const Vec3V prevDisp1( UnpackV1010102( vertexPos1.GetIntrin128() ) );
		const Vec3V prevDisp2( UnpackV1010102( vertexPos2.GetIntrin128() ) );
		const Vec3V prevDisp3( UnpackV1010102( vertexPos3.GetIntrin128() ) );
		const Vec3V prevDisp4( UnpackV1010102( vertexPos4.GetIntrin128() ) );
		const Vec3V prevDisp5( UnpackV1010102( vertexPos5.GetIntrin128() ) );
		const Vec3V prevDisp6( UnpackV1010102( vertexPos6.GetIntrin128() ) );
		const Vec3V prevDisp7( UnpackV1010102( vertexPos7.GetIntrin128() ) );

		const Vec3V disp0 = AddScaled( gravityDisplacement, prevDisp0, scaledDamping );
		const Vec3V disp1 = AddScaled( gravityDisplacement, prevDisp1, scaledDamping );
		const Vec3V disp2 = AddScaled( gravityDisplacement, prevDisp2, scaledDamping );
		const Vec3V disp3 = AddScaled( gravityDisplacement, prevDisp3, scaledDamping );
		const Vec3V disp4 = AddScaled( gravityDisplacement, prevDisp4, scaledDamping );
		const Vec3V disp5 = AddScaled( gravityDisplacement, prevDisp5, scaledDamping );
		const Vec3V disp6 = AddScaled( gravityDisplacement, prevDisp6, scaledDamping );
		const Vec3V disp7 = AddScaled( gravityDisplacement, prevDisp7, scaledDamping );

		clothVertexPositions[i0] = Add( vertexPos0, disp0 );
		clothVertexPositions[i1] = Add( vertexPos1, disp1 );
		clothVertexPositions[i2] = Add( vertexPos2, disp2 );
		clothVertexPositions[i3] = Add( vertexPos3, disp3 );
		clothVertexPositions[i4] = Add( vertexPos4, disp4 );
		clothVertexPositions[i5] = Add( vertexPos5, disp5 );
		clothVertexPositions[i6] = Add( vertexPos6, disp6 );
		clothVertexPositions[i7] = Add( vertexPos7, disp7 );
	}

	PrefetchDC( &clothPrevVertexPositions[i] );
	PrefetchDC( &clothVertexPositions[i] );

	// Update the rest
	for ( ; i < numVerts ; ++i )
	{	
		const Vec3V vertexPos = clothVertexPositions[i];

		clothPrevVertexPositions[i] = vertexPos;		
		const Vec3V prevDisp( UnpackV1010102( vertexPos.GetIntrin128() ) );
		const Vec3V disp = AddScaled( gravityDisplacement, prevDisp, scaledDamping );		

		clothVertexPositions[i] = Add( vertexPos, disp );
	}
}


inline void phVerletCloth::IntegrationRope( Vec3V_In gravity, ScalarV_In timeStep, ScalarV_In gravityScale, 
	Vec3V* RESTRICT clothVertexPositions, 
	Vec3V* RESTRICT clothPrevVertexPositions, 
	ScalarV_In scaledDamping )
{
	// TODO: revise and optimize bitset usage IsClear - svetli

	Assert( clothVertexPositions );
	Assert( clothPrevVertexPositions );

	PF_FUNC( RopeIntegration );

	Vec3V gravityDelVel = Scale( Scale( gravity, timeStep ), gravityScale );
	Vec3V gravityDisplacement = Scale( gravityDelVel,timeStep );
	gravityDisplacement.SetW( ScalarV(V_ZERO) );

	const int firstVertex = GetNumLockedEdgesFront();
	const int lastVertex = GetNumVertices() - GetNumLockedEdgesBack();
	for (int vertexIndex = firstVertex; vertexIndex < lastVertex; ++vertexIndex )
	{		
		const Vec3V prevPosition = clothPrevVertexPositions[vertexIndex];	// Copy the previous vertex position before updating it.		
		const Vec3V vertexPosition = clothVertexPositions[vertexIndex];		// Update the previous vertex position to the current.
		clothPrevVertexPositions[vertexIndex] = vertexPosition;

		if (m_DynamicPinList.IsClear(vertexIndex))
		{
			// Compute the projected change in position (from steady velocity plus gravity minus damping).
			Vec3V prevDisplacement = Subtract(vertexPosition,prevPosition);
			Vec3V displacement = AddScaled( gravityDisplacement, prevDisplacement, scaledDamping );
			clothVertexPositions[vertexIndex] = Add( clothVertexPositions[vertexIndex], displacement );	
		}
	}
}



const ScalarV SCALAR_ZERO		= ScalarV(V_ZERO);
const ScalarV SCALAR_ONE		= ScalarV(V_ONE);
const ScalarV SCALAR_M_ONE		= -ScalarV(V_ONE);


inline void RecomputeTangent(  Vec3V_In vecLUV_0, Vec3V_In vecLUV_1 , Vec3V_In vecLUV_2
	, Vec3V_In vEdge1, Vec3V_In vEdge2 
	, Vec3V_InOut tangent0, Vec3V_InOut tangent1, Vec3V_InOut tangent2
	, Vec3V_InOut biNormal0, Vec3V_InOut biNormal1, Vec3V_InOut biNormal2 )
{
	const Vec3V vTexEdge1 = Subtract( vecLUV_1, vecLUV_0 );
	const Vec3V vTexEdge2 = Subtract( vecLUV_2, vecLUV_0 );

	ScalarV vTE1x = vTexEdge1.GetX(),
		vTE1y = vTexEdge1.GetY(),
		vTE2x = vTexEdge2.GetX(),
		vTE2y = vTexEdge2.GetY();

	ScalarV divisor = Subtract(Scale(vTE1x,vTE2y),Scale(vTE2x,vTE1y));
	divisor = SelectFT( IsEqual( divisor, SCALAR_ZERO ), Invert( divisor ), SCALAR_ZERO );

	const Vec3V tangent	= Scale( vEdge1, vTE2y ) - Scale( vEdge2, vTE1y );
	const Vec3V binormal= Scale( vEdge2, vTE1x ) - Scale( vEdge1, vTE2x );

	tangent0 = AddScaled( tangent0, tangent, divisor );
	tangent1 = AddScaled( tangent1, tangent, divisor );
	tangent2 = AddScaled( tangent2, tangent, divisor );

	biNormal0 = AddScaled( biNormal0, binormal, divisor );
	biNormal1 = AddScaled( biNormal1, binormal, divisor );
	biNormal2 = AddScaled( biNormal2, binormal, divisor );
}

// NOTE: this is used only by cloth, which is guaranteed to not have binormal channel
inline void SetTangentFast2(Vec3V_InOut vTan, const Vector3& tangent, const Vector3& binormal, const Vector3& normal)
{
	// Determine the sign of the binormal
	Vector3 txn(tangent);
	txn.Cross(normal);
	Vector3 vDot = txn.DotV(binormal);
	Vector3 vTest = vDot.IsLessThanV4(VEC3_ZERO);
	Vector3 vBi = vTest.Select(VEC3V_TO_VECTOR3(Vec3V(V_NEGONE)), VEC3V_TO_VECTOR3(Vec3V(V_ONE)));
	vTan = VECTOR3_TO_VEC3V(tangent);
	vTan.SetW( (VECTOR3_TO_VEC3V(vBi)).GetX() );
}


void clothController::ApplySimulationToMesh( grmGeometryQB& geometry, const Vector3& translationV, int renderableLodIndex, Vec3V_In 
#if __PFDRAW && !__SPU
											vDebugOffset 
#endif
											)
{
	PF_FUNC( ClothMapping );

//	Assertf( GetBridge(), "Cloth %s is missing clothBridgeSimGfx member. It won't be rendered. This is bug for art to check the cloth piece export (there might be an error?).", GetName() );
	if ( GetBridge() == NULL ) 
	{
		return;
	}

	grcIndexBuffer* RESTRICT pIndexBuffer = geometry.GetIndexBuffer();
	const int nIndices = pIndexBuffer->GetIndexCount();

	phVerletCloth* cloth = GetCloth(renderableLodIndex);
	Assert(cloth);
	const int nVerts = cloth->GetNumVertices();

	// Note that index buffers must always be locked before locking vertex
	// buffers.  This is to prevent possible deadlocks with different code paths
	// locking in different order.
#if __SPU
	int size = (nIndices * sizeof(u16) + 15) & ~15;
	u16* RESTRICT pIndexPtr = (u16*)sysScratchAlloc(size);
	sysDmaGet(pIndexPtr, (u32)(static_cast<grcIndexBufferGCM*>(pIndexBuffer)->GetGCMBuffer()), size, 0);
	Vec3V* RESTRICT pLNormals = sysScratchAllocObj<Vec3V>(nVerts);
#else
	const u16* RESTRICT pIndexPtr = pIndexBuffer->LockRO();
	WIN32PC_ONLY
	(
		Assert( pIndexPtr );
		if (pIndexPtr == NULL) 
		{
			return;
		}
	)

	Vec3V* RESTRICT pLNormals = Alloca(Vec3V, nVerts);
#endif // __SPU

	Assert( pLNormals );
	Assert( geometry.GetType() == grmGeometry::GEOMETRYQB );
	grcVertexBuffer* pMeshBuffer = geometry.GetVertexBuffer();
	Assert( pMeshBuffer != NULL );

	// Scoped so that meshBufferEditor is destroyed (and unlocks the vertex
	// buffer) before the index buffer is unlocked
	{
		grcVertexBufferEditor meshBufferEditor(pMeshBuffer);
		WIN32PC_ONLY
		(
			bool isMeshBufferEditorValid = meshBufferEditor.isValid();
			Assert( isMeshBufferEditorValid );
			if( !isMeshBufferEditorValid ) 
			{ 			
				pIndexBuffer->UnlockRO(); 
				return; 
			}
		)

		const u16* RESTRICT clothDisplayMap = GetClothDisplayMap( renderableLodIndex );
		Assert( clothDisplayMap );

		const Vec3V* RESTRICT clothVerts = (const Vec3V*)cloth->GetClothData().GetVertexPointer();
		Assert(clothVerts);

		Vec4V* RESTRICT fastSetX = (Vec4V *)(((u8 *)(pMeshBuffer->GetFastLockPtr())) /*+ pMeshBuffer->GetFvf()->GetOffset(grcFvf::grcfcPosition)*/ );

#if !__SPU
		for( int i = 0; i < ((nVerts >> 6) << 6); i += 64 )
		{
			PrefetchDC( &clothDisplayMap[i] );
		}
		//	PrefetchDC( &clothVerts[clothDisplayMap[iVert]] );
		PrefetchDC( fastSetX );
#endif

		sysMemSet(pLNormals, 0, sizeof(Vec3V) * nVerts);
		grcAssertf(pIndexPtr, "Couldn't get locked index buffer pointer");

#if __SPU
		sysDmaWait(1<<0);
#endif

		Assert( pMeshBuffer->GetVertexStride() == 32 );

		// TEMP: B*1731376
		if( pMeshBuffer->GetVertexStride() != 32 )
		{
			return;
		}
		const bool flipIndexWinding = m_IndicesWindingOrder;

		if( pMeshBuffer->GetFvf()->GetTangentChannel(0) )
		{
			const u32 vertexStride = pMeshBuffer->GetVertexStride();
			u8* blockPtr = (u8*)pMeshBuffer->GetFastLockPtr();

#if __SPU
			Vec3V* RESTRICT pLUVs = sysScratchAllocObj<Vec3V>(nVerts);
			Vec3V* RESTRICT pLTangents = sysScratchAllocObj<Vec3V>(nVerts);
			Vec3V* RESTRICT pLBiNormals = sysScratchAllocObj<Vec3V>(nVerts);	
#else
			Vec3V* RESTRICT pLUVs = Alloca(Vec3V, nVerts);
			Vec3V* RESTRICT pLTangents = Alloca(Vec3V, nVerts);
			Vec3V* RESTRICT pLBiNormals = Alloca(Vec3V, nVerts);	
#endif

			Assert( pLUVs );
			Assert( pLTangents );
			Assert( pLBiNormals );

			sysMemSet(pLTangents, 0, sizeof(Vec3V) * nVerts);
			sysMemSet(pLBiNormals, 0, sizeof(Vec3V) * nVerts);

			Vec3V _zero(V_ZERO);
			Vec3V vFlipV(V_Y_AXIS_WZERO);		

			const int uvOffset = m_UVOffset;
			//		Assert( fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTexture0)) == 4 );

			Vec3V maskXY = Add( Vec3V(V_X_AXIS_WZERO), Vec3V(V_Y_AXIS_WZERO) );

			int i;		
			for(i = 0; i < nVerts; ++i )
			{
				const Float16* uvF16 = reinterpret_cast<const Float16*>(blockPtr + vertexStride * i + uvOffset);
				pLUVs[i] = SubtractScaled( vFlipV, Vec3V( uvF16[0].GetFloat32_FromFloat16(), uvF16[1].GetFloat32_FromFloat16(), 0.0f), maskXY );
			}		

			const int extra0 = flipIndexWinding ? 2: 1;
			const int extra1 = flipIndexWinding ? 1: 2;

			for( i = 0; i < nIndices; i += 3 )
			{
				const u16 index0 = pIndexPtr[i];
				const u16 index1 = pIndexPtr[i+extra0];
				const u16 index2 = pIndexPtr[i+extra1];

				const int i0 = clothDisplayMap[index0];
				const int i1 = clothDisplayMap[index1];
				const int i2 = clothDisplayMap[index2];

				Vec3V vEdge1 = Subtract( clothVerts[i1], clothVerts[i0] );
				Vec3V vEdge2 = Subtract( clothVerts[i2], clothVerts[i0] );

				Vec3V normal = /*NormalizeFast*/(Cross(vEdge1, vEdge2));		// if you don't want vertex normals weighted by the size of the triangle then use NormalizeFast

				pLNormals[index0] = Add( pLNormals[index0], normal );
				pLNormals[index1] = Add( pLNormals[index1], normal );
				pLNormals[index2] = Add( pLNormals[index2], normal );

				// DX11 TODO:- Investigate why we hit this Assert().
				AssertMsg( ((i0 < nVerts) && (i1 < nVerts) && (i2 < nVerts)), "grmGeometryQB::RecomputeNormals()...Index out of range!" );

				RecomputeTangent( pLUVs[index0] , pLUVs[index1], pLUVs[index2], vEdge1, vEdge2, pLTangents[index0],pLTangents[index1], pLTangents[index2], pLBiNormals[index0], pLBiNormals[index1], pLBiNormals[index2] );
			}

			Assert( /*fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcNormal))*/ m_NormalOffset == 12 );
			//		const int tangentOffset = fvf->GetOffset(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0));
			//		const int tangentSize = fvf->GetSize(static_cast<grcFvf::grcFvfChannels>(grcFvf::grcfcTangent0));
			//		Displayf("tangentOffset: %d    tangentSize: %d", tangentOffset, tangentSize);
			//		Assert(  tangentOffset/*m_TangentOffset*/ == 16 );

			ScalarV _vhalfPi( Vec::V4VConstant(V_PI_OVER_TWO) );
			Vec3V _b = VECTOR3_TO_VEC3V( M34_IDENTITY.b );
			Vec3V _a = VECTOR3_TO_VEC3V( M34_IDENTITY.a );

			for(i = 0; i < nVerts; i++ )
			{
#if __XENON
				PrefetchDC( fastSetX+2 );			
#endif
				Vec4V part1 = *(Vec4V*)(fastSetX+1);

				Vec3V pos = Add( clothVerts[clothDisplayMap[i]], VECTOR3_TO_VEC3V(translationV) );
				Vec3V n = NormalizeFast(pLNormals[i]);
				Vec3V ntan = SelectFT( IsEqual( pLTangents[i], _zero ), pLTangents[i], SelectFT( IsLessThan( Dot(_b, n), _vhalfPi) , _a, _b));

				ScalarV ndt = Dot( n, ntan );
				Vec3V vTangent = AddScaled( ntan, n, -ndt );
				vTangent = NormalizeFast(vTangent);

				Vec3V vBiNormal = Cross( n, ntan );
				ScalarV vScale = Dot( vBiNormal, pLBiNormals[i]);
				BoolV IsLessThanV0 = IsLessThan( vScale, SCALAR_ZERO);
				vScale = SelectFT( IsLessThanV0, SCALAR_M_ONE, SCALAR_ONE );

				vBiNormal = Scale( vBiNormal, vScale );
				vBiNormal = NormalizeFast(vBiNormal);

				Vec3V tan;
				SetTangentFast2(tan, VEC3V_TO_VECTOR3( vTangent ), VEC3V_TO_VECTOR3( vBiNormal),  VEC3V_TO_VECTOR3( n ) );				

#if __XENON
				fastSetX[0].SetIntrin128( PackV1010102(pos.GetIntrin128Ref(), n.GetIntrin128Ref()) );				
#else
				fastSetX[0].SetXYZ( pos );
	#if __PS3
				(reinterpret_cast<int*>(&fastSetX[0].GetIntrin128Ref()))[3] = PackNormal_11_11_10(VEC3V_TO_VECTOR3(n));
	#else
				(reinterpret_cast<int*>(&fastSetX[0].GetIntrin128Ref()))[3] = PackNormal_8_8_8(VEC3V_TO_VECTOR3(n));
	#endif
				//fastSetX[1].SetIntrin128( Vec::V4Float16Vec4PackIntoZW(part1.GetIntrin128Ref(),tan.GetIntrin128Ref()) ) ;		
#endif // __XENON

				fastSetX[1].SetIntrin128( PackVFloat16(part1.GetIntrin128Ref(), tan.GetIntrin128Ref()) ) ;
				fastSetX += 2;
			}
		}
		else
		{
			const int extra0 = flipIndexWinding ? 2: 1;
			const int extra1 = flipIndexWinding ? 1: 2;

			for( int i = 0; i < nIndices; i += 3 )
			{
				const u16 index0 = pIndexPtr[i];
				const u16 index1 = pIndexPtr[i+extra0];
				const u16 index2 = pIndexPtr[i+extra1];

				const int i0 = clothDisplayMap[index0];
				const int i1 = clothDisplayMap[index1];
				const int i2 = clothDisplayMap[index2];

				Vec3V vEdge1 = Subtract( clothVerts[i1], clothVerts[i0] );
				Vec3V vEdge2 = Subtract( clothVerts[i2], clothVerts[i0] );

				Vec3V normal = /*NormalizeFast*/(Cross(vEdge1, vEdge2));	// if you don't want vertex normals weighted by the size of the triangle then use NormalizeFast

				pLNormals[index0] = Add( pLNormals[index0], normal );
				pLNormals[index1] = Add( pLNormals[index1], normal );
				pLNormals[index2] = Add( pLNormals[index2], normal );

				// DX11 TODO:- Investigate why we hit this Assert().
				AssertMsg( ((i0 < nVerts) && (i1 < nVerts) && (i2 < nVerts)), "grmGeometryQB::RecomputeNormals()...Index out of range!" );
			}

			for(int i = 0; i < nVerts; i++ )
			{
				PrefetchDC( fastSetX+2 );
				Vec4V part1 = *(Vec4V*)(fastSetX+1);

				Vec3V n = NormalizeFast(pLNormals[i]);

				// 				if( IsLessThanAll(Abs(n), Vec3V(V_FLT_LARGE_6)) == 0 )
				// 				{
				// 					Displayf("Write bad normal %d: %f %f %f", i, n.GetXf(), n.GetYf(), n.GetZf() );
				// 				}

				//				Assertf(IsLessThanAll(Abs(n), Vec3V(V_FLT_LARGE_6)), "Bad normal, Vertex Index %d", i );

				Vec3V pos = Add( clothVerts[clothDisplayMap[i]], VECTOR3_TO_VEC3V(translationV) );

				fastSetX[0] = Vec4V( Vec::V4PermuteTwo<Vec::X1,Vec::Y1,Vec::Z1,Vec::X2>(pos.GetIntrin128(), n.GetIntrin128()) );
				fastSetX[1] = Vec4V( Vec::V4PermuteTwo<Vec::Y1,Vec::Z1,Vec::Z2,Vec::W2>(n.GetIntrin128(), part1.GetIntrin128()) );
				fastSetX += 2;
			}
		}
		
#if __PFDRAW && !__SPU
		if( cloth->GetFlag(phVerletCloth::FLAG_ENABLE_DEBUG_DRAW) )
		{
			if( PFD_ClothNormals.Begin() )
			{
				static float scaleF = 1.0f;
				ScalarV normalScale = ScalarVFromF32( scaleF );

				for(int i = 0; i < nVerts; i++ )
				{
					Vec3V pos = Add( clothVerts[clothDisplayMap[i]], vDebugOffset );

					Vector3 pos0 = VEC3V_TO_VECTOR3(pos);
					Vector3 pos1 = VEC3V_TO_VECTOR3( AddScaled(pos, pLNormals[i], normalScale) );
					grcDrawLine( pos0, pos1, Color_green );
				}
				PFD_ClothNormals.End();
			}
			if( PFD_ClothFaceNormals.Begin() )
			{
				static float scaleF = 1.0f;
				ScalarV normalScale = ScalarVFromF32( scaleF );
				ScalarV _third(V_THIRD);
				const int extra0 = flipIndexWinding ? 2: 1;
				const int extra1 = flipIndexWinding ? 1: 2;

				for( int i = 0; i < nIndices; i += 3 )
				{
					const u16 index0 = pIndexPtr[i];
					const u16 index1 = pIndexPtr[i+extra0];
					const u16 index2 = pIndexPtr[i+extra1];

					const int i0 = clothDisplayMap[index0];
					const int i1 = clothDisplayMap[index1];
					const int i2 = clothDisplayMap[index2];

					Vec3V vEdge1 = Subtract( clothVerts[i1], clothVerts[i0] );
					Vec3V vEdge2 = Subtract( clothVerts[i2], clothVerts[i0] );

					Vec3V n = Cross( vEdge1, vEdge2 );

					Vec3V pos = Scale( Add( clothVerts[i0], Add(clothVerts[i1], clothVerts[i2]) ), _third );
					Vec3V pos0 = Add( pos, vDebugOffset );
					Vector3 pos1 = VEC3V_TO_VECTOR3( AddScaled( pos0, n, normalScale ) );
					grcDrawLine( VEC3V_TO_VECTOR3(pos0), pos1, Color_green );
				}
				PFD_ClothFaceNormals.End();
			}	
		} // if( cloth->GetFlag(FLAG_ENABLE_DEBUG_DRAW) )
#endif // #if __PFDRAW && !__SPU
	}


#if !__SPU
	geometry.GetIndexBuffer()->UnlockRO();
#endif
}

#define UNUSED_PARAM_CUSTOM_FVF(x)
#define USED_PARAM_CUSTOM_FVF(x) x


void clothController::GetVertexNormals( Vector3* RESTRICT vertNormals, u32 UNUSED_PARAM_CUSTOM_FVF(normalOffset), u32 vertexStride, u8* blockPtr, const POLYGON_INDEX* RESTRICT indexMap, int nVerts, bool USED_PARAM_CUSTOM_FVF(packedNormals) ) const
{
	Assert( vertNormals );
	Assert( indexMap );

	PF_FUNC( GetVertexNormals );

	Vec3V* RESTRICT vertNormalsV = (Vec3V* RESTRICT)vertNormals;

	if( packedNormals )
	{
		// NOTE: i.e. ... there is tangent channel
		for (int i = 0; i < nVerts; ++i )
		{	
			const u32 offset = vertexStride * indexMap[i];
#if __XENON
			Vec3V part0 = *(Vec3V*)(blockPtr+offset);
			vertNormalsV[ i ].SetIntrin128( UnpackV1010102(part0.GetIntrin128()) );
#else
			u32 packedNormal = *(u32*)(blockPtr+offset+12);
	#if __PS3
			UnpackNormal_11_11_10V( RC_VECTOR3(vertNormalsV[ i ]), packedNormal );
	#else
			UnpackNormal_8_8_8V( RC_VECTOR3(vertNormalsV[ i ]), packedNormal );
	#endif
#endif
			//			Assertf(IsLessThanAll(Abs(vertNormalsV[i]), Vec3V(V_FLT_LARGE_6)), "Vertex Index %d", i );
		}
	}
	else
	{
		for (int i = 0; i < nVerts; ++i )
		{	
			const u32 offset = vertexStride * indexMap[i];

			Vec4V part0 = *(Vec4V*)(blockPtr+offset);
			Vec4V part1 = *(Vec4V*)(blockPtr+offset+16);

			vertNormalsV[ i ] = Vec3V( Vec::V4PermuteTwo<Vec::W1,Vec::X2,Vec::Y2,Vec::Z2>(part0.GetIntrin128(), part1.GetIntrin128()) );
			//Assertf(IsLessThanAll(Abs(vertNormalsV[i]), Vec3V(V_FLT_LARGE_6)), "Vertex Index %d", i );
			// 			if( IsLessThanAll(Abs(vertNormalsV[i]), Vec3V(V_FLT_LARGE_6)) == 0 )
			// 			{
			// 				Displayf("Offset %d. Read bad normal %d: %f %f %f", offset, i, vertNormalsV[i].GetXf(), vertNormalsV[i].GetYf(), vertNormalsV[i].GetZf() );
			// 			}
		}
	}
}



} // namespace rage
