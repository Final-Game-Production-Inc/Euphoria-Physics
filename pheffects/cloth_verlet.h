//
// pheffects/cloth_verlet.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_CLOTH_VERLET_H
#define PHEFFECTS_CLOTH_VERLET_H


#include "cloth_verlet_spu.h"
#include "cloth_verlet_config.h"
#include "clothdata.h"

#include "atl/atfunctor.h"
#include "data/struct.h"
#include "system/task.h"
#include "pheffects/tune.h"


#define NO_CLOTH_BOUND_CLAMP				(1)
#if !NO_CLOTH_BOUND_CLAMP
	#define MAX_CLOTH_RADIUS				20.0f
	#define MAX_CLOTH_RADIUS_SQR			(MAX_CLOTH_RADIUS*MAX_CLOTH_RADIUS)
#endif


#define MAX_CHAR_CLOTH_VERTS			255
#define MAX_NUM_ROPE_VERTS				64
#define	MAX_CLOTH_VERTS_ON_SPU			(32*32 + 128)
#define	MAX_NUM_CULLED_POLYGIONS		128
#define	DEFAULT_CLOTH_WEIGHT			1.0f
#define	DEFAULT_INFLATION_SCALE			0.0f

#define	RECORDING_VERTS					__PFDRAW && !__SPU			
#define RECORDS_PER_FRAME				(6 * 2 * DEFAULT_ROPE_ITERATIONS)
#define MAX_RECORD_FRAMES				(120)
#define	MAX_RECORD_QUEUE_SIZE			(RECORDS_PER_FRAME * MAX_RECORD_FRAMES)

// TODO: not important
#define	NO_ROPE_WEIGHTS					(1)

// TODO: important
#define USED_IN_GTAV					(0)
#define	USE_CLOTH_PHINST				(0)
#define USE_FLAGS_IN_VERLET				(1)
#define CLOTH_SIM_MESH_ONLY				(1)



namespace rage {

class datCallback;
class phBoundCapsule;
class phBoundComposite;
class phBoundSphere;
class phBoundBox;
USE_TAPERED_CAPSULE_ONLY(class phBoundTaperedCapsule;)
class phCollider;
class phInst;
class TriangleShape;
class grmGeometryQB;

// NOTE: GetClassType will return enum defined in game side of the code.
enum en_PH_INST_TYPES
{
	DUP_PH_INST_FRAG_VEH = 15,		// PH_INST_FRAG_VEH
	DUP_PH_INST_FRAG_PED = 16,		// PH_INST_FRAG_PED
};


struct phVerletSPUDebug
{
	u8* debugDrawBuf;
	u32 debugDrawCountOut;
	u32 debugIdx;
	u32 padding[1];
};

struct phVerletClothUpdate
{
	Vector3 m_Gravity;
	Vector3 m_Force;
	Mat34V  m_Frame;

#if ENABLE_SPU_COMPARE
	u8* compareBuf;
#endif

#if ENABLE_SPU_DEBUG_DRAW
	u32 verletSpuDebugAddr;
#endif

	u32 boundingCenterAndRadiusAddress;

	u8* instLastMtxIdxAddrMM;
	Mat34V* instLastMatricsAddrMM;
	u8 vertexBufferIdx;

	int simLodIndex;		// lod index from the sim mesh
	int drwLodIndex;		// lod index from the drawable

	u32 userDataAddress;

	bool separateMotion;
	bool pauseSimulation;
	bool bClothSimMeshOnly;
};


struct phVerletCharacterClothUpdate
{
	Vector3 m_Gravity;
	Vector3 m_Force;
	Mat34V  m_Frame;

#if ENABLE_SPU_COMPARE
	u8* compareBuf;
#endif

#if ENABLE_SPU_DEBUG_DRAW
	u32 verletSpuDebugAddr;
#endif

	void*	spu_skeleton;
	const phBoundComposite* customBound;
	int* bonesIndices;

	u32 entityVertexBufferAddress;
	u32 flagsOffsetAddress;

	u8* instLastMtxIdxAddrMM;
	Mat34V* instLastMatricsAddrMM;

	bool pauseSimulation;
};


struct phVerletRopeUpdate
{
	Vector3 m_Gravity;

#if ENABLE_SPU_DEBUG_DRAW
	u32 verletSpuDebugAddr;
#endif

	u8* instLastMtxIdxAddrMM;
	Mat34V* instLastMatricsAddrMM;
};



// NOTE: only for debug purposes
#if RECORDING_VERTS

struct recordBase
{
	u32		frame;
};

struct recordLine : public recordBase
{
	Vec3V	m_V0;
	Vec3V	m_V1;

	void Draw();
};

struct recordSphere : public recordBase
{
	Vec3V	m_Center;
	float	m_Radius;

	Color32 m_Color;

	void Draw();
};

struct recordTriangle : public recordBase
{
	Vec3V	m_V0;
	Vec3V	m_V1;
	Vec3V	m_V2;

	void Draw();
};

struct recordCapsule : public recordBase
{
	Mat34V	m_Transform;
	float	m_Radius;
	float	m_Length;

	void Draw();
};

struct recordCustomClothEvent : public recordBase
{
	Vec3V m_Position;
	Vec3V m_Rotation;
	const char* m_Text;
	const char* m_ClothControllerName;
	int m_EventType;
	int m_Frames;	
	float m_CapsuleRadius;
	float m_CapsuleLength;
	void Draw(float x, float y);
};



template<class _Type>
class dbgRecords
{
public:
	dbgRecords()
		: m_Frame(0)
		, m_Size(0)
		, m_First(0)
	{
		USE_DEBUG_MEMORY();
		m_Q = rage_new _Type[MAX_RECORD_QUEUE_SIZE];
	}

	~dbgRecords()
	{
		USE_DEBUG_MEMORY();
		delete[] m_Q;
	}

	void Reset()
	{
		m_Frame = m_Size = m_First = 0;
	}

	int Next() const 
	{
		return (m_First+m_Size) % MAX_RECORD_QUEUE_SIZE;
	}

	void Push(_Type& d)
	{
		bool queueFull = (m_Size >= MAX_RECORD_QUEUE_SIZE) ? true: false;
		if( queueFull )
		{
			++m_First;
			m_First = (m_First >= MAX_RECORD_QUEUE_SIZE) ? 0: m_First;
		}
		d.frame = m_Frame;
		m_Q[ Next() ] = d;

		m_Size = queueFull ? m_Size: (m_Size+1);
	}

	void PopFirst()
	{
		Assert( m_Size > 0 );
		++m_First;
		m_First = (m_First >= MAX_RECORD_QUEUE_SIZE) ? 0: m_First;
		--m_Size;
	}

	_Type& Get(int idx)
	{
		return m_Q[ (m_First+idx) % MAX_RECORD_QUEUE_SIZE ];
	}

protected:
	_Type* m_Q;

public:
	u32 m_First;
	u32 m_Size;
	u32 m_Frame;
};

#endif // RECORDING_VERTS


class ALIGNAS(16) phEdgeData
{
public:

	DECLARE_PLACE(phEdgeData);

	phEdgeData(class datResource& rsc);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	phEdgeData(){}

	void Init()
	{
		m_vertIndices[0] = 0;
		m_vertIndices[1] = 0;	
		m_CompressionWeight = 0.0f;		
		m_EdgeLength2 = LARGE_FLOAT;
		m_Weight0 = 0.0f;
	}

#if !NO_ROPE_WEIGHTS
	// PURPOSE: Get the fraction of the edge motion that will apply to vertex 0.
	// RETURN:	the fraction of the edge motion that is on vertex 0
	// NOTES:	This is either 0.0f (no influence), 0.5f (both vertices equally) or 1.0f (only this vertex moves).
	ScalarV_Out GetRopeVertexWeight0 () const;

	// PURPOSE: Get the fraction of the edge force that will apply to vertex 1.
	// RETURN:	the fraction of the edge motion that is on vertex 1
	// NOTES:	This is either 0.0f (no influence), 0.5f (both vertices equally) or 1.0f (only this vertex moves).
	ScalarV_Out GetRopeVertexWeight1 () const;


	void SetRopeVertexWeights (ScalarV_In weight0, ScalarV_In weight1);
	void SetRopeVertexWeights (float weight0, float weight1);
#endif

	void PinRopeVertex0 ();
	void PinRopeVertex1 ();

	void UnpinRopeVertex0 ();
	void UnpinRopeVertex1 ();

	Vec4V_Out GetEdgeDataAsVector () const { return *(Vec4V*)this; }

	// PURPOSE: the index numbers of the two cloth vertices on the edge
	u16 m_vertIndices[2];

	// PURPOSE: the squared rest length of the edge
	float m_EdgeLength2;

#if !NO_ROPE_WEIGHTS
	union {
		float m_Weight0;

		struct {

			// PURPOSE: the fraction of the control for vertex 0 and vertex 1
			// NOTES:
			//	0: this edge does not affect either vertex
			//	1: this edge affects only vertex 0
			//	2: this edge affects only vertex 1
			//	3: this edge affects both vertices equally
			//	There are 6 unused bits.
			u8 m_VertexWeights;
			u8 m_TearStrength;	// future use
			u16	m_TimeOnFire;	// future use
		} m_RopeEdgeData;
	};
#else
	float m_Weight0;
#endif


	// PURPOSE:	scale factor for the cloth reaction to compression
	// NOTES:
	//	1. The default value for regular cloth edges is 0, meaning there is no resistance to compression.
	//	2. Bend edges normally have non-zero compression weight.
	//	3. A non-zero compression weight can help a low-polygon mesh behave like a higher-polygon mesh.
	float	m_CompressionWeight;

	PAR_SIMPLE_PARSABLE;
} ;


#if !NO_ROPE_WEIGHTS

inline ScalarV_Out phEdgeData::GetRopeVertexWeight0 () const
{
	switch (m_RopeEdgeData.m_VertexWeights)
	{
	case 0:
		return ScalarV(V_ZERO);
	case 1:
		return ScalarV(V_ONE);
	case 2:
		return ScalarV(V_ZERO);
	default:
		return ScalarV(V_HALF);
	}
}

inline ScalarV_Out phEdgeData::GetRopeVertexWeight1 () const
{
	switch (m_RopeEdgeData.m_VertexWeights)
	{
	case 0:
		return ScalarV(V_ZERO);
	case 1:
		return ScalarV(V_ZERO);
	case 2:
		return ScalarV(V_ONE);
	default:
		return ScalarV(V_HALF);
	}
}

#endif //!NO_ROPE_WEIGHTS


typedef atArray< u32 > phInstDatRefArray;


#define		PADDING_SIZE				(16 - sizeof(phPolygon::Index))
#define		MAX_TRIANGLES_PER_EDGE		32	
#define		MAX_BOXES_PER_EDGE			8	

struct TriangleTransformed
{
	Vec3V pt[3];
	Vec3V n;
	Vec3V pos;	
	phPolygon::Index  polyIdx;			// unique polygon index
	char padding[PADDING_SIZE];
};

struct BoxTransformed
{
	Vec3V pt[4];
	phPolygon::Index  polyIdx;			// unique polygon index
	char padding[PADDING_SIZE];
};


struct EdgeTriangleMap
{
	int triangleIdx[MAX_TRIANGLES_PER_EDGE];		// triangleIdx[0] is triangles count
};

struct EdgeBoxMap
{
	int boxIdx[MAX_BOXES_PER_EDGE];		// triangleIdx[0] is triangles count
};



// phVerletCloth

class phVerletCloth : public pgBase
{
public:
	static bool sm_SpuEnvUpdate;

	enum { RORC_VERSION = 8 };

	DECLARE_PLACE(phVerletCloth);

	phVerletCloth(class datResource& rsc);

#if CLOTH_INSTANCE_FROM_DATA
	void InstanceFromData(int vertsCapacity, int edgesCapacity);
#endif

	void InstanceFromTemplate( const phVerletCloth* copyme );

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	phVerletCloth();
	virtual ~phVerletCloth();

	void Shutdown();

	void UpdateCharacterCloth( float timeScale, Mat34V_In gravityTransform, Mat34V_In attachedFrame, float timeStep, Vec3V_In gravityV, ScalarV_In gravityScale, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset);
	void UpdateCharacterClothAgainstCollisionInsts( Mat34V_In attachedFrame, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset);
	void DetectAndEnforceList( Mat34V_In attachedFrame, phInstDatRefArray& collisionInst, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset );
	void DetectAndEnforceInstance( const bool bCollideEdges, Mat34V_In attachedFrame, const phInst *otherInstance, const phBound* customBound, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset );
	void UpdateEnvironmentCloth( Vec3V_In parentOffset, float timeScale, ScalarV_In timeStep, phInstDatRefArray& collisionInst, Vec3V_In gravityV, ScalarV_In gravityScale/*=ScalarV(V_ONE)*/);
	void UpdateEnvironmentClothAgainstCollisionInsts( Vec3V_In parentOffset, phInstDatRefArray& collisionInst);
	void UpdateEnvironmentRope( Vec3V_In gravity, ScalarV_In timeStep, phInstDatRefArray& collisionInst, ScalarV_In gravityScale=ScalarV(V_ONE));

	inline void IntegrationRope( Vec3V_In gravity, ScalarV_In timeStep, ScalarV_In gravityScale, Vec3V* RESTRICT clothVertexPositions, Vec3V* RESTRICT clothPrevVertexPositions, ScalarV_In scaledDamping );
	inline void IntegrationCloth_WithCompression( Vec3V* RESTRICT clothVertexPositions, Vec3V* RESTRICT clothPrevVertexPositions, const int numPinnedVerts, Vec3V_In gravityDisplacement, ScalarV_In scaledDamping );

	inline void IterationCloth( Mat34V_In attachedFrame, Vec3V* RESTRICT clothVertexPositions, phInstDatRefArray& collisionInst, Vec3V_In org, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset );
	inline void IterationRope( const phEdgeData* RESTRICT pEdgeDataPtr, Vec3V* RESTRICT clothVertexPositions, Vec3V* RESTRICT clothVertexPrevPositions, phInstDatRefArray& collisionInst, Vec3V_In org );

	inline void CalculateDeltaPositions_WithCompression(Vec3V* RESTRICT newVertexPositions, Vec3V* RESTRICT prevVertexPositions, const int numVerts);
	void ZeroDeltaPositions_WithCompression();

	inline void ApplyForce_WithCompression( Vec3V* RESTRICT vertNormals, int vertNormalStride, const float* RESTRICT vertWeights, Vec3V_In force, int nPin, int nVert, const float* RESTRICT inflationScale );
	inline void ApplyForceTransform_WithCompression( const int* RESTRICT matIndices, const Mat34V* RESTRICT tMat, Vec3V* RESTRICT vertNormals, int vertNormalStride, const float* RESTRICT vertWeights, Vec3V_In force, int nPin, int nVert, const float* RESTRICT inflationScale );

	inline void ApplyForce_NoCompression( Vec3V_In v_dragCoef, const Vector3& force, int nVert );

	inline void UpdateRope( Vec3V_In gravity, ScalarV_In timeStep, ScalarV_In gravityScale, Vec3V* RESTRICT clothVertexPositions, phInstDatRefArray& collisionInst, ScalarV_In scaledDamping );
	inline void UpdateCloth( Mat34V_In attachedFrame, Vec3V* RESTRICT clothVertexPositions, phInstDatRefArray& collisionInst, int numPinnedVerts, Vec3V_In gravityDisplacement, ScalarV_In scaledDamping, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset );
	inline void UpdateClothAgainstCollisionInsts( Mat34V_In attachedFrame, Vec3V* RESTRICT clothVertexPositions, phInstDatRefArray& collisionInst, int numPinnedVerts, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset );


#if (__PPU)
	void SetCollisionInst( phInstDatRefArray& collisionInst );
	void AddCollisionInst( u32 fatID );
#endif

	void ResetCollisionInst();

	sysTaskHandle UpdateEnvironmentClothSpu( Mat34V_In attachedFrame, ScalarV_In timeStep, phInstDatRefArray& collisionInst, ScalarV_In gravityScale=ScalarV(V_ONE), grmGeometryQB* vb = 0, void* clothcontroller = 0, int sizeofClothController = 0, Vec3V_In m_Force = Vec3V(V_ONE), int lodIndex = -1, phVerletCloth* drwCloth = 0, bool bClothSimMeshOnly = false, const bool separateMotion = false, const u32 userDataAddress = 0, phVerletSPUDebug* s_VerletSpuDebug = NULL );
	sysTaskHandle UpdateEnvironmentRopeSpu( ScalarV_In timeStep, phInstDatRefArray& collisionInst, ScalarV_In gravityScale=ScalarV(V_ONE), void* clothcontroller = 0, int sizeOfClothController = 0);

	void UpdateEdgesSoAVec3VBy8s (int numEdges, __vector4* XENON_ONLY(RESTRICT) clothVertexPositions, const phEdgeData* XENON_ONLY(RESTRICT) pEdgeData, float overRelaxationConstant);

	// NOTE: used only by rope 
	void UpdateEdgesVec3V( __vector4* XENON_ONLY(RESTRICT) clothVertexPositions, const phEdgeData* XENON_ONLY(RESTRICT) pEdgeData );

	void AddCustomEdges( const float *perVertexBendStrength = NULL );

	// Used only by rope
	void InitStrand(const int totalNumVerts, atArray<indxA2>& edgeToVertexIndices, const Vec3V* vertices, int numVertices, bool allocNormals, int extraVerts, Vec3V_In gravity );

	void InitDiamond(atArray<indxA2>& edgeToVertexIndices, const Vector2& size, int numSquareRows, int numSquareCols, const Vector3& position, const Vector3& rotation, bool allocNormals );
	void InitWithBound(atArray<indxA2>& edgeToVertexIndices, phBoundGeometry& clothBound, bool allocNormals, int extraVerts );
	void InitWithPolylist(atArray<indxA2>& edgeToVertexIndices, const Vec3V* vertices, int numVertices, const phPolygon* polygons, int numPolygons, bool allocNormals, int extraVerts);

	const phClothData& GetClothData() const { return m_ClothData; }
	phClothData& GetClothData()		{ return m_ClothData; }

	// set the edgelength2 for each of the edges so that the final max length will be fNewLength in total
	void SetLength(float fNewLength);

	void ScaleRopeLength(float lengthScale);
	void SetRopeLength(float length);

	void SwapVertex(int oldIndex, int newIndex, phClothConnectivityData* connectivity=NULL, bool allocNormals = false);
	void SwapVertexInEdges(int indexA, int indexB );

	void ApplyImpulse( const atBitSet& unpinnableList, Vec3V_In impulse, Vec3V_In position, atFunctor4<void,int,int,int,bool>* swapEventHandler, Vec3V_In vOffset);

	void PinVerts( int lodIndex, atArray< int > &vertIndicesToPin, phClothConnectivityData* connectivity, atFunctor4< void, int, int, int,bool > *swapEventHandler=NULL, float* perVertexCompression=NULL, bool allocNormals = false );

	void DynamicUnPinVerts( Mat34V_In frame, const atBitSet& unpinnableList, const int lodIndex, const int numVertsToUnpin, atArray<int>& vertIndicesToUnPin, atFunctor4< void, int, int, int, bool >* swapEventHandler, float* perVertexCompression=NULL, bool allocNormals = false );
	void DynamicPinVerts( Mat34V_In frame, const int lodIndex, const int numVertsToPin, atArray<int>&vertIndicesToPin, atFunctor4< void, int, int, int,bool > *swapEventHandler, float* perVertexCompression=NULL, bool allocNormals = false );

	void DynamicPinVertex(int vertexIndex);
	void DynamicUnpinVertex(int vertexIndex);
	void DynamicUnpinAll();

#if NO_PIN_VERTS_IN_VERLET
	bool IsPinned(int vertexIndex) const { return (vertexIndex < GetClothData().GetNumPinVerts() ); }
#else
	bool IsPinned(int vertexIndex) const { return (vertexIndex<m_NumPinnedVerts); }
#endif
	bool IsDynamicPinned(int vertexIndex) const { return m_DynamicPinList.IsSet(vertexIndex); }

#if !NO_BOUND_CENTER_RADIUS
	void GetBoundingSphere(Vector3& centre, float& radius) const;
	const Vector4 GetBoundingCenterAndRadius() const;
#endif

	void ApplyAirResistance( const float* RESTRICT inflationScale, const float* RESTRICT vertWeights, Vec3V* RESTRICT vertNormals, int vertNormalStride, const Vector3& windVector = Vector3(Vector3::ZeroType) ); 
	void ApplyAirResistanceTransform( const int* RESTRICT matIndices, const Mat34V* RESTRICT tMat, const float* RESTRICT inflationScale, const float* RESTRICT vertWeights, Vec3V* RESTRICT vertNormals, int vertNormalStride, const Vector3& windVector = Vector3(Vector3::ZeroType) ); 
	void ApplyAirResistanceRope( float dragCoef, const Vector3& windVector);

	void Init();
	void InitEdgeData(const float* perVertexCompression, const atArray<indxA2>& edgeToVertexIndices, const int numExtraEdges );

	const phVerletCloth* GetVerletClothType() const { return m_VerletClothType ? m_VerletClothType: this; }
	const atArray<phEdgeData>& GetEdgeList()		const { return m_VerletClothType ? m_VerletClothType->m_EdgeData : m_EdgeData; }
	const atArray<phEdgeData>& GetCustomEdgeList()	const { return m_VerletClothType ? m_VerletClothType->m_CustomEdgeData : m_CustomEdgeData; }
	const atArray<phEdgeData>& GetInstanceCustomEdgeList() const { return m_CustomEdgeData; }

	bool		  IsCollideEdges()			const { return m_CustomBound.ptr ? GetFlag(FLAG_COLLIDE_EDGES): ( m_VerletClothType ? m_VerletClothType->GetFlag(FLAG_COLLIDE_EDGES): false); }
	const phBound*	  GetCustomBound()			const { return m_CustomBound.ptr ? m_CustomBound.ptr: ( m_VerletClothType ? m_VerletClothType->m_CustomBound.ptr: NULL); }
	const phEdgeData& GetEdge(int idx)			const { return m_VerletClothType ? m_VerletClothType->m_EdgeData[idx] : m_EdgeData[idx]; }
	const phEdgeData& GetCustomEdge(int idx)	const { return m_VerletClothType ? m_VerletClothType->m_CustomEdgeData[idx] : m_CustomEdgeData[idx]; }
	phEdgeData& GetEdge(int idx)			{ return m_VerletClothType ? const_cast<phEdgeData&>(m_VerletClothType->m_EdgeData[idx]) : m_EdgeData[idx]; }
	phEdgeData& GetCustomEdge(int idx)	{ return m_VerletClothType ? const_cast<phEdgeData&>(m_VerletClothType->m_CustomEdgeData[idx]) : m_CustomEdgeData[idx]; }

	void TransformTriangle( const TriangleShape* RESTRICT triangleBound, Mat34V_In boundPose, Vec3V_InOut a, Vec3V_InOut b, Vec3V_InOut c, Vec3V_InOut triangleNormal, Vec3V_InOut trianglePosition );

	// TODO: rename DetectAndEnforce... to something more meaningful
	void DetectAndEnforceOne( const bool bCollideEdges, const phBound* RESTRICT bound, Mat34V_In boundPose, const Mat34V* RESTRICT customMatrices, Vec3V_In parentOffset );	
	void DetectAndEnforceOneCapsule( const bool bCollideEdges, ScalarV_In boundRadius, Vec3V_In boundCentroid, ScalarV_In capsuleLength, ScalarV_In capsuleRadius, Mat34V_In boundPose, Vec3V_In parentOffset );
	void DetectAndEnforceOnePlane(Vec3V_InOut vtx, Vec3V_In planeNormal, Vec3V_In planePos, Vec3V_In parentOffset);

#if USE_TAPERED_CAPSULE
	void DetectAndEnforceOneTaperedCapsule(const phBoundTaperedCapsule* RESTRICT bound, Mat34V_In boundPose );
#endif

	bool DetectAndEnforceOneTriangle( Vec3V_InOut vtx, Vec3V_In a, Vec3V_In b, Vec3V_In c, Vec3V_In triangleNormal, Vec3V_In trianglePosition );

#if USED_IN_GTAV
	void DetectAndEnforceOneSphere(const phBoundSphere* RESTRICT bound, Mat34V_In boundPose );
	void DetectAndEnforceOneGeometry( const phBoundGeometry* RESTRICT bound, Mat34V_In boundPose, Vec3V* RESTRICT pClothVerts, Vec3V* RESTRICT pPrevClothVerts );
	void DetectAndEnforceOneBox( const phBound* RESTRICT bound, Mat34V_In boundPose, Vec3V* RESTRICT pClothVerts, Vec3V* RESTRICT pPrevClothVerts );
#endif

#if USE_CAPSULE_EXTRA_EXTENTS
	void CollideCapsuleWithExtraExtents(const bool bCollideEdges, const phBoundCapsule* RESTRICT capsuleBound, Mat34V_In boundPose, Vec3V_In parentOffset );
#endif

	void CullPrimitives(const phBound* RESTRICT bound, Mat34V_In boundPose, Vec3V* RESTRICT pClothVerts, Vec3V* RESTRICT pPrevClothVerts, Vec3V_In parentOffset);

	void ComputeBoundingVolume(phBound* bound=NULL, Vector3* worldCenter=NULL, float* radius=NULL);
	void ComputeClothBoundingVolume();

#if !NO_PIN_VERTS_IN_VERLET
	int GetPinCount()	const { return m_NumPinnedVerts; }
#endif

	void SetNumEdges(int numEdges) 
	{
		Assert( numEdges <= GetEdgeCapacity() );
		m_NumEdges = numEdges;
	}
	int  GetNumEdges()	const { return m_NumEdges; }
	int  GetEdgeCapacity() const { return m_EdgeData.GetCapacity(); }

#if NO_VERTS_IN_VERLET
	int GetNumVertices() const { return GetClothData().GetNumVerts(); }
#else
	int GetNumVertices()const { return m_NumVertices; }
#endif

	phBoundComposite& CreateRopeBound(ScalarV_In ropeRadius);
// 	phBoundComposite& CreateRopeBound(float ropeRadius=-1.0f);
// 	phBoundComposite& CreateRopeBound(int initialNumEdges, float ropeLength, float startRadius, float endRadius=-1.0f);

	ScalarV_Out ComputeRopeLength() /*const*/;

	static int CountEdges(const phPolygon* triangles, int numTriangles);
	static bool ClampCompositeExtents (phBoundComposite& compositeBound, float maxRadius);

	void AttachVirtualBoundCapsule(const float capsuleRadius, const float capsuleLen, Mat34V_In boundMat, const int boundIdx/*, const Mat34V* transformMat*/ );
	void AttachVirtualBoundGeometry(const u32 numVerts, const u32 numPolys, const Vec3V* RESTRICT verts, const phPolygon::Index* RESTRICT triangles, const int boundIdx);
	void AttachBound( phBound* virtualBound, Mat34V_In boundMat, const int boundIdx);
	void CreateVirtualBound( int numBounds, const Mat34V* transformMat );
	void DetachVirtualBound();

	inline int GetNumActiveEdges() const { return (m_NumEdges - m_NumLockedEdgesFront - m_NumLockedEdgesBack); }
	inline int GetNumLockedEdges() const { return (GetNumLockedEdgesFront() + GetNumLockedEdgesBack()); }
	inline int GetNumLockedEdgesFront() const { return m_NumLockedEdgesFront; }
	inline int GetNumLockedEdgesBack() const { return m_NumLockedEdgesBack; }
	inline void SetNumLockedEdgesFront(int numEdgesToLock) { m_NumLockedEdgesFront = numEdgesToLock; }
	inline void SetNumLockedEdgesBack(int numVertsToLock) { m_NumLockedEdgesBack = numVertsToLock; }

#if __ASSERT
	void CheckVerts(const char*, const char*, u32);
	bool TestVertexToBox( Vec3V_In vtx, Vec3V_In boxMax, Vec3V_In boxMin, ScalarV_In threshold );
#endif

// TODO: work in progress
	void AddCustomEdge(const u16 vtxIndex0, const u16 vtxIndex1, const float edgeLen2, const float compressionWeight);
	void RemoveCustomEdge(const u32 edgeIdx);

// TODO: tear is still work in progress
	u16 TearEdges( int vertexIdx, int edgeIdx0, int edgeIdx1, u16* RESTRICT pClothDisplayMap, Vec3V* RESTRICT pClothVertices );

	Vec3V GetEdgeVec3V(int vertexIdx, int edgeIdx, Vec3V* RESTRICT pClothVertices);
	void CopyEdge(int vertexIdx, int newVertexIdx, int edgeToCopyIdx);
	void DetachEdge( int vertexIdx, int newVertexIdx, int edgeToDetachIdx );

#if __PPU
	static void DebugDrawSPU();
	static void InitDebugSPU();
	static void ShutdownDebugSPU();
#endif	

	float GetClothWeight() const { return m_ClothWeight; }
	void SetClothWeight(float clothWeight) { m_ClothWeight = clothWeight; }


	enum enVERLET_CLOTH_FLAGS
	{
		FLAG_NONE				= 0,
		FLAG_IGNORE_OFFSET		= 1 << 0,
		FLAG_COLLIDE_EDGES		= 1 << 1,
		FLAG_IS_ROPE			= 1 << 2,
#if __PFDRAW
		FLAG_ENABLE_DEBUG_DRAW	= 1 << 3,
#endif
	};

	bool GetFlag(enVERLET_CLOTH_FLAGS flagToCheck) const { return (m_Flags & flagToCheck) ? true: false; }
	void SetFlag(enVERLET_CLOTH_FLAGS flagToSet, bool val)
	{
		if( val )
			m_Flags |= flagToSet;
		else
			m_Flags &= (~flagToSet);
	}

#if NO_BOUND_CENTER_RADIUS
	Vec3V GetCenter() const { return Scale( Add(m_BBMax, m_BBMin), ScalarV(V_HALF) ); }
	float GetRadius( Vec3V_In center ) const { return Mag( Subtract( center, m_BBMax) ).Getf(); }
#else
	Vec3V GetCenter() const { return VECTOR3_TO_VEC3V(m_BoundingCenterAndRadius.GetVector3()); }
	float GetRadius( Vec3V_In /*center*/ ) const { return m_BoundingCenterAndRadius.GetW();	}
#endif

	Vec3V_Out GetBBMin() const { return m_BBMin; }
	Vec3V_Out GetBBMax() const { return m_BBMax; }

	void SetBBMin(Vec3V_In v) { m_BBMin = v; }
	void SetBBMax(Vec3V_In v) { m_BBMax = v; }

public:

	pgRef<const phVerletCloth> m_VerletClothType;

	datOwner<phBound> m_CustomBound;

// TODO: m_BoundingCenterAndRadius is actually not needed and can be computed when needed from bbmin and bbmax ( practically only in ComputeBoundingVolume )
#if NO_BOUND_CENTER_RADIUS
	char m_Pad007[sizeof(Vector4)];
#else
	Vector4 m_BoundingCenterAndRadius;		// bounding sphere centre is relative to cloth vertex 0
#endif

protected:
	Vec3V	m_BBMin;
	Vec3V	m_BBMax;
public:

	phClothData m_ClothData;

	char m_Pad005[sizeof(u32*)*2 + sizeof(float)];

//	float m_GravityFactor;				// TODO: this is needed only because is used in the SPU job

	char m_Pad00[sizeof(atBitSet) + sizeof(u32)];

	datOwner<phBound> m_VirtualBound;	

protected:

	// TODO: pinned verts count should go to clothdata
#if NO_PIN_VERTS_IN_VERLET
	char m_Pad001[sizeof(int)];
#else
	int m_NumPinnedVerts;
#endif

	int m_NumEdges;

#if NO_VERTS_IN_VERLET
	char m_Pad002[sizeof(int)];
#else
	int m_NumVertices;
#endif

public:

	char m_Pad01[4];

	u16 m_nIterations;						// the number of verlet integration steps per update
	u16 m_Flags;

	char m_Pad006[sizeof(bool)*4];

// TODO: custom edge data will be combined with edge data ( once new cloth pipeline is ready )
	atArray<phEdgeData> m_CustomEdgeData;
	atArray<phEdgeData> m_EdgeData;
	
#if __PS3
	pgRef<phBoundComposite> m_PedBound0;
	u32 m_PedBoundMatrix0;
#else
	pgRef<phBoundComposite> m_PedBound0;
	datRef<Mat34V> m_PedBoundMatrix0;
#endif

// TODO: wanted to remove this array but is really needed by the rope
//	we need some ropes on RDR colliding with the terrain to figure out if is worth keeping it around
	atArray<u32>		m_CollisionInst;
	atBitSet			m_DynamicPinList;	

	int m_NumLockedEdgesFront;
	int m_NumLockedEdgesBack;

// TODO: lod has the same weight as the highest lod. 
//	lower lod should be heavier, the weight should be some kind of mix between highest lod weight and vertex ratio between the lods
	float	m_ClothWeight;		// technically this is just a multiplier. default is 1.0f .. higher makes cloth more heavy ... less more light	

	char m_Pad0[4];	

#if __PS3	
	u32 m_VirtualBoundMat;					// ah, the SPU
	pgRef<phBoundComposite> m_PedBound1;
	u32 m_PedBoundMatrix1;
#else
	datRef<Mat34V> m_VirtualBoundMat;
	pgRef<phBoundComposite> m_PedBound1;
	datRef<Mat34V> m_PedBoundMatrix1;
#endif


	static const char* sm_tuneFilePath;
	static const char* sm_MetaDataFileAppendix;

#if __BANK && !__RESOURCECOMPILER
public:
	int PickVertices( const Vector3* RESTRICT clothVertices, const Vector3& offset, int startVertex, int endVertex, Vector3& mouseScreen, Vector3& mouseFar );
	int PickEdges( const Vector3* RESTRICT clothVertices, const Vector3& offset, Vector3& mouseScreen, Vector3& mouseFar, int& vtxIndex, int* edgesFound, const atArray<phEdgeData>& edgeData );

#endif
	int SearchEdges( const Vector3* RESTRICT clothVertices, const Vector3& offset, const int vtxIndex, int* edgesFound, const atArray<phEdgeData>& edgeData);
#if __PFDRAW
	void DrawPickedEdges( const Vector3* RESTRICT clothVertices, const Vector3& offset, const int edgesFoundCount, const int* RESTRICT edgesFound, const atArray<phEdgeData>& edgeData, Color32* RESTRICT edgesColors = NULL );
#endif

#if !__SPU
	void Load(void* controllerName, const char* filePath = "common:/data/cloth/");	
	void Save(void* controllerName, const char* filePath = "common:/data/cloth/");
#if __BANK && !__RESOURCECOMPILER
	void RegisterRestInterface(const char* controllerName);
	void UnregisterRestInterface(const char* controllerName);
#endif
#endif
	PAR_SIMPLE_PARSABLE;
};


} // namespace rage

#endif // end of #ifndef PHEFFECTS_CLOTH_H
