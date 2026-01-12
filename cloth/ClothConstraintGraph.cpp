#include "ClothMgr.h"
#include "ClothConstraintGraph.h"
#include "ClothConstraints.h"

#if NORTH_CLOTHS

CLOTH_OPTIMISATIONS()

void CClothConstraintGraph::ComputeGraph()
{
	int i,j,k;

	Assert(m_iNumConstraints!=0);

	//Array of sorted bodies.
	//These will be arranged in levels starting from the root of the graph.
	int aiSortedParticles[MAX_NUM_CLOTH_PARTICLES];
	//Num bodies in each level of graph and number of body levels.
	int aiNumBodiesInLevel[MAX_NUM_CLOTH_PARTICLES];
	int iCurrentBodyLevel=0;
	int iNumSortedParticles=0;

	//Array of sorted constraints.
	//These will be arranged in levels starting from constraints that touch the root of the graph.
	const CClothConstraintGraphElement* apSortedConstraints[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS];
	//Num constraints in each level of graph and number of constraint levels.
	int aiNumConstraintsInLevels[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS];
	int iCurrentConstraintLevel=0;
	int iNumSortedConstraints=0;

	//Array of constraints still to be sorted.
	//We'll sort from the unsorted constraints to the sorted array until 
	//there are none left to sort.
	CClothConstraintGraphElement* apUnsortedConstraints[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS];
	int iNumUnsortedConstraints=m_iNumConstraints;
	for(i=0;i<m_iNumConstraints;i++)
	{
		apUnsortedConstraints[i]=&m_aConstraints[i];
	}

	//Start by computing the particles that form the roots of the graph.
	//Store the number of particles at the root level.
	iNumSortedParticles=0;
	ComputeRootParticles(apUnsortedConstraints, iNumUnsortedConstraints, aiSortedParticles, iNumSortedParticles);
	aiNumBodiesInLevel[iCurrentBodyLevel]=iNumSortedParticles;

	//Test if there were any root rigid bodies.
	//If there were no roots then just add all the constraints into a single level.
	if(0==iNumSortedParticles)
	{
		m_iNumLevels=1;
		m_aiLevelOffsets[0]=0;
		m_aiLevelSizes[0]=m_iNumConstraints;
		for(i=0;i<m_iNumConstraints;i++)
		{
			m_apOrderedConstraints[i]=&m_aConstraints[i];
		}
		return;
	}

	//Now look for bodies that occupy the next level by looking for couplings to bodies in the current level.
	//Work out the start and end of the bodies in the array of sorted bodies.
	int iSortedBodyStart=0;
	int iSortedBodyEnd=0;
	while(iNumSortedConstraints<m_iNumConstraints)
	{
		//Increment the current body level and set the number of bodies in the new body level to zero.
		iCurrentBodyLevel++;
		Assert(iCurrentBodyLevel<MAX_NUM_CLOTH_PARTICLES);
		aiNumBodiesInLevel[iCurrentBodyLevel]=0;

		//Set the number of constraints in the current constraint level to zero.
		//(We'll increment the constraint level later because our notation is such  
		//that the ith constraint level couples the ith and i+1th body levels.
		Assert(iCurrentConstraintLevel<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS);
		aiNumConstraintsInLevels[iCurrentConstraintLevel]=0;

		//Work out where in the sorted body array we will find the previous layer of sorted bodies.
		iSortedBodyStart=iSortedBodyEnd;
		iSortedBodyEnd=iSortedBodyStart+aiNumBodiesInLevel[iCurrentBodyLevel-1];

		//Go through all unsorted constraints that involve a sorted body and set those constraints to 
		//treat those sorted bodies as infinite mass.
		for(i=0;i<iNumUnsortedConstraints;i++)
		{
			CClothConstraintGraphElement* pConstraint=apUnsortedConstraints[i];
			for(j=iSortedBodyStart;j<iSortedBodyEnd;j++)
			{
				const int iParticleId=aiSortedParticles[j];
				if(pConstraint->CouplesToParticle(iParticleId))
				{
					pConstraint->SetInfiniteMass(iParticleId,true);
				}
			}
		}

		//Look for bodies that are coupled to sorted bodies by unsorted constraints.
		//Store these bodies in the current body level and store the relevant
		//constraints in the current constraint level.	
		for(i=iSortedBodyStart;i<iSortedBodyEnd;i++)
		{
			//Get the next sorted body.
			const int iSortedParticle=aiSortedParticles[i];
			Assert(iSortedParticle>=0 || 0==iCurrentConstraintLevel);

			//Look for unsorted constraints that involve the sorted body under scrutiny.
			for(j=0;j<iNumUnsortedConstraints;j++)
			{
				CClothConstraintGraphElement* pUnsortedConstraint=apUnsortedConstraints[j];
				if(pUnsortedConstraint && pUnsortedConstraint->CouplesToParticle(iSortedParticle))
				{
					//The coupling of this constraint is being handled so we don't need to bother
					//with this constraint again.
					apUnsortedConstraints[j]=0;

					//Add the unsorted constraint to the list of sorted constraints.
					Assert(iNumSortedConstraints<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS);
					apSortedConstraints[iNumSortedConstraints]=pUnsortedConstraint;
					iNumSortedConstraints++;
					aiNumConstraintsInLevels[iCurrentConstraintLevel]++;

					//Add the unsorted body to the list of sorted bodies if it hasn't already been added.
					int aiNewSortedParticles[3]={-1,-1,-1};
					pUnsortedConstraint->GetOtherParticles(iSortedParticle,aiNewSortedParticles[0],aiNewSortedParticles[1],aiNewSortedParticles[2]);
					for(int m=0;m<3;m++)
					{
						if(aiNewSortedParticles[m]>=0)
						{
							//Test if the new body to be sorted has been added to the list.
							bool bAlreadyAdded=false;
							for(k=iSortedBodyEnd;k<iSortedBodyEnd+aiNumBodiesInLevel[iCurrentBodyLevel];k++)
							{
								if(aiSortedParticles[k]==aiNewSortedParticles[m])
								{
									bAlreadyAdded=true;
									break;
								}
							}
							//If the body hasn't been added then add it to the list of sorted bodies.
							if(!bAlreadyAdded)
							{
								Assert(iNumSortedParticles<MAX_NUM_CLOTH_PARTICLES);
								aiSortedParticles[iNumSortedParticles]=aiNewSortedParticles[m];
								iNumSortedParticles++;
								aiNumBodiesInLevel[iCurrentBodyLevel]++;
							}
						}//pNewSortedBody
					}
				}//jth unsorted constraint couples to ith sorted body
			}//j
		}//i

		//Before we do this take a note of where the recently added bodies start in the list of sorted bodies.
		int iNewSortedBodyStart=iSortedBodyEnd;

		//Now look for unsorted constraints that couple together bodies that we just sorted.
		int iNewSortedBodyEnd=iNewSortedBodyStart+aiNumBodiesInLevel[iCurrentBodyLevel];
		for(i=0;i<iNumUnsortedConstraints;i++)
		{
			const CClothConstraintGraphElement* pConstraint=apUnsortedConstraints[i];
			bool bFoundBodyA=false;
			bool bFoundBodyB=false;
			if(pConstraint)
			{
				for(j=iNewSortedBodyStart;j<iNewSortedBodyEnd;j++)
				{
					if(pConstraint->GetParticleA()==aiSortedParticles[j])
					{
						bFoundBodyA=true;
					}
					else if(pConstraint->GetParticleB()==aiSortedParticles[j])
					{
						bFoundBodyB=true;
					}
					if(bFoundBodyA && bFoundBodyB)
					{
						break;
					}
				}
			}

			if(bFoundBodyA && bFoundBodyB)
			{
				apUnsortedConstraints[i]=0;
				Assert(iNumSortedConstraints<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS);
				apSortedConstraints[iNumSortedConstraints]=pConstraint;
				iNumSortedConstraints++;
				aiNumConstraintsInLevels[iCurrentConstraintLevel]++;
			}
		}


		//Now look for unsorted bodies that couple to the bodies that we just sorted above (coupled through unsorted
		//constraints).  First identity unsorted bodies that are coupled to sorted bodies by unsorted constraints
		//and then identity those bodies that couple to two (or more) different sorted bodies.  These bodies represent
		//a loop in the current level so we'll have to add the relevant bodies and constraints to the sorted lists.  
		//We need to keep doing this until we don't find any more bodies to sort (when we have completed all the 
		//inter-level loops).
		bool bFoundExtraConstraints=true;
		while(bFoundExtraConstraints)
		{
			bFoundExtraConstraints=false;

			//Compute a list of unique unsorted bodies that are coupled by unsorted constraints 
			//to bodies that we just sorted.
			int iNumLocalUnsortedBodies=0;
			int aiLocalUnsortedParticles[MAX_NUM_CLOTH_PARTICLES];
			//Keep a track of the bodies that were just sorted.
			int iNewSortedBodyEnd=iNewSortedBodyStart+aiNumBodiesInLevel[iCurrentBodyLevel];
			//Iterate over all the unsorted constraints.
			for(i=0;i<iNumUnsortedConstraints;i++)
			{
				const CClothConstraintGraphElement* pUnsortedConstraint=apUnsortedConstraints[i];
				if(pUnsortedConstraint)
				{
					//Iterate over all the sorted bodies in the current body layer.
					for(j=iNewSortedBodyStart;j<iNewSortedBodyEnd;j++)
					{
						const int iSortedParticle=aiSortedParticles[j];
						Assert(iSortedParticle>=0);
						//Test if the sorted body couples to the unsorted constraint.
						if(pUnsortedConstraint->CouplesToParticle(iSortedParticle))
						{
							//Found a constraint that couples a sorted body to an unsorted body.
							//Get the unsorted body and add it to a list of unsorted bodies if necessary.
							int iLocalUnsortedParticles[3]={-1,-1,-1};
							pUnsortedConstraint->GetOtherParticles(iSortedParticle,iLocalUnsortedParticles[0],iLocalUnsortedParticles[1],iLocalUnsortedParticles[2]);
							for(int m=0;m<3;m++)
							{
								if(iLocalUnsortedParticles[m]>=0)
								{
									bool bAlreadyAdded=false;
									for(k=0;k<iNumLocalUnsortedBodies;k++)
									{
										if(iLocalUnsortedParticles[m]==aiLocalUnsortedParticles[k])
										{
											bAlreadyAdded=true;
											break;
										}
									}
									if(!bAlreadyAdded)
									{
										Assert(iNumLocalUnsortedBodies<MAX_NUM_CLOTH_PARTICLES);
										aiLocalUnsortedParticles[iNumLocalUnsortedBodies]=iLocalUnsortedParticles[m];
										iNumLocalUnsortedBodies++;
									}
								}
							}
						}//pUnsortedConstraint->CouplesToParticle(pSortedBody)
					}//j
				}//pUnsortedConstraint
			}//i

			//Now iterate over the unsorted bodies that couple to sorted bodies through unsorted constraints
			//and look for unsorted bodies that couples to TWO different sorted bodies by unsorted constraints.
			//These bodies need to be added to the list of sorted bodies and their constraints 
			//need to be added to the list of sorted constraints.
			for(i=0;i<iNumLocalUnsortedBodies;i++)
			{
				//Get the ith unsorted body that might need sorted if it couples to two sorted bodies through unsorted
				//constraints.
				const int iLocalUnsortedParticle=aiLocalUnsortedParticles[i];

				//Set the array element to zero.
				//If the body couples to two sorted bodies through unsorted constraints then we'll
				//refill the array element.
				aiLocalUnsortedParticles[i]=-1;

				int i1=-1;
				int i2=-1;
				for(j=0;j<iNumUnsortedConstraints;j++)
				{
					const CClothConstraintGraphElement* pUnsortedConstraint=apUnsortedConstraints[j];
					if(pUnsortedConstraint && pUnsortedConstraint->CouplesToParticle(iLocalUnsortedParticle))
					{
						//We've identified an unsorted constraint that couples to the unsorted body that might need sorted.
						//Check if the unsorted constraint couples to a sorted body.
						int aiOtherParticles[3]={-1,-1,-1};
						pUnsortedConstraint->GetOtherParticles(iLocalUnsortedParticle,aiOtherParticles[0],aiOtherParticles[1],aiOtherParticles[2]);
						for(int m=3;m<3;m++)
						{
							if(aiOtherParticles[m]>=0)
							{
								int iSortedParticle=-1;
								for(k=iNewSortedBodyStart;k<iNewSortedBodyEnd;k++)
								{
									if(aiSortedParticles[k]==aiOtherParticles[m])
									{
										iSortedParticle=aiSortedParticles[k];
										break;
									}
								}

								if(iSortedParticle>=0)
								{
									if(-1==i1)
									{
										i1=iSortedParticle;
									}
									else if(-1==i2 && iSortedParticle!=i1)
									{
										i2=iSortedParticle;
										break;
									}
								}
							}
						}
					}//pUnsortedConstraint
				}//j

				if(i1>=0 && i2>=0)
				{
					aiLocalUnsortedParticles[i]=iLocalUnsortedParticle;
				}
			}//i


			//We now know the unsorted bodies that couple to two sorted bodies by unsorted constraints.
			//Add all the unsorted bodies to the sorted list and add all the relevant unsorted constraints 
			//to the sorted list.
			int aiConstraintsToSort[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS];
			int iNumConstraintsToSort=0;
			for(i=0;i<iNumLocalUnsortedBodies;i++)
			{
				const int iParticleToSort=aiLocalUnsortedParticles[i];
				if(iParticleToSort>=0)
				{
					//Add the body to the sorted list.
					Assert(iNumSortedParticles<MAX_NUM_CLOTH_PARTICLES);
					aiSortedParticles[iNumSortedParticles]=iParticleToSort;
					iNumSortedParticles++;
					aiNumBodiesInLevel[iCurrentBodyLevel]++;
					bFoundExtraConstraints=true;

					//Add the relevant constraints.
					//Any constraint that couples the newly sorted body to an already sorted body is to be added
					//to the sorted constraint list.
					for(j=0;j<iNumUnsortedConstraints;j++)
					{
						const CClothConstraintGraphElement* pUnsortedConstraint=apUnsortedConstraints[j];
						if(pUnsortedConstraint && pUnsortedConstraint->CouplesToParticle(iParticleToSort))
						{
							//We've identified an unsorted constraint that couples to the unsorted body that might need sorted.
							//Check if the unsorted constraint couples to a sorted body.
							int aiOtherParticles[3]={-1,-1,-1};
							pUnsortedConstraint->GetOtherParticles(iParticleToSort,aiOtherParticles[0],aiOtherParticles[1],aiOtherParticles[2]);
							for(int m=0;m<3;m++)
							{
								if(aiOtherParticles[m]>=0)
								{
									int iSortedParticle=-1;
									for(k=iNewSortedBodyStart;k<(iNewSortedBodyStart+aiNumBodiesInLevel[iCurrentBodyLevel]);k++)
									{
										if(aiSortedParticles[k]==aiOtherParticles[m])
										{
											iSortedParticle=aiSortedParticles[k];
											break;
										}
									}

									if(iSortedParticle>=0)
									{
										bool bAlreadyAdded=false;
										for(k=0;k<iNumConstraintsToSort;k++)
										{
											if(aiConstraintsToSort[k]==j)
											{
												bAlreadyAdded=true;
											}
										}
										if(!bAlreadyAdded)
										{
											Assert(iNumConstraintsToSort<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS);
											aiConstraintsToSort[iNumConstraintsToSort]=j;
											iNumConstraintsToSort++;
										}
									}
								}
							}
						}
					}
				}
			}
			for(i=0;i<iNumConstraintsToSort;i++)
			{
				Assert(iNumSortedConstraints<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS);
				apSortedConstraints[iNumSortedConstraints]=apUnsortedConstraints[aiConstraintsToSort[i]];
				iNumSortedConstraints++;
				aiNumConstraintsInLevels[iCurrentConstraintLevel]++;
				apUnsortedConstraints[aiConstraintsToSort[i]]=0;
			}
		}
		Assertf(aiNumConstraintsInLevels[iCurrentConstraintLevel]!=0, "Empty graph layer");

		FillArrayHoles(apUnsortedConstraints,iNumUnsortedConstraints,iNumUnsortedConstraints);

		//Increment the constraint level
		iCurrentConstraintLevel++;

	}//iNumSortedConstraints<m_iNumConstraints

	//We've finished now.
	//Copy across constraints and set the flags for their infinite mass.
	int iOffset=0;
	for(i=0;i<iCurrentConstraintLevel;i++)
	{
		m_aiLevelOffsets[i]=iOffset;
		m_aiLevelSizes[i]=aiNumConstraintsInLevels[i];
		iOffset+=aiNumConstraintsInLevels[i];
	}
	m_iNumLevels=iCurrentConstraintLevel;
	for(i=0;i<iNumSortedConstraints;i++)
	{
		m_apOrderedConstraints[i]=apSortedConstraints[i];
	}
}

void CClothConstraintGraph::ComputeRootParticles(CClothConstraintGraphElement** papConstraints, int iNumConstraints, int* paiParticles, int& iNumparticles) const
{
	Assert(0==iNumparticles);

	//Start by identifying the particles in the zeroth level.
	for(int i=0;i<iNumConstraints;i++)
	{
		Assert(papConstraints[i]);
		//Test if the unsorted constraint is a root constraint.
		if(papConstraints[i]->IsRootConstraint())
		{
			//The unsorted constraint is a root constraint. 
			//Get the root body.
			const int iParticleId=papConstraints[i]->GetRootParticle();

			//Check if the root body has been added already.
			bool bAlreadyAdded=false;
			for(int j=0;j<iNumparticles;j++)
			{
				if(paiParticles[j]==iParticleId)
				{
					bAlreadyAdded=true;
					break;
				}
			}

			if(!bAlreadyAdded)
			{
				Assert(iNumparticles<MAX_NUM_CLOTH_PARTICLES);
				paiParticles[iNumparticles]=iParticleId;
				iNumparticles++;
			}
		}
	}
}

void CClothConstraintGraph::FillArrayHoles(CClothConstraintGraphElement** papConstraints, const int iArraySize, int& iNewArraySize) const
{
	//This isn't very efficient but it'll do for now.
	iNewArraySize=0;
	CClothConstraintGraphElement* apConstraints[MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS];
	for(int i=0;i<iArraySize;i++)
	{
		if(papConstraints[i])
		{
			Assert(iNewArraySize<MAX_NUM_CLOTH_SEPARATION_CONSTRAINTS);
			apConstraints[iNewArraySize]=papConstraints[i];
			iNewArraySize++;
		}
	}
	for(int i=0;i<iNewArraySize;i++)
	{
		papConstraints[i]=apConstraints[i];
	}
}

bool CClothConstraintGraph::IsLegal() const
{
	int i;
	for(i=0;i<GetNumLevels();i++)
	{
		int j;
		for(j=0;j<GetNumConstraints(i);j++)
		{
			int iConstraintIndex;
			int iParticleA,iParticleB;
			float fInverseMassAMultiplier,fInverseMassBMultiplier;
			GetConstraintBodies(i,j,iConstraintIndex,iParticleA,iParticleB,fInverseMassAMultiplier,fInverseMassBMultiplier);
			if(0.0f==fInverseMassAMultiplier && 0.0f==fInverseMassBMultiplier)
			{
				return false;
			}
		}
	}
	return true;
}

#endif //NORTH_CLOTHS
