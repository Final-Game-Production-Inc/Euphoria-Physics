#include "ClothUpdater.h"
#include "Cloth.h"
#include "ClothConstraintGraph.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

CClothUpdater::CClothUpdater()
{
	Init();
}

CClothUpdater::~CClothUpdater()
{
	Shutdown();
}

void CClothUpdater::SetSolverMaxNumIterations(const int iMaxIter) 
{
	m_iMaxNumSolverIterations=iMaxIter;
}

void CClothUpdater::SetSolverTolerance(const float fTolerance) 
{
	m_fSolverTolerance=fTolerance;
}

void CClothUpdater::Init()
{
	//Zero the vectors and floats for the particle states.
	for(int i=0;i<MAX_NUM_CLOTH_PARTICLES;i++)
	{
		m_avForceTimesInvMasses[i].Zero();
		m_afInvMasses[i]=0;
		m_afDampingRateTimesInvMasses[i]=0;
	}

	for(int i=0;i<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS;i++)
	{
		m_sepConstraintData.m_afStrengthA[i]=0;
		m_sepConstraintData.m_afStrengthB[i]=0;
		m_aiActiveSepConstraintIndices[i]=0;
	}
}

void CClothUpdater::Shutdown()
{
}

void CClothUpdater::UpdateBallisticPhase(CCloth& cloth, const float fTimeStep)
{
	//Update ballistic phase.
	CClothParticles& particles=cloth.GetParticles();
	CClothPendants& pendants=cloth.GetPendants();

	//Record the inverse mass of each particle.
	int i;
	for(i=0;i<particles.GetNumParticles();i++)
	{
		m_afInvMasses[i]=particles.GetInverseMass(i);
	}
	//Set all particles attached to pendants to have infinite mass.
	for(i=0;i<pendants.GetNumPendants();i++)
	{
		const int iParticleId=pendants.GetParticleId(i);
		Assertf(iParticleId<MAX_NUM_CLOTH_PARTICLES, "Out of bounds");
		m_afInvMasses[iParticleId]=0;
	}
	//Set the force of gravity to act on all the particles.
	//The ballistic update equation contains a term F*dt*dt/M so where F=Mg so its definitely
	//a good optimisation to store just g for finite mass particles (and zero for infinite mass particles).
	//The ballistic update equation stores another term dampingRate*dt/M to model damping.
	//We're going to keep dampingRate/M constant (and zero for infinite mass particles).  This seems like 
	//a good place to store these values.
	Vector3 vGravityAccel(0,0,-10.0f);
	for(i=0;i<particles.GetNumParticles();i++)
	{
		//If particle has finite mass set F/M = g and dampingRate*dt/M = const*dt
		//If particle has infinite mass then set F/M=0 and dampingRate*dt/M=0.
		if(m_afInvMasses[i]>0)
		{
			//Particle has finite mass.
			m_avForceTimesInvMasses[i]=vGravityAccel;
			m_afDampingRateTimesInvMasses[i]=particles.GetDampingRate(i);
		}
		else
		{
			//Particle has infinite mass.
			m_avForceTimesInvMasses[i].Zero();
			m_afDampingRateTimesInvMasses[i]=0;
		}
	}

	//Update and compute the position of the pendants.
	Vector3 vPendantPositions[MAX_NUM_CLOTH_PENDANTS];
	pendants.Update(fTimeStep,vPendantPositions,MAX_NUM_CLOTH_PENDANTS);
	//Update the position of the particles that are pinned to pendants with the position of the pendant.
	//Update the previous position too just in case the pendant gets removed and the velocity of the 
	//particle becomes important again.
	for(i=0;i<pendants.GetNumPendants();i++)
	{
		const int iParticleId=pendants.GetParticleId(i);
		particles.SetPositionPrev(iParticleId,vPendantPositions[i]);
		particles.SetPosition(iParticleId,vPendantPositions[i]);
	}

	//Apply force for one timestep.
	const float dtSquared=fTimeStep*fTimeStep;
	for(i=0;i<particles.GetNumParticles();i++)
	{
		//Get everything we need to advance the ith particle by one timestep.
		const Vector3& vCurrentPos=particles.GetPosition(i);
		const Vector3& vPrevPos=particles.GetPositionPrev(i);
		const Vector3& vForceTimesInvMass=m_avForceTimesInvMasses[i];
		const float fDampingRateTimesInvMass=m_afDampingRateTimesInvMasses[i];

		//Now advance the particle using verlet integration.
		//With xCurr/vCurr/FCurr denoting the current position/velocity/force and 
		//xPrev denoting the previous position, M denoting the particle
		//mass, alpha denoting the particle damping, and dt the time-step we have
		//xNew = xCurr + vCurr*dt + (FCurr - alpha*vCurr)*dt*dt/Mass
		//Just to avoid confusion FCurr is the applied force and -alpha*vCurr represents the damping force.
		//Substituting vCurr = (xCurr-xPrev)/dt yields
		//xNew = xCurr + (xCurr-xPrev)*(1-alpha*dt/M) + F*dt*dt/M
		//We've already noted that F/M is just gravitational acceleration and that we're keeping alpha/M constant.
		Vector3 w=vCurrentPos-vPrevPos;
		w*=(1-fDampingRateTimesInvMass*fTimeStep);
		Vector3 vNewPos=vCurrentPos;					//vNew=vCurrent
		vNewPos+=w;										//vNew=vCurrent + (vCurrent-vPrev)*(1-dampingRate*dt/mass)
		vNewPos+=vForceTimesInvMass*dtSquared;			//vNew=vCurrent + (vCurrent-vPrev)*(1-dampingRate*dt/mass) + (force/mass)*dt*dt

		//Set the current position and the previous position.
		particles.SetPositionPrev(i,vCurrentPos);
		particles.SetPosition(i,vNewPos);
	}
}

void CClothUpdater::UpdateSprings(CCloth& cloth, const float UNUSED_PARAM(fTimeStep))
{
	CClothParticles& particles=cloth.GetParticles();
	CClothSprings& springs=cloth.GetSprings();

	//Treat benders springs as though they can resolve the spring displacement in a single update.
	//We're going to iterate across all springs in the order dictated by the graph.
	m_graph.Reset();
	int i;
	for(i=0;i<springs.GetNumSprings();i++)
	{
		const int iParticleA=springs.GetParticleIdA(i);
		const int iParticleB=springs.GetParticleIdB(i);
		const float fInvMassA=m_afInvMasses[iParticleA];
		const float fInvMassB=m_afInvMasses[iParticleB];
		Assert(fInvMassA!=0 || fInvMassB!=0);
		m_graph.AddConstraint(i,iParticleA,iParticleB,fInvMassA,fInvMassB);
	}
	if(springs.GetNumSprings()>0)
	{
		m_graph.ComputeGraph();
	}

	//Propagate along the graph applying the springs at each level of the graph.
	const float fSpringStiffness=0.95f;
	for(i=0;i<m_graph.GetNumLevels();i++)
	{
		int j;
		for(j=0;j<m_graph.GetNumConstraints(i);j++)
		{
			int iSpringIndex;
			int iParticleA,iParticleB;
			float fInverseMassAMultiplier,fInverseMassBMultiplier;
			m_graph.GetConstraintBodies(i,j,iSpringIndex,iParticleA,iParticleB,fInverseMassAMultiplier,fInverseMassBMultiplier);
			Assert(iSpringIndex>=0 && iSpringIndex<springs.GetNumSprings());
			Assert(springs.GetParticleIdA(iSpringIndex)==iParticleA);
			Assert(springs.GetParticleIdB(iSpringIndex)==iParticleB);

			//Get the spring data.
			const float fSpringAllowedLengthDifference=springs.GetAllowedLengthDifference(iSpringIndex);
			const float fSpringRestLength=springs.GetRestLength(iSpringIndex);

			//Work out the distance between the two particles.
			const Vector3& vA=particles.GetPosition(iParticleA);
			const Vector3& vB=particles.GetPosition(iParticleB);
			Vector3 vBA=vB-vA;
			const float fLength=vBA.Mag();
			vBA*=(1.0f/fLength);
			//vBA.NormalizeFast();

			//Work out the distance from the rest length.
			//Test if spring has compressed by a threshold amount.
			Assert(fSpringAllowedLengthDifference>=0);
			float fDelta=fLength-fSpringRestLength;
			if(fDelta<-fSpringAllowedLengthDifference)
			{
				//Account for the allowed threshold compression.
				fDelta+=fSpringAllowedLengthDifference;

				//Work out the bias of the two particles.
				Assert(m_afInvMasses[iParticleA]!=0 || m_afInvMasses[iParticleB]!=0);
				const float fInvMassA=fInverseMassAMultiplier*m_afInvMasses[iParticleA];
				const float fInvMassB=fInverseMassBMultiplier*m_afInvMasses[iParticleB];
				const float fa=fInvMassA/(fInvMassA+fInvMassB);
				const float fb=1.0f-fa;

				//Work out how far to move the two particles.
				const float fStrength=fSpringStiffness;
				Assert(fStrength<=1.0f);
				Vector3 vMoveA=vBA*(fDelta*fa*fStrength);
				Vector3 vMoveB=vBA*(fDelta*fb*fStrength);

				//Work out the new particle positions.
				Vector3 vNewPosA=vA+vMoveA;
				Vector3 vNewPosB=vB-vMoveB;

				//Set the particle positions.
				particles.SetPosition(iParticleA,vNewPosA);
				particles.SetPosition(iParticleB,vNewPosB);
			}
		}
	}
}

void CClothUpdater::UpdateBallisticPhaseV(CCloth& cloth, const float fTimeStep)
{
	//Update ballistic phase.
	CClothParticles& particles=cloth.GetParticles();
	CClothPendants& pendants=cloth.GetPendants();
	
	//Record the inverse mass of each particle.
	int i;
	for(i=0;i<particles.GetNumParticles();i++)
	{
		m_afInvMasses[i]=particles.GetInverseMass(i);
	}
	//Set all particles attached to pendants to have infinite mass.
	for(i=0;i<pendants.GetNumPendants();i++)
	{
		const int iParticleId=pendants.GetParticleId(i);
		Assertf(iParticleId<MAX_NUM_CLOTH_PARTICLES, "Out of bounds");
		m_afInvMasses[iParticleId]=0;
	}
	//Set the force of gravity to act on all the particles.
	//The ballistic update equation contains a term F*dt*dt/M so where F=Mg so its definitely
	//a good optimisation to store just g for finite mass particles (and zero for infinite mass particles).
	//The ballistic update equation stores another term dampingRate*dt/M to model damping.
	//We're going to keep dampingRate/M constant (and zero for infinite mass particles).  This seems like 
	//a good place to store these values.
	Vector3 vGravityAccel(0,0,-10.0f);
	for(i=0;i<particles.GetNumParticles();i++)
	{
		//If particle has finite mass set gravityForce=gravityAccel*fMass
		//If particle has infinite mass then set gravityForce=0 just for safety.
		if(m_afInvMasses[i]>0)
		{
			//Particle has finite mass.
			m_avForceTimesInvMasses[i]=vGravityAccel;
			m_afDampingRateTimesInvMasses[i]=particles.GetDampingRate(i);
		}
		else
		{
			//Particle has infinite mass.
			m_avForceTimesInvMasses[i].Zero();
			m_afDampingRateTimesInvMasses[i]=0.0f;
		}
	}

	//Update and compute the position of the pendants.
	Vector3 vPendantPositions[MAX_NUM_CLOTH_PENDANTS];
	pendants.Update(fTimeStep,vPendantPositions,MAX_NUM_CLOTH_PENDANTS);
	//Update the position of the particles that are pinned to pendants with the position of the pendant.
	//Update the previous position too just in case the pendant gets removed and the velocity of the 
	//particle becomes important again.
	for(i=0;i<pendants.GetNumPendants();i++)
	{
		const int iParticleId=pendants.GetParticleId(i);
		particles.SetPositionPrev(iParticleId,vPendantPositions[i]);
		particles.SetPosition(iParticleId,vPendantPositions[i]);
	}

	//Apply force for one timestep.
	const ScalarV dt=ScalarVFromF32(fTimeStep);
	const ScalarV fDtSquared=dt*dt;
	for(i=0;i<particles.GetNumParticles();i++)
	{
		//Now advance the particle using verlet integration.
		//With xCurr/vCurr/FCurr denoting the current position/velocity/force and 
		//xPrev denoting the previous position, M denoting the particle
		//mass, alpha denoting the particle damping, and dt the time-step we have
		//xNew = xCurr + vCurr*dt + (FCurr - alpha*vCurr)*dt*dt/Mass
		//Just to avoid confusion FCurr is the applied force and -alpha*vCurr represents the damping force.
		//Substituting vCurr = (xCurr-xPrev)/dt yields
		//xNew = xCurr + (xCurr-xPrev)*(1-alpha*dt/M) + F*dt*dt/M
		//We've already noted that F/M is just gravitational acceleration and that we're keeping alpha/M constant.

		//Get everything we need to advance the ith particle by one timestep.
		const Vec3V vCurrentPos=RCC_VEC3V(particles.GetPosition(i));
		const Vec3V vPrevPos=RCC_VEC3V(particles.GetPositionPrev(i));
		const Vec3V vForceTimesInvMass=RCC_VEC3V(m_avForceTimesInvMasses[i]);
		const ScalarV fDampingRateTimesInvMass=ScalarVFromF32(m_afDampingRateTimesInvMasses[i]);
		//Compute a few intermediate terms.
		const ScalarV fOneMinusDampingRateTimesDtTimesInvMass=ScalarV(V_ONE)-fDampingRateTimesInvMass*dt;
		const Vec3V vForcetimesDtSquaredTimesInvMass=vForceTimesInvMass*fDtSquared;
		Vec3V w=vCurrentPos-vPrevPos;
		w*=fOneMinusDampingRateTimesDtTimesInvMass;
		//Advance the position.
		Vec3V vNewPos=vCurrentPos;
		vNewPos+=w;
		vNewPos+=vForcetimesDtSquaredTimesInvMass;		

		//Set the current position and the previous position.
		particles.SetPositionPrev(i,RCC_VECTOR3(vCurrentPos));
		particles.SetPosition(i,RCC_VECTOR3(vNewPos));
	}
}

void CClothUpdater::UpdateSpringsV(CCloth& cloth, const float UNUSED_PARAM(fTimeStep))
{
	const ScalarV zero=ScalarV(V_ZERO);
	const ScalarV one=ScalarV(V_ONE);

	CClothParticles& particles=cloth.GetParticles();
	CClothSprings& springs=cloth.GetSprings();
	
	//Treat benders springs as though they can resolve the spring displacement in a single update.
	//We're going to iterate across all springs in the order dictated by the graph.
	m_graph.Reset();
	int i;
	for(i=0;i<springs.GetNumSprings();i++)
	{
		const int iParticleA=springs.GetParticleIdA(i);
		const int iParticleB=springs.GetParticleIdB(i);
		const float fInvMassA=m_afInvMasses[iParticleA];
		const float fInvMassB=m_afInvMasses[iParticleB];
		Assert(fInvMassA!=0 || fInvMassB!=0);
		m_graph.AddConstraint(i,iParticleA,iParticleB,fInvMassA,fInvMassB);
	}
	if(springs.GetNumSprings()>0)
	{
		m_graph.ComputeGraph();
	}

	//Propagate along the graph applying the springs at each level of the graph.
	const ScalarV fSpringStiffness=ScalarVFromF32(0.95f);
	for(i=0;i<m_graph.GetNumLevels();i++)
	{
		int j;
		for(j=0;j<m_graph.GetNumConstraints(i);j++)
		{
			int iSpringIndex;
			int iParticleA,iParticleB;
			float fInverseMassAMultiplier,fInverseMassBMultiplier;
			m_graph.GetConstraintBodies(i,j,iSpringIndex,iParticleA,iParticleB,fInverseMassAMultiplier,fInverseMassBMultiplier);
			Assert(iSpringIndex>=0 && iSpringIndex<springs.GetNumSprings());
			Assert(springs.GetParticleIdA(iSpringIndex)==iParticleA);
			Assert(springs.GetParticleIdB(iSpringIndex)==iParticleB);

			//Get the spring data.
			const ScalarV fSpringAllowedLengthDifference=ScalarVFromF32(springs.GetAllowedLengthDifference(iSpringIndex));
			const ScalarV fSpringRestLength=ScalarVFromF32(springs.GetRestLength(iSpringIndex));

			//Work out the distance between the two particles.
			const Vec3V vA=RCC_VEC3V(particles.GetPosition(iParticleA));
			const Vec3V vB=RCC_VEC3V(particles.GetPosition(iParticleB));
			Vec3V vBA=Subtract(vB,vA);
			const ScalarV fLength=Mag(vBA);
			vBA*=(one/fLength);
			//NormalizeFast(vBA);

			//Work out the distance from the rest length.
			//Test if spring has compressed by a threshold amount.
			const ScalarV fDelta=Min(fLength-fSpringRestLength+fSpringAllowedLengthDifference,zero);

			//Work out the bias of the two particles.
			const ScalarV fInvMassA=ScalarVFromF32(m_afInvMasses[iParticleA])*ScalarVFromF32(fInverseMassAMultiplier);
			const ScalarV fInvMassB=ScalarVFromF32(m_afInvMasses[iParticleB])*ScalarVFromF32(fInverseMassBMultiplier);
			const ScalarV fa=fInvMassA/(fInvMassA+fInvMassB);
			const ScalarV fb=one-fa;

			//Work out how far to move the two particles.
			const Vec3V vMoveA=vBA*(fDelta*fa*fSpringStiffness);
			const Vec3V vMoveB=vBA*(fDelta*fb*fSpringStiffness);

			//Work out the new particle positions.
			const Vec3V vNewPosA=vA+vMoveA;
			const Vec3V vNewPosB=vB-vMoveB;

			//Set the particle positions.
			particles.SetPosition(iParticleA,RCC_VECTOR3(vNewPosA));
			particles.SetPosition(iParticleB,RCC_VECTOR3(vNewPosB));
		}
	}
}


void CClothUpdater::HandleConstraintsAndContacts(CCloth& cloth, const float UNUSED_PARAM(dt))
{
	CClothParticles& particles=cloth.GetParticles();
	CClothSeparationConstraints& sepConstraints=cloth.GetSeparationConstraints();
	//CClothVolumeConstraints& volConstraints=cloth.GetVolumeConstraints();

	//Update the position of the particles that are pinned to contacts.
	//Apply a bit of extra contact damping in a direction tangential to the normal.
	int i;
	for(i=0;i<m_contacts.GetNumContacts();i++)
	{
		//Get the details of the ith contact.
		//The contact position is the particle pushed along the normal until it hits the contact surface.
		//We want to set the particle at this position but also to account for a bit of contact damping
		//tangential to the normal.
		//Only move the particle if it has finite mass.
		const int iParticleId=m_contacts.GetParticleId(i);
		const float fInvMass=m_afInvMasses[iParticleId];
		Assertf(fInvMass!=0, "Contact with infinite mass particle");
		if(fInvMass!=0)
		{
			//Contact data.
			const Vector3& vPos=m_contacts.GetPosition(i);
			const Vector3& vNormal=m_contacts.GetNormal(i);

			//Get the previous position of the particle and work out the velocity vector.
			const Vector3& vPosPrev=particles.GetPositionPrev(iParticleId);
			Vector3 vVel=vPos-vPosPrev;

			//Get the components in the normal and tangential to the normal.
			//vel=fn*N + ft*T (N is normal direction, T is tangential direction)
			//fn=vel.N so vel=(vel.N)*fn + ft*T and ft*T=vel-(vel.N)*N
			const float fn=vVel.Dot(vNormal);
			Vector3 vTangent=vVel-(vNormal*fn);
			//Multiply the tangential bit by (1-damping) to simulate a bit of contact friction.
			vTangent*=0.9f;
			//Now add the normal and tangential bits back together.
			vVel=vTangent+vNormal*fn;

			//Now set the new position of the particle
			Vector3 vNewPos=vPosPrev+vVel;
			particles.SetPosition(iParticleId,vNewPos);
		}
	}

	//Compute the number of active constraints and store them in an array.
	//Store the details of the active particle constraints in handy arrays to use later when 
	//handling the active constraints. When we handle the constraints 
	//we're going to work out the length of the array of (length-RestLength) and compare it 
	//with the length of the array of restLengths.  The ratio of these lengths will be our error measure.
	//Set errorResidual to be the sum of length-RestLength and 
	//set errorNormaliser to be the sum of the rest lengths.
	//We can then compute error=errorResidual/errorNormaliser and compare this to the solver tolerance.
	//To save some square roots we compare error^2 with errorResidual^2/errorNormaliser^2
	//To save an inverse we compute errorNormaliser^-2.
	//As we record the active constraints we might as well compute errorNormaliser^-2 as well.
	int iNumActiveSepConstraints=0;
	float fErrorNormaliserSquaredInverse=0;
	for(i=0;i<sepConstraints.GetNumConstraints();i++)
	{
		//Get everything we need.
		const int iParticleA=sepConstraints.GetParticleIndexA(i);
		const int iParticleB=sepConstraints.GetParticleIndexB(i);
		const float fInvMassA=m_afInvMasses[iParticleA];
		const float fInvMassB=m_afInvMasses[iParticleB];

		//Record the active constraints.
		if((fInvMassA+fInvMassB)!=0)
		{
			//Found another active constraint.
			m_aiActiveSepConstraintIndices[iNumActiveSepConstraints]=i;
			iNumActiveSepConstraints++;

			//Store the data.
			m_sepConstraintData.m_afStrengthA[i]=fInvMassA/(fInvMassA+fInvMassB);
			m_sepConstraintData.m_afStrengthB[i]=1.0f-m_sepConstraintData.m_afStrengthA[i];
			fErrorNormaliserSquaredInverse+=sepConstraints.GetRestLength(i);
		}
	}
	fErrorNormaliserSquaredInverse=1.0f/fErrorNormaliserSquaredInverse;

	//Iterate round a few times till the error is reduced to an acceptable level.
	//We'll compare the square of the error (errorSquared = errorResidualSquared * errorNormaliserInverseSquared)
	//with the square of the solver tolerance.
	const float fSolverTolerance=m_fSolverTolerance;
	const float fSolverToleranceSquared=fSolverTolerance*fSolverTolerance;
	float fErrorSquared=1.0f;
	Assert(fErrorSquared>fSolverToleranceSquared);
	int k=0;
	while(k<m_iMaxNumSolverIterations && fErrorSquared>fSolverToleranceSquared)
	{
		//One more iteration to update all the particle positions.
		int j;
		for(j=0;j<iNumActiveSepConstraints;j++)
		{
			const int iConstraintIndex=m_aiActiveSepConstraintIndices[j];
			Assert(iConstraintIndex>=0 && iConstraintIndex<sepConstraints.GetNumConstraints());
			const int iParticleA=sepConstraints.GetParticleIndexA(iConstraintIndex);
			const int iParticleB=sepConstraints.GetParticleIndexB(iConstraintIndex);
			Vector3 vA=particles.GetPosition(iParticleA);
			Vector3 vB=particles.GetPosition(iParticleB);
			Vector3 vDiff=vA-vB;
			const float fDist=vDiff.Mag();
			vDiff*=(1.0f/fDist);
			//vDiff.NormalizeFast();
			const float fRestLength=sepConstraints.GetRestLength(iConstraintIndex);
			const float fDelta=fDist-fRestLength;
			vA-=vDiff*(m_sepConstraintData.m_afStrengthA[iConstraintIndex]*fDelta);
			vB+=vDiff*(m_sepConstraintData.m_afStrengthB[iConstraintIndex]*fDelta);
			particles.SetPosition(iParticleA,vA);
			particles.SetPosition(iParticleB,vB);
		}

		//Recompute the error.
		fErrorSquared=0;
		for(j=0;j<iNumActiveSepConstraints;j++)
		{
			const int iConstraintIndex=m_aiActiveSepConstraintIndices[j];
			Assert(iConstraintIndex>=0 && iConstraintIndex<sepConstraints.GetNumConstraints());
			const int iParticleA=sepConstraints.GetParticleIndexA(iConstraintIndex);
			const int iParticleB=sepConstraints.GetParticleIndexB(iConstraintIndex);
			const Vector3& vA=particles.GetPosition(iParticleA);
			const Vector3& vB=particles.GetPosition(iParticleB);
			Vector3 vDiff=vA-vB;
			const float fDist=vDiff.Mag();
			const float fRestLength=sepConstraints.GetRestLength(iConstraintIndex);
			const float fDelta=fDist-fRestLength;
			fErrorSquared+=fDelta*fDelta;
		}
		fErrorSquared=fErrorSquared*fErrorNormaliserSquaredInverse;
		k++;
	}

	//Finish off by handling in an order given by a constraint graph.
	//This is an efficient way of rigidly enforcing the constraints.
	//Compute the constraint graph for the active constraints.
	m_graph.Reset();
	for(i=0;i<iNumActiveSepConstraints;i++)
	{
		const int iConstraintIndex=m_aiActiveSepConstraintIndices[i];
		Assert(iConstraintIndex>=0 && iConstraintIndex<sepConstraints.GetNumConstraints());
		const int iParticleA=sepConstraints.GetParticleIndexA(iConstraintIndex);
		const int iParticleB=sepConstraints.GetParticleIndexB(iConstraintIndex);
		const float fInvMassA=m_afInvMasses[iParticleA];
		const float fInvMassB=m_afInvMasses[iParticleB];
		m_graph.AddConstraint(iConstraintIndex,iParticleA,iParticleB,fInvMassA,fInvMassB);
	}
	if(iNumActiveSepConstraints>0)
	{
		m_graph.ComputeGraph();
	}

	//Propagate along the graph from the root to the tip.
	for(i=0;i<m_graph.GetNumLevels();i++)
	{ 
		//Work out all the particle constraints in the next level and store their details
		//in handy arrays. If no constraint in the current graph level has both particles 
		//to be treated with finite mass then we can guarantee that the current graph level
		//will be resolved in a single iteration. Find out if we can do this.
		bool bConstraintExistsWithTwoFiniteMassParticles=false;
		const int iNumConstraintsInLevel=m_graph.GetNumConstraints(i);
		int j;
		for(j=0;j<iNumConstraintsInLevel;j++)
		{
			//Get the next constraint in the current level.
			int iConstraintIndex;
			int iParticleA,iParticleB;
			//bool bA,bB;
			//m_graph.GetConstraintBodies(i,j,iConstraintIndex,iParticleA,iParticleB,bA,bB);
			float fInvMassAMultipier,fInvMassBMultiplier;
			m_graph.GetConstraintBodies(i,j,iConstraintIndex,iParticleA,iParticleB,fInvMassAMultipier,fInvMassBMultiplier);

			Assert(iConstraintIndex>=0 && iConstraintIndex<sepConstraints.GetNumConstraints());
			Assert(sepConstraints.GetParticleIndexA(iConstraintIndex)==iParticleA);
			Assert(sepConstraints.GetParticleIndexB(iConstraintIndex)==iParticleB);

			//Get the inverse masses based on bA and bB.
			const float fInvMassA=fInvMassAMultipier*m_afInvMasses[iParticleA];
			const float fInvMassB=fInvMassBMultiplier*m_afInvMasses[iParticleB];
			Assert(fInvMassA!=0 || fInvMassB!=0);
			if(fInvMassA!=0 && fInvMassB!=0)
			{
				bConstraintExistsWithTwoFiniteMassParticles=true;
			}

			//Store the data we'll need in handy arrays.
			m_sepConstraintData.m_afStrengthA[j]=fInvMassA/(fInvMassA+fInvMassB);
			m_sepConstraintData.m_afStrengthB[j]=1.0f-m_sepConstraintData.m_afStrengthA[j];
		}

		//Now iterate around each level until all the particles in each level are in the correct position.
		fErrorSquared=1.0f;
		k=0;
		while(k<m_iMaxNumSolverIterations && fErrorSquared>fSolverToleranceSquared)
		{
			//One more iteration to compute new positions.
			int j;
			for(j=0;j<m_graph.GetNumConstraints(i);j++)
			{	
				const int iConstraintIndex=m_graph.GetConstraintIndex(i,j);
				const int iParticleA=sepConstraints.GetParticleIndexA(iConstraintIndex);
				const int iParticleB=sepConstraints.GetParticleIndexB(iConstraintIndex);
				Vector3 vA=particles.GetPosition(iParticleA);
				Vector3 vB=particles.GetPosition(iParticleB);
				Vector3 vDiff=vA-vB;
				const float fDist=vDiff.Mag();
				vDiff*=(1.0f/fDist);
				//vDiff.NormalizeFast();
				const float fRestLength=sepConstraints.GetRestLength(iConstraintIndex);
				const float fDelta=fDist-fRestLength;
				vA-=vDiff*(m_sepConstraintData.m_afStrengthA[j]*fDelta);
				vB+=vDiff*(m_sepConstraintData.m_afStrengthB[j]*fDelta);
				particles.SetPosition(iParticleA,vA);
				particles.SetPosition(iParticleB,vB);
			}
			//Recompute the error if there is a possibility that the error is non-zero.
			fErrorSquared=0;
			if(bConstraintExistsWithTwoFiniteMassParticles)
			{
				for(j=0;j<iNumConstraintsInLevel;j++)
				{
					const int iConstraintIndex=m_graph.GetConstraintIndex(i,j);
					const int iParticleA=sepConstraints.GetParticleIndexA(iConstraintIndex);
					const int iParticleB=sepConstraints.GetParticleIndexB(iConstraintIndex);
					const Vector3& vA=particles.GetPosition(iParticleA);
					const Vector3& vB=particles.GetPosition(iParticleB);
					Vector3 vDiff=vA-vB;
					const float fDist=vDiff.Mag();
					const float fRestLength=sepConstraints.GetRestLength(iConstraintIndex);
					const float fDelta=fDist-fRestLength;
					fErrorSquared+=fDelta*fDelta;
				}
			}
			fErrorSquared=fErrorSquared*fErrorNormaliserSquaredInverse;
			k++;
		}
	}
}

void CClothUpdater::HandleConstraintsAndContactsV(CCloth& cloth, const float UNUSED_PARAM(dt))
{
	const ScalarV zero=ScalarV(V_ZERO);
	const ScalarV one=ScalarV(V_ONE);
	const ScalarV zeroPointNine=ScalarVFromF32(0.9f);

	CClothParticles& particles=cloth.GetParticles();
	CClothSeparationConstraints& sepConstraints=cloth.GetSeparationConstraints();
	//CClothVolumeConstraints& volConstraints=cloth.GetVolumeConstraints();

	//Update the position of the particles that are pinned to contacts.
	//Apply a bit of extra contact damping in a direction tangential to the normal.
	int i;
	for(i=0;i<m_contacts.GetNumContacts();i++)
	{
		//Get the details of the ith contact.
		//The contact position is the particle pushed along the normal until it hits the contact surface.
		//We want to set the particle at this position but also to account for a bit of contact damping
		//tangential to the normal.
		//Only move the particle if it has finite mass.
		const int iParticleId=m_contacts.GetParticleId(i);
		Assertf(m_afInvMasses[iParticleId]!=0, "Contact with infinite mass particle");

		//Contact data.
		const Vec3V vPos=RCC_VEC3V(m_contacts.GetPosition(i));
		const Vec3V vNormal=RCC_VEC3V(m_contacts.GetNormal(i));

		//Get the previous position of the particle and work out the velocity vector.
		const Vec3V vPosPrev=RCC_VEC3V(particles.GetPositionPrev(iParticleId));
		Vec3V vVel=vPos-vPosPrev;

		//Get the components in the normal and tangential to the normal.
		//vel=fn*N + ft*T (N is normal direction, T is tangential direction)
		//fn=vel.N so vel=(vel.N)*fn + ft*T and ft*T=vel-(vel.N)*N
		const ScalarV fn=Dot(vVel,vNormal);
		Vec3V vTangent=vVel-(vNormal*fn);
		//Multiply the tangential bit by (1-damping) to simulate a bit of contact friction.
		vTangent*=zeroPointNine;
		//Now add the normal and tangential bits back together.
		vVel=vTangent+vNormal*fn;

		//Now set the new position of the particle
		const Vec3V vNewPos=vPosPrev+vVel;
		particles.SetPosition(iParticleId,RCC_VECTOR3(vNewPos));
	}

	//Compute the number of active constraints and store them in an array.
	//Store the details of the active particle constraints in handy arrays to use later when 
	//handling the active constraints. When we handle the constraints 
	//we're going to work out the length of the array of (length-RestLength) and compare it 
	//with the length of the array of restLengths.  The ratio of these lengths will be our error measure.
	//Set errorResidual to be the sum of length-RestLength and 
	//set errorNormaliser to be the sum of the rest lengths.
	//We can then compute error=errorResidual/errorNormaliser and compare this to the solver tolerance.
	//To save some square roots we compare error^2 with errorResidual^2/errorNormaliser^2
	//To save an inverse we compute errorNormaliser^-2.
	//As we record the active constraints we might as well compute errorNormaliser^-2 as well.
	int iNumActiveSepConstraints=0;
	ScalarV fErrorNormaliserSquaredInverse=zero;
	for(i=0;i<sepConstraints.GetNumConstraints();i++)
	{
		//Get everything we need.
		const int iParticleA=sepConstraints.GetParticleIndexA(i);
		const int iParticleB=sepConstraints.GetParticleIndexB(i);
		const float fInvMassA=m_afInvMasses[iParticleA];
		const float fInvMassB=m_afInvMasses[iParticleB];

		//Record the active constraints.
		if((fInvMassA+fInvMassB)!=0)
		{
			//Found another active constraint.
			m_aiActiveSepConstraintIndices[iNumActiveSepConstraints]=i;
			iNumActiveSepConstraints++;

			//Store the data.
			m_sepConstraintData.m_afStrengthA[i]=fInvMassA/(fInvMassA+fInvMassB);
			m_sepConstraintData.m_afStrengthB[i]=1.0f-m_sepConstraintData.m_afStrengthA[i];
			const float fRestLength=sepConstraints.GetRestLength(i);
			fErrorNormaliserSquaredInverse+=ScalarVFromF32(fRestLength)*ScalarVFromF32(fRestLength);
		}
	}
	/*
	int iNumActiveVolConstraints=0;
	for(i=0;i<volConstraints.GetNumConstraints();i++)
	{
		const int iParticleA=volConstraints.GetParticleIndexA(i);
		const int iParticleB=volConstraints.GetParticleIndexB(i);
		const int iParticleC=volConstraints.GetParticleIndexC(i);
		const int iParticleD=volConstraints.GetParticleIndexD(i);
		const float fInvMassA=m_afInvMasses[iParticleA];
		const float fInvMassB=m_afInvMasses[iParticleB];
		const float fInvMassC=m_afInvMasses[iParticleC];
		const float fInvMassD=m_afInvMasses[iParticleD];
		if((fInvMassA+fInvMassB+fInvMassC+fInvMassD)!=0)
		{
			//Found another active constraint.
			m_aiActiveVolConstraintIndices[iNumActiveVolConstraints]=i;
			iNumActiveVolConstraints++;
		}
		const float fRestVolume=volConstraints.GetRestVolume(i);
		fErrorNormaliserSquaredInverse+=ScalarVFromF32(fRestVolume)*ScalarVFromF32(fRestVolume);
	}
	*/
	fErrorNormaliserSquaredInverse=one/fErrorNormaliserSquaredInverse;

	//Iterate round a few times till the error is reduced to an acceptable level.
	//We'll compare the square of the error (errorSquared = errorResidualSquared * errorNormaliserInverseSquared)
	//with the square of the solver tolerance.
	const ScalarV fSolverTolerance=ScalarVFromF32(m_fSolverTolerance);
	const ScalarV fSolverToleranceSquared=fSolverTolerance*fSolverTolerance;
	ScalarV fErrorSquared=one;
	int k=0;
	while(k<m_iMaxNumSolverIterations && fErrorSquared.Getf()>fSolverToleranceSquared.Getf())
	{
		//One more iteration to update all the particle positions.
		int j;
		for(j=0;j<iNumActiveSepConstraints;j++)
		{
			const int iConstraintIndex=m_aiActiveSepConstraintIndices[j];
			Assert(iConstraintIndex>=0 && iConstraintIndex<sepConstraints.GetNumConstraints());
			const int iParticleA=sepConstraints.GetParticleIndexA(iConstraintIndex);
			const int iParticleB=sepConstraints.GetParticleIndexB(iConstraintIndex);
			Vec3V vA=RCC_VEC3V(particles.GetPosition(iParticleA));
			Vec3V vB=RCC_VEC3V(particles.GetPosition(iParticleB));
			Vec3V vDiff=vA-vB;
			const ScalarV fDist=Mag(vDiff);
			vDiff*=one/fDist;
			//NormalizeFast(vDiff);
			const ScalarV fRestLength=ScalarVFromF32(sepConstraints.GetRestLength(iConstraintIndex));
			const ScalarV fDelta=fDist-fRestLength;
			const ScalarV fStrengthA=ScalarVFromF32(m_sepConstraintData.m_afStrengthA[iConstraintIndex]);
			const ScalarV fStrengthB=ScalarVFromF32(m_sepConstraintData.m_afStrengthB[iConstraintIndex]);
			const ScalarV fStrengthATimesDelta=fStrengthA*fDelta;
			const ScalarV fStrengthBTimesDelta=fStrengthB*fDelta;
			Vec3V vDiffA=vDiff;
			vDiffA*=fStrengthATimesDelta;
			Vec3V vDiffB=vDiff;
			vDiffB*=fStrengthBTimesDelta;
			vA-=vDiffA;
			vB+=vDiffB;
			particles.SetPosition(iParticleA,RCC_VECTOR3(vA));
			particles.SetPosition(iParticleB,RCC_VECTOR3(vB));
		}

		/*
		Vec3V vOne(1.0f,1.0f,1.0f);
		for(j=0;j<iNumActiveVolConstraints;j++)
		{
			const int iConstraintIndex=m_aiActiveVolConstraintIndices[j];
			Assert(iConstraintIndex>=0 && iConstraintIndex<volConstraints.GetNumConstraints());
			const int iParticleA=volConstraints.GetParticleIndexA(iConstraintIndex);
			const int iParticleB=volConstraints.GetParticleIndexB(iConstraintIndex);
			const int iParticleC=volConstraints.GetParticleIndexC(iConstraintIndex);
			const int iParticleD=volConstraints.GetParticleIndexD(iConstraintIndex);
			Vec3V v1=RCC_VEC3V(particles.GetPosition(iParticleA));
			Vec3V v2=RCC_VEC3V(particles.GetPosition(iParticleB));
			Vec3V v3=RCC_VEC3V(particles.GetPosition(iParticleC));
			Vec3V v4=RCC_VEC3V(particles.GetPosition(iParticleD));

			Vec3V v21=Subtract(v2,v1);
			Vec3V v31=Subtract(v3,v1);
			Vec3V v41=Subtract(v4,v1);
			Vec3V v32=Subtract(v3,v2);

			ScalarV vDeriv1;
			{
				Vec3V v32Cross1=Cross(v32,vOne);
				ScalarV a=Dot(v32Cross1,v41);
				Vec3V v32CrossV1=Cross(v32,v1);
				ScalarV b=Dot(v32CrossV1,vOne);
				vDeriv1=Subtract(a,b);
			}

			ScalarV vDeriv2;
			{
				Vec3V oneCrossV31=Cross(vOne,v31);
				vDeriv2=Dot(oneCrossV31,v41);
			}

			ScalarV vDeriv3;
			{
				Vec3V v21Cross1=Cross(v21,vOne);
				vDeriv3=Dot(v21Cross1,v41);
			}

			ScalarV vDeriv4;
			{
				Vec3V v21Crossv31=Cross(v21,v31);
				vDeriv3=Dot(v21Crossv31,vOne);
			}
		}
		*/

		//Recompute the error.
		fErrorSquared=ScalarV(V_ZERO);
		for(j=0;j<iNumActiveSepConstraints;j++)
		{
			const int iConstraintIndex=m_aiActiveSepConstraintIndices[j];
			Assert(iConstraintIndex>=0 && iConstraintIndex<sepConstraints.GetNumConstraints());
			const int iParticleA=sepConstraints.GetParticleIndexA(iConstraintIndex);
			const int iParticleB=sepConstraints.GetParticleIndexB(iConstraintIndex);
			const Vec3V vA=RCC_VEC3V(particles.GetPosition(iParticleA));
			const Vec3V vB=RCC_VEC3V(particles.GetPosition(iParticleB));
			const Vec3V vDiff=vA-vB;
			const ScalarV fDist=Mag(vDiff);
			const ScalarV fRestLength=ScalarVFromF32(sepConstraints.GetRestLength(iConstraintIndex));
			const ScalarV fDelta=fDist-fRestLength;
			fErrorSquared+=fDelta*fDelta;
		}
		/*
		for(j=0;j<iNumActiveVolConstraints;j++)
		{
			const int iConstraintIndex=m_aiActiveVolConstraintIndices[j];
			Assert(iConstraintIndex>=0 && iConstraintIndex<volConstraints.GetNumConstraints());
			const int iParticleA=volConstraints.GetParticleIndexA(iConstraintIndex);
			const int iParticleB=volConstraints.GetParticleIndexB(iConstraintIndex);
			const int iParticleC=volConstraints.GetParticleIndexC(iConstraintIndex);
			const int iParticleD=volConstraints.GetParticleIndexD(iConstraintIndex);
			const Vec3V vA=RCC_VEC3V(particles.GetPosition(iParticleA));
			const Vec3V vB=RCC_VEC3V(particles.GetPosition(iParticleB));
			const Vec3V vC=RCC_VEC3V(particles.GetPosition(iParticleC));
			const Vec3V vD=RCC_VEC3V(particles.GetPosition(iParticleD));
			const ScalarV fVol=CClothVolumeConstraints::ComputeVolume(vA,vB,vC,vD);
			const ScalarV fRestVolume=ScalarVFromF32(volConstraints.GetRestVolume(iConstraintIndex));
			const ScalarV fDelta=Subtract(fVol,fRestVolume);
			fErrorSquared+=fDelta*fDelta;
		}
		*/
		fErrorSquared=fErrorSquared*fErrorNormaliserSquaredInverse;
		k++;
	}

	//Finish off by handling in an order given by a constraint graph.
	//This is an efficient way of rigidly enforcing the constraints.
	//Compute the constraint graph for the active constraints.
	m_graph.Reset();
	for(i=0;i<iNumActiveSepConstraints;i++)
	{
		const int iConstraintIndex=m_aiActiveSepConstraintIndices[i];
		Assert(iConstraintIndex>=0 && iConstraintIndex<sepConstraints.GetNumConstraints());
		const int iParticleA=sepConstraints.GetParticleIndexA(iConstraintIndex);
		const int iParticleB=sepConstraints.GetParticleIndexB(iConstraintIndex);
		const float fInvMassA=m_afInvMasses[iParticleA];
		const float fInvMassB=m_afInvMasses[iParticleB];
		m_graph.AddConstraint(iConstraintIndex,iParticleA,iParticleB,fInvMassA,fInvMassB);
	}
	if(iNumActiveSepConstraints>0)
	{
		m_graph.ComputeGraph();
	}

	//Propagate along the graph from the root to the tip.
	for(i=0;i<m_graph.GetNumLevels();i++)
	{ 
		//Work out all the particle constraints in the next level and store their details
		//in handy arrays. If no constraint in the current graph level has both particles 
		//to be treated with finite mass then we can guarantee that the current graph level
		//will be resolved in a single iteration. Find out if we can do this.
		bool bConstraintExistsWithTwoFiniteMassParticles=false;
		const int iNumConstraintsInLevel=m_graph.GetNumConstraints(i);
		int j;
		for(j=0;j<iNumConstraintsInLevel;j++)
		{
			//Get the next constraint in the current level.
			int iConstraintIndex;
			int iParticleA,iParticleB;
			//bool bA,bB;
			//m_graph.GetConstraintBodies(i,j,iConstraintIndex,iParticleA,iParticleB,bA,bB);
			float fInvMassAMultipier,fInvMassBMultiplier;
			m_graph.GetConstraintBodies(i,j,iConstraintIndex,iParticleA,iParticleB,fInvMassAMultipier,fInvMassBMultiplier);

			Assert(iConstraintIndex>=0 && iConstraintIndex<sepConstraints.GetNumConstraints());
			Assert(sepConstraints.GetParticleIndexA(iConstraintIndex)==iParticleA);
			Assert(sepConstraints.GetParticleIndexB(iConstraintIndex)==iParticleB);

			//Get the inverse masses based on bA and bB.
			const float fInvMassA=fInvMassAMultipier*m_afInvMasses[iParticleA];
			const float fInvMassB=fInvMassBMultiplier*m_afInvMasses[iParticleB];
			Assert(fInvMassA!=0 || fInvMassB!=0);
			if(fInvMassA!=0 && fInvMassB!=0)
			{
				bConstraintExistsWithTwoFiniteMassParticles=true;
			}

			//Store the data we'll need in handy arrays.
			m_sepConstraintData.m_afStrengthA[j]=fInvMassA/(fInvMassA+fInvMassB);
			m_sepConstraintData.m_afStrengthB[j]=1.0f-m_sepConstraintData.m_afStrengthA[j];
		}

		//Now iterate around each level until all the particles in each level are in the correct position.
		fErrorSquared=one;
		k=0;
		while(k<m_iMaxNumSolverIterations && fErrorSquared.Getf()>fSolverToleranceSquared.Getf())
		{
			//One more iteration to compute new positions.
			int j;
			for(j=0;j<m_graph.GetNumConstraints(i);j++)
			{	
				const int iConstraintIndex=m_graph.GetConstraintIndex(i,j);
				const int iParticleA=sepConstraints.GetParticleIndexA(iConstraintIndex);
				const int iParticleB=sepConstraints.GetParticleIndexB(iConstraintIndex);
				Vec3V vA=RCC_VEC3V(particles.GetPosition(iParticleA));
				Vec3V vB=RCC_VEC3V(particles.GetPosition(iParticleB));
				Vec3V vDiff=vA-vB;
				const ScalarV fDist=Mag(vDiff);
				vDiff*=(one/fDist);
				//vDiff=NormalizeFast(vDiff);
				const ScalarV fRestLength=ScalarVFromF32(sepConstraints.GetRestLength(iConstraintIndex));
				const ScalarV fDelta=fDist-fRestLength;
				const ScalarV fStrengthA=ScalarVFromF32(m_sepConstraintData.m_afStrengthA[j]);
				const ScalarV fStrengthB=ScalarVFromF32(m_sepConstraintData.m_afStrengthB[j]);
				const ScalarV fStrengthATimesDelta=fStrengthA*fDelta;
				const ScalarV fStrengthBTimesDelta=fStrengthB*fDelta;
				Vec3V vDeltaA=vDiff;
				vDeltaA*=fStrengthATimesDelta;
				Vec3V vDeltaB=vDiff;
				vDeltaB*=fStrengthBTimesDelta;
				vA-=vDeltaA;
				vB+=vDeltaB;
				particles.SetPosition(iParticleA,RCC_VECTOR3(vA));
				particles.SetPosition(iParticleB,RCC_VECTOR3(vB));
			}
			//Recompute the error if there is a possibility that the error is non-zero.
			fErrorSquared=zero;
			if(bConstraintExistsWithTwoFiniteMassParticles)
			{
				for(j=0;j<iNumConstraintsInLevel;j++)
				{
					const int iConstraintIndex=m_graph.GetConstraintIndex(i,j);
					const int iParticleA=sepConstraints.GetParticleIndexA(iConstraintIndex);
					const int iParticleB=sepConstraints.GetParticleIndexB(iConstraintIndex);
					const Vec3V vA=RCC_VEC3V(particles.GetPosition(iParticleA));
					const Vec3V vB=RCC_VEC3V(particles.GetPosition(iParticleB));
					const Vec3V vDiff=vA-vB;
					const ScalarV fDist=Mag(vDiff);
					const ScalarV fRestLength=ScalarVFromF32(sepConstraints.GetRestLength(iConstraintIndex));
					const ScalarV fDelta=fDist-fRestLength;
					fErrorSquared+=fDelta*fDelta;
				}
			}
			fErrorSquared=fErrorSquared*fErrorNormaliserSquaredInverse;
			k++;
		}
	}
}



bool CClothUpdater::TestIsMoving(const CCloth& cloth, const float dt) const
{
	bool bIsMoving=false;
	const float fMinMotion=10.0f*dt*dt*0.5f;
	const CClothParticles& particles=cloth.GetParticles();
	for(int i=0;i<particles.GetNumParticles();i++)
	{
		const Vector3& vPos=particles.GetPosition(i);
		const Vector3& vPosPrev=particles.GetPositionPrev(i);
		Vector3 w=vPos-vPosPrev;
		if(fabsf(w.x)>fMinMotion || fabsf(w.y>fMinMotion) || fabsf(w.z)>fMinMotion)
		{
			bIsMoving=true;
			break;
		}
	}
	return bIsMoving;
}


/*
#if USE_MATRIX_SOLVER
void CClothUpdater::HandleConstraintsAndContacts(CCloth& cloth, const float UNUSED_PARAM(dt))
{
CClothParticles& particles=cloth.GetParticles();
CClothConstraints& constraints=cloth.GetConstraints();

//Update the position of the particles that are pinned to contacts.
//Apply a bit of extra contact damping in a direction tangential to the normal.
int i;
for(i=0;i<m_contacts.GetNumContacts();i++)
{
//Get the details of the ith contact.
//The contact position is the particle pushed along the normal until it hits the contact surface.
//We want to set the particle at this position but also to account for a bit of contact damping
//tangential to the normal.
//Only move the particle if it has finite mass.
const int iParticleId=m_contacts.GetParticleId(i);
const float fInvMass=m_afInvMasses[iParticleId];
if(fInvMass!=0)
{
const Vector3& vPos=m_contacts.GetPosition(i);
const Vector3& vNormal=m_contacts.GetNormal(i);

//Get the previous position of the particle and work out the velocity vector.
const Vector3& vPosPrev=particles.GetPositionPrev(iParticleId);
Vector3 vVel=vPos-vPosPrev;

//Get the components in the normal and tangential to the normal.
//vel=fn*N + ft*T (N is normal direction, T is tangential direction)
//fn=vel.N so vel=(vel.N)*fn + ft*T and ft*T=vel-(vel.N)*N
const float fn=vVel.Dot(vNormal);
Vector3 vTangent=vVel-(vNormal*fn);
//Multiply the tangential bit by (1-damping) to simulate a bit of contact friction.
vTangent*=0.9f;
//Now add the normal and tangential bits back together.
vVel=vTangent+vNormal*fn;

//Now set the new position of the particle
Vector3 vNewPos=vPosPrev+vVel;
particles.SetPosition(iParticleId,vNewPos);
}
}

//Compute constraint strengths and the number of active constraints.
int iNumActiveConstraints=0;
int aiActiveConstraintIndices[MAX_NUM_CLOTH_CONSTRAINTS];
for(i=0;i<constraints.GetNumConstraints();i++)
{
//Get everything we need.
const int iParticleA=constraints.GetParticleIndexA(i);
const int iParticleB=constraints.GetParticleIndexB(i);
const float fInvMassA=m_afInvMasses[iParticleA];
const float fInvMassB=m_afInvMasses[iParticleB];

//Record the active constraints.
if((fInvMassA+fInvMassB)!=0)
{
//Found another active constraint.
aiActiveConstraintIndices[iNumActiveConstraints]=i;
iNumActiveConstraints++;

//Compute the strengths.
m_fStrengthA[i]=fInvMassA/(fInvMassA+fInvMassB);
m_fStrengthB[i]=fInvMassB/(fInvMassA+fInvMassB);
}
}

//Compute direction and length of each active constraint.
for(i=0;i<iNumActiveConstraints;i++)
{
const int iConstraintId=aiActiveConstraintIndices[i];
//Compute the direction and length.
const int iParticleA=constraints.GetParticleIndexA(iConstraintId);
const int iParticleB=constraints.GetParticleIndexB(iConstraintId);
const Vector3& vPosA=particles.GetPosition(iParticleA);
const Vector3& vPosB=particles.GetPosition(iParticleB);
Vector3 vConstraintDir=vPosA-vPosB;
const float fLength=vConstraintDir.Mag();
vConstraintDir.Normalize();
m_avConstraintDirs[iConstraintId]=vConstraintDir;
m_afLengths[iConstraintId]=fLength;
}

//Set up the matrix problem A*alpha=b for all active constraints.
m_A.Clear();
m_A.Resize(iNumActiveConstraints,iNumActiveConstraints);
for(i=0;i<iNumActiveConstraints;i++)
{
const int iConstraintIndex=aiActiveConstraintIndices[i];
//Get everything we need for the ith constraint.
const int iParticleA=constraints.GetParticleIndexA(iConstraintIndex);
const int iParticleB=constraints.GetParticleIndexB(iConstraintIndex);
const Vector3& vDirI=m_avConstraintDirs[iConstraintIndex];

//Compute b[i].
const float fLength=m_afLengths[iConstraintIndex];
const float fRestLength=constraints.GetRestLength(iConstraintIndex);
m_b[i]=fRestLength-fLength;

//Now compute the jth row of the matrix.
int j;
for(j=0;j<iNumActiveConstraints;j++)
{
const int jConstraintIndex=aiActiveConstraintIndices[j];
//Get everything we need for the jth constraint.
const int jParticleA=constraints.GetParticleIndexA(jConstraintIndex);
const int jParticleB=constraints.GetParticleIndexB(jConstraintIndex);
const Vector3& vDirJ=m_avConstraintDirs[jConstraintIndex];

//Now work out how strongly the jth constraint is applied to particle A of the ith constraint.
float fCouplingA=0;
if(iParticleA==jParticleA)
{
//Constraint j will be applied +vely to body A of constraint i.
fCouplingA=m_fStrengthA[jConstraintIndex];
}
else if(iParticleA==jParticleB)
{
//Constraint j will be applied -vely to body A of constraint i
fCouplingA=-m_fStrengthB[jConstraintIndex];
}

//Now work out how strongly the jth constraint is applied to particle B of the ith constraint.
float fCouplingB=0;
if(iParticleB==jParticleA)
{
//Constraint j will be applied +vely to body B of constraint i.
fCouplingB=m_fStrengthA[jConstraintIndex];
}
else if(iParticleB==jParticleB)
{
//Constraint j will be applied -vely to body B of constraint i
fCouplingB=-m_fStrengthB[jConstraintIndex];
}

//Now combine the coupling of the jth constraint to the two bodies of the ith constraint
//into a single term.
const float fCoupling=(fCouplingA-fCouplingB)*vDirI.Dot(vDirJ);
m_A.SetElement(i,j,fCoupling);
}//j

//We ought to zero the solution just before we recompute.
//Does this help the performance of the solver or should we just pass the previous solution as an 
//initial guess for the next solution?
m_alpha[i]=0;
}//i

//Now solve the matrix problem A*alpha=b using the chosen solver type.
switch(m_iSolverType)
{
case SOLVER_TYPE_BICG:
m_solverBiCG.Solve(m_A,m_b,iNumActiveConstraints,m_alpha,iNumActiveConstraints);
break;
case SOLVER_TYPE_QMR:
m_solverQMR.Solve(m_A,m_b,iNumActiveConstraints,m_alpha,iNumActiveConstraints);
break;
default:
Assertf(false, "Unknown solver type");
m_solverBiCG.Solve(m_A,m_b,iNumActiveConstraints,m_alpha,iNumActiveConstraints);
break;
}

//Now apply the constraint solutions.
for(i=0;i<iNumActiveConstraints;i++)
{
//Get the index of the ith active constraint.
const int iConstraintIndex=aiActiveConstraintIndices[i];
Assertf((m_fStrengthA[iConstraintIndex] + 
m_fStrengthB[iConstraintIndex])!=0, 
"Constraint has both bodies infinite mass");

//Get the constraint direction.
const Vector3& vConstraintdir=m_avConstraintDirs[iConstraintIndex];
//Get the solution to A*alpha=b
const float fAlpha=m_alpha[i];

//Compute the new position of particle A
{
const float fStrengthA=m_fStrengthA[iConstraintIndex];
if(fStrengthA!=0)
{
const int iParticleA=constraints.GetParticleIndexA(iConstraintIndex);
const Vector3& vPosA=particles.GetPosition(iParticleA);
Vector3 vNewPosA=vPosA+vConstraintdir*fStrengthA*fAlpha;
particles.SetPosition(iParticleA,vNewPosA);
}
}

//Compute the new position of particleB
{
const float fStrengthB=m_fStrengthB[iConstraintIndex];
if(fStrengthB!=0)
{
const int iParticleB=constraints.GetParticleIndexB(iConstraintIndex);
const Vector3& vPosB=particles.GetPosition(iParticleB);
Vector3 vNewPosB=vPosB-vConstraintdir*fStrengthB*fAlpha;
particles.SetPosition(iParticleB,vNewPosB);
}
}
}//i
}
#endif
*/
#endif // NORTH_CLOTHS
