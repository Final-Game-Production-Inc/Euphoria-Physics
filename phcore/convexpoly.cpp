//
// phcore/convexpoly.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "convexpoly.h"

#include "vector/vector2.h"

namespace rage {

static Vector2 rayA[CPI_MAXPOLYSIDES];
static float dA[CPI_MAXPOLYSIDES];
static Vector2 rayB[CPI_MAXPOLYSIDES];
static float dB[CPI_MAXPOLYSIDES];
static bool SwitchedAB = false;
static int PreSeekCountdown;
static float vHeadOut;
static float vTailOut;
static float uHeadOut;
static float uTailOut;
enum { ModeA, ModeB, ModeC, ModeD, ModeExit };
static int Mode;
static int curA;			// Current vertex number of A
static int curB;			// Current vertex number of B
static int countdownA;		// Number of edges after intersection found
static int countdownB;		// Number of edges after intersection found
static int numA, numB;		// Total number of vertices
static const Vector2* vertsA;
static const Vector2* vertsB;
static int numIsects;		// Number of returned intersections.
static phConvexPoly::Data* CPIlist;

#if __WIN32
#pragma warning(disable: 4127)	// Conditional expression is constant (prevents while (1) from working)
#endif

inline float phConvexPoly::SideTestA( const Vector2* pt )
{
	return ( Cross( rayA[curA], *pt ) - dA[curA] );
}

inline float phConvexPoly::SideTestB( const Vector2* pt )
{
	return ( Cross( rayB[curB], *pt ) - dB[curB] );
}

inline void phConvexPoly::SwitchAdvancingEdge() {
	SwitchedAB = !SwitchedAB;
	vHeadOut = uHeadOut;		// Make vTailOut be computed correctly after AdvanceV
	vTailOut = uTailOut;		// Also needed for Mode A
}

/*
PURPOSE
	finds the extreme points of the intersection of two convex polygons
RETURN VALUE
	the number of intersection points returned (or zero)
NOTES
1.	The two polygons are given by the sequence of vertices in counterclockwise order.
2.	The values returned are given by specifying the edge(s) and t value(s) of the points. */
int phConvexPoly::ConvexPolyIntersect( int numberA, const Vector2* verticesA,
						 int numberB, const Vector2* verticesB,
						 phConvexPoly::Data* IntersectionList )
{
	Assert (numberA<=CPI_MAXPOLYSIDES 
			&& numberB<=CPI_MAXPOLYSIDES);
	Assert (numberA>=3 && numberB >=3);

	vertsA = verticesA;
	vertsB = verticesB;
	countdownA = numA = numberA;
	countdownB = numB = numberB;
	CPIlist = IntersectionList;

	PrecomputeRays(numA, vertsA, rayA, dA);
	PrecomputeRays(numB, vertsB, rayB, dB);

	// Control variables for the main loop
	SwitchedAB = false;
	PreSeekCountdown = numA+numA+numB+numB;

	numIsects = 0;				// Number of edges output
	curA = 0;					// Vertex number of A
	curB = 0;					// Vertex number of B
	uHeadOut = SideTestB(vertsA+1);
	vHeadOut = SideTestA(vertsB+1);
	uTailOut = SideTestB(vertsA);
	vTailOut = SideTestA(vertsB);

	if (vHeadOut>=0.0f) {
		if ( uHeadOut<0.0f ) {
			if ( uTailOut>0.0f && vTailOut<0.0f ) {
				if ( vHeadOut==0.0f ) {
					Mode = ModeD;	// Mode D records the intersection
				}
				else {
					// have an intersection!
					Mode = ModeC;
					RecordEE();			// Record it
				}
			}
			else {
				Mode = ModeA;
			}
		}
		else {
			if ( vHeadOut<vTailOut ) {	// if v points to u's line
				Mode = ModeA;
			}
			else if ( uHeadOut<uTailOut ) { // if u points to v's line
				Mode = ModeA;
				SwitchAdvancingEdge();
			}
			else {
				//RecordNoIsect( CPI_NONE_EE );
				//Mode = ModeExit;
				Mode = ModeA;
			}
		}
	}
	else {
		if ( uHeadOut>=0 ) {
			if ( uTailOut<0.0f && vTailOut>0.0f ) {
				if ( uHeadOut==0.0f ) {
					Mode = ModeD;
				}
				else {
					// have an intersection
					Mode = ModeC;
					RecordEE();		// Record it
				}
			}
			else {
				Mode = ModeA;
			}
			SwitchAdvancingEdge();
		}
		else {
			Mode = ModeB;
			if (uTailOut<uHeadOut) {	// if u points to v
				SwitchAdvancingEdge();
			}
		}
	}


	// Main loop: is iterated every time the mode changes.
	while (1) {
		switch ( Mode ) {

		case ModeA:
			{
				bool towards = (vHeadOut<vTailOut);
				while (1) {
					PreSeekCountdown--;
					if ( PreSeekCountdown<=0 ) {	// Exceeded limits
						RecordInteriorCollides( SwitchedAB );
						Mode = ModeExit;
						break;
					}
					AdvanceV();
					vTailOut = vHeadOut;
					GetvHeadOut();
					if ( vHeadOut<0.0f ) {
						GetuTailOut();
						GetuHeadOut();				// Needs to be set before SwitchAdvancingEdge called
						if ( uTailOut>=0 ) {
							towards = false;		// edge v is pointing away from edge u.
							SwitchAdvancingEdge(); // change mode
						}
						else {
							if ( uHeadOut<=0.0f ) {
								if ( uHeadOut==0.0f ) {
									Mode = ModeD;
								}
								else {
									Mode = ModeB;
								}
								SwitchAdvancingEdge();
								break;
							}
							else {
								Mode = ModeC;
								if ( vTailOut>0.0f ) {
									RecordEE();
								}
								else {
									RecordVTail();
								}
								SwitchAdvancingEdge();
								break;
							}
						}
					}
					else {
						if ( towards ) {
							if ( vHeadOut>=vTailOut ) {
								Mode = ModeExit;
								if ( vHeadOut==vTailOut ) {
									RecordNoIsect( CPI_NONE_EE );
								}
								else {
									RecordNoIsect( SwitchedAB?CPI_NONE_EV:CPI_NONE_VE );
								}
								break;
							}
						}
						else {
							towards = (vHeadOut<vTailOut);
						}
					}
				}
			}
			break;

		case ModeB:
			while (1) {
				PreSeekCountdown--;
				if ( PreSeekCountdown<=0 ) {	// Exceeded limits
					RecordInteriorCollides( !SwitchedAB );
					Mode = ModeExit;
					break;
				}
				AdvanceV();
				vTailOut = vHeadOut;
				GetuHeadOut();
				if ( uHeadOut>=0.0f ) {
					GetuTailOut();				// Need this for Mode A
					Mode = ModeA;
					SwitchAdvancingEdge();
					break;
				}
				else {
					GetvHeadOut();
					if (vHeadOut<0.0f) {
						continue;
					}
					GetuTailOut();
					if ( uTailOut>0.0f ) {
						// Found an intersection
						if ( vHeadOut==0.0f ) {
							Mode = ModeD;		// Mode D will record
							break;
						}
						else {
							Mode = ModeC;
							RecordEE();			// Record it
							break;
						}
					}
					else if ( uTailOut==0.0f ) {
						if ( vHeadOut>0.0f ) {
							Mode = ModeC;
							RecordUTail();		// Record intersection
							break;
						}
						else {			// vHeadOut==0.0f
							continue;
						}
					}
					else {				// uTailOut<0
						Mode = ModeA;
						break;
					}
				}
			}
			break;

		case ModeC:
			while (1) {
				AdvanceV();
				if ( Mode==ModeExit ) {
					break;
				}
				vTailOut = vHeadOut;
				GetvHeadOut();
				if (vHeadOut<0.0f) {
					GetuHeadOut();
					if ( uHeadOut>0.0f ) {
						if ( vTailOut>0.0f ) {
							GetuTailOut();
							RecordEE();
						}
						else {
							RecordVTail();
						}
					}
					else {
						Mode = ModeD;
					}
					SwitchAdvancingEdge();
					break;
				}
			}
			break;

		case ModeD:
			while (1) {
				AdvanceV();
				if ( Mode==ModeExit ) {
					break;
				}
				vTailOut = vHeadOut;
				GetuHeadOut();
				RecordVTail();		// Record the intersection!
				if ( Mode==ModeExit ) {
					break;
				}
				if ( uHeadOut>=0.0f ) {
					Mode = ModeC;
					SwitchAdvancingEdge();
					break;
				}
				else {
					GetvHeadOut();
					if ( vHeadOut>0.0f ) {
						Mode = ModeC;
						if ( vTailOut<0.0f ) {
							GetuTailOut();
							RecordEE();
						}
						break;
					}
				}
			}
			break;

		case ModeExit:
			return ( numIsects );
		}
	}

//	return -1;
}

void phConvexPoly::PrecomputeRays( int num, const Vector2* verts,
					Vector2* rays, float* dValues )
{
	int i;
	const Vector2* nextVert = verts;
	const Vector2* thisVert = verts+(num-1);
	Vector2* thisRay = rays+(num-1);
	float* dPtr = dValues+(num-1);
	for ( i=num; i>0; i-- ) {
		*thisRay = *nextVert;
		thisRay->Subtract(*thisVert);
		*dPtr = Cross(*thisRay,*thisVert);
		nextVert = thisVert;
		thisVert--;
		thisRay--;
		dPtr--;
	}
}

void phConvexPoly::AdvanceV() {
	if ( SwitchedAB ) {
		curA ++;
		countdownA--;
		if (curA>=numA) {
			curA = 0;
		}
	}
	else {
		curB ++;
		countdownB--;
		if ( curB>=numB ) {
			curB = 0;
		}
	}
//	if ( numIsects>0 && ( (countdownA<=0 && countdownB<=0) || countdownA<0 || countdownB<0 )  )
	if ( numIsects>0 && ( countdownA<0 || countdownB<0 ) )
	{
		Mode = ModeExit;
	}
}

inline float phConvexPoly::GetHeadOut( bool switched) {  // Returns B-head-out if false
	int i;
	if ( !switched ) {
		i = curB+1;
		if ( i>=numB ) {
			i = 0;
		}
		return ( SideTestA( &vertsB[i] ) );
	}
	else {
		i = curA+1;
		if ( i>=numA ) {
			i = 0;
		}
		return ( SideTestB( &vertsA[i] ) );
	}
}

void phConvexPoly::GetvHeadOut() {
	vHeadOut = GetHeadOut (SwitchedAB);
}

void phConvexPoly::GetuHeadOut() {
	uHeadOut = GetHeadOut (!SwitchedAB);
}

void phConvexPoly::GetuTailOut() {
	if ( !SwitchedAB ) {
		uTailOut = SideTestB ( & vertsA[curA] );
	}
	else {
		uTailOut = SideTestA ( & vertsB[curB] );
	}
}

		
// The Record routines record intersections.
//		They may also change the Mode.

void phConvexPoly::RecordEE()
{
	if ( numIsects>0 && CPIlist->IsectType==CPI_EE
					 && curA == CPIlist->edgeA
					 && curB == CPIlist->edgeB ) {
		Mode = ModeExit;
		return;
	}
	if ( numIsects==0 ) {
		countdownA = numA;
		countdownB = numB;
	}

	Assert ( numIsects < numA+numB );

	phConvexPoly::Data* thisIsect = CPIlist+numIsects;

	if ( !SwitchedAB ) {
		thisIsect->tA = uTailOut/(uTailOut-uHeadOut);
		thisIsect->tB = vTailOut/(vTailOut-vHeadOut);
	}
	else {
		thisIsect->tA = vTailOut/(vTailOut-vHeadOut);
		thisIsect->tB = uTailOut/(uTailOut-uHeadOut);
	}
	thisIsect->IsectType = CPI_EE;	// Edge to edge
	thisIsect->edgeA = curA;
	thisIsect->edgeB = curB;
	numIsects++;
}

void phConvexPoly::RecordUTail() {
	// Tail of u is inside the other polygon
	RecordTail( SwitchedAB );
}

void phConvexPoly::RecordVTail() {
	// Tail of v is inside the other polygon
	RecordTail( !SwitchedAB );
}

void phConvexPoly::RecordTail( bool switchFlag ) {
	// switchFlag is false if recording an A edge tail.
	if ( numIsects==0 ) {
		countdownA = numA;
		countdownB = numB;
	}

	phConvexPoly::Data* thisIsect = CPIlist+numIsects;
	if ( !switchFlag ) {
		if ( numIsects>0 && CPIlist->IsectType==CPI_VP
						 && CPIlist->edgeA == curA ) {
			Mode = ModeExit;
			return;
		}
		thisIsect->IsectType = CPI_VP;
		thisIsect->edgeA = curA;
	}
	else {
		if ( numIsects>0 && CPIlist->IsectType==CPI_PV
						 && CPIlist->edgeB == curB ) {
			Mode = ModeExit;
			return;
		}
		thisIsect->IsectType = CPI_PV;
		thisIsect->edgeB = curB;
	}
	Assert ( numIsects < numA+numB );
	numIsects++;
}

void phConvexPoly::RecordNoIsect( int TypeCode ) {
	Assert (numIsects==0);

	CPIlist->IsectType = TypeCode;
	CPIlist->edgeA = curA;
	CPIlist->edgeB = curB;
}

void phConvexPoly::RecordInteriorCollides( bool SwitchedFlag )
{
	if ( !SwitchedFlag ) { 
		// A is inside B
		for (curA=0; curA<numA; curA++ ) {
			RecordTail( false );
		}
	}
	else {
		for ( curB=0; curB<numB; curB++ ) {
			RecordTail( true );
		}
	}
}

} // namespace rage
