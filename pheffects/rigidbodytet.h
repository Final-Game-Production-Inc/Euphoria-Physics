//
// pheffects/clothverletinst.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_RIGIDBODY_TET_H
#define PHEFFECTS_RIGIDBODY_TET_H

#include "cloth_verlet.h"


namespace rage
{

class phRigidBodyTet
{
public:

	const phRigidBodyTet &operator=( const phRigidBodyTet& rbt)
	{
	
		m_VertSpacing2 = rbt.m_VertSpacing2;

		int iFour;
		for( iFour = 0; iFour < 4; iFour++ )
		{
			m_Vert[iFour] = rbt.m_Vert[iFour];
			m_PrevVert[iFour] = rbt.m_PrevVert[iFour];
		}

		return *this;
	}

	phRigidBodyTet()
	{}

	phRigidBodyTet( Vector3::Vector3Param centroid )
	{
		// create the tetrahedron
		// tetrahedron can be embeded in cube so let's do that, then the centriod is the same as that of the cube

		CreateTetrahedron( centroid );

		InitForCollisions();

	}


	void CreateTetrahedron( Vector3::Vector3Param centroid )
	{
	 
		m_Vert[0].Add( Vector3(1, 1, 1), centroid ); 
		m_Vert[1].Add( Vector3(-1, -1, 1), centroid ); 
		m_Vert[2].Add( Vector3(-1, 1, -1), centroid ); 
		m_Vert[3].Add( Vector3(1, -1, -1), centroid ); 

		m_VertSpacing2.Set(2.82842712f); 

	}


	void RespondToPush( Vector3::Vector3Param point, Vector3::Vector3Param n, Vector3::Vector3Param depth )
	{
		// calculate tetrahedral barycentric coords of collision point

		Vector3 Pp0, P10, P20, P30;
		Pp0.Subtract( point, m_Vert[0] );
		P10.Subtract( m_Vert[1], m_Vert[0] );
		P20.Subtract( m_Vert[2], m_Vert[0] );
		P30.Subtract( m_Vert[3], m_Vert[0] );

		// finally a real use for Xenon's fast dot product!
		Vector3 P1010 = P10.DotV(P10);
		Vector3 P2010 = P20.DotV(P10);
		Vector3 P3010 = P30.DotV(P10);
		Vector3 P2020 = P20.DotV(P20);
		Vector3 P3020 = P30.DotV(P20);
		Vector3 P3030 = P30.DotV(P30);

		Vector3 Pp010 = Pp0.DotV(P10);
		Vector3 Pp020 = Pp0.DotV(P20);
		Vector3 Pp030 = Pp0.DotV(P30);

		//!me oh this is teh sux0rz, write a custom in-place invert3x3 using the P#0P#0 results
		Matrix34 baryizer;
		baryizer.Set( 
			P1010.x, P2010.y, P3010.z,
			P2010.x, P2020.y, P3020.z,
			P3010.x, P3020.y, P3030.z
			);

		baryizer.Inverse3x3();

		Vector3 baryb( Pp010.x, Pp020.y, Pp030.z );
		Vector3 bary3;
		baryizer.Transform3x3( baryb, bary3 );

		Vector4 bary( bary3.x, bary3.y, bary3.z, 1.0f - ( bary3.x + bary3.y + bary3.z ) );

		// clamp to 

		// now we have the barycentric coords.... this tells us what proportion of the push to apply to each tetrahedral vertex

		Vector4 lambda = bary.Mag2V();
		lambda.Invert();
		lambda.Multiply( depth );

		bary.Multiply( lambda );

		Vector4 d0, d1, d2, d3;
		d0.SplatX(bary);
		d1.SplatY(bary);
		d2.SplatZ(bary);
		d3.SplatW(bary);

		m_Vert[0].AddScaled( n, Vector3(d0) );
		m_Vert[1].AddScaled( n, Vector3(d1) );
		m_Vert[2].AddScaled( n, Vector3(d2) );
		m_Vert[3].AddScaled( n, Vector3(d3) );

	}

	void IntegratePushes()
	{

		// figure out velocity change


		//	delta = x2-x1;
		//	delta*=restlength*restlength/(delta*delta+restlength*restlength)-0.5;
		//	x1 += delta;
		//	x2 -= delta;

		int iIter;
		const int nIter = 3;
		for( iIter = 0; iIter < nIter; iIter++ ) 
		{

			Vector3 e01, e12, e23, e30;
			Vector3 e01d, e12d, e23d, e30d;

			e01.Subtract( m_Vert[1], m_Vert[0] );
			e12.Subtract( m_Vert[2], m_Vert[1] );
			e23.Subtract( m_Vert[3], m_Vert[2] );
			e30.Subtract( m_Vert[0], m_Vert[3] );

			e01d = e01.Mag2V();
			e12d = e12.Mag2V();
			e23d = e23.Mag2V();
			e30d = e30.Mag2V();

			e01d.Add( m_VertSpacing2 );
			e12d.Add( m_VertSpacing2 );
			e23d.Add( m_VertSpacing2 );
			e30d.Add( m_VertSpacing2 );

			e01d.Invert();
			e12d.Invert();
			e23d.Invert();
			e30d.Invert();

			e01d.Multiply( m_VertSpacing2 );
			e12d.Multiply( m_VertSpacing2 );
			e23d.Multiply( m_VertSpacing2 );
			e30d.Multiply( m_VertSpacing2 );

			e01d.Subtract( VEC3_HALF );
			e12d.Subtract( VEC3_HALF );
			e23d.Subtract( VEC3_HALF );
			e30d.Subtract( VEC3_HALF );

			Vector3 d01, d12, d23, d30;

			d01.Multiply( e01, e01d );
			d12.Multiply( e12, e12d );
			d23.Multiply( e23, e23d );
			d30.Multiply( e30, e30d );

			m_Vert[0].Add( d01 );
			m_Vert[1].Add( d12 );
			m_Vert[2].Add( d23 );
			m_Vert[3].Add( d30 );
			m_Vert[0].Subtract( d30 );
			m_Vert[1].Subtract( d01 );
			m_Vert[2].Subtract( d12 );
			m_Vert[3].Subtract( d23 );

		}

	} 

	void GetVelocityChange( Vector3 &linear, Vector3 &angular )
	{
		Vector3 centroid, prevCentroid;
		Vector3 vec3_quarter( 0.25f, 0.25f, 0.25f ); 

		centroid.AddScaled( VEC3_ZERO, m_Vert[0], vec3_quarter );
		centroid.AddScaled( m_Vert[1], vec3_quarter );
		centroid.AddScaled( m_Vert[2], vec3_quarter );
		centroid.AddScaled( m_Vert[3], vec3_quarter );

		prevCentroid.AddScaled( VEC3_ZERO, m_PrevVert[0], vec3_quarter );
		prevCentroid.AddScaled( m_PrevVert[1], vec3_quarter );
		prevCentroid.AddScaled( m_PrevVert[2], vec3_quarter );
		prevCentroid.AddScaled( m_PrevVert[3], vec3_quarter );

		linear.Subtract( centroid, prevCentroid );

		// find angle and axis we rotated around
		Vector3 arm, prevArm;
		arm.Subtract( m_Vert[0], centroid );
		prevArm.Subtract( m_PrevVert[0], prevCentroid );

		Vector3 axis;
		angular.Zero();
		if( prevArm.IsNotEqual( arm ) )
		{
			axis.Cross( prevArm, arm );
			axis.NormalizeFast();

			prevArm.NormalizeFast();
			Vector3 altitude, run;
			run.DotV( arm, prevArm );
			altitude.Subtract( arm, run );
			altitude = altitude.MagFastV();

			//!me performance problem, shoudl use approx and vector version
			Vector3 angle;
			angle.Set( atan2f( run.x, altitude.y ) );

			angular.Multiply( axis, angle );
		}
	}


	void InitForCollisions()
	{
		m_PrevVert[0] = m_Vert[0];
		m_PrevVert[1] = m_Vert[1];
		m_PrevVert[2] = m_Vert[2];
		m_PrevVert[3] = m_Vert[3];
	}

	Vector3 m_VertSpacing2;

	Vector3 m_Vert[4];
	Vector3 m_PrevVert[4];

};

}

#endif

