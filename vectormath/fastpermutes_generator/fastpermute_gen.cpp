#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <bitset>
#include <cassert>
using namespace std;

// Two functions to find the best paths for.
#define PERMUTE_SOLUTION		0
#define PERMUTETWO_SOLUTION		(!PERMUTE_SOLUTION)

// Two platforms to find the best solutions for.
#define PS3						0
#define XBOX360					(!PS3)

// (NOTE THAT THE COMBINATION (PERMUTE_SOLUTION == 1 && XBOX360 == 1) is unnecessary!

#define MERGEXY		0
#define MERGEZW		1
#define VSLDOI		2
#define SPLATX		3
#define SPLATY		4
#define SPLATZ		5
#define SPLATW		6
#define VPERMWI		7
#define VRLIMI		8

#if PERMUTE_SOLUTION
#define NUM_RESULTS		(4*4*4*4)
#elif PERMUTETWO_SOLUTION
#define NUM_RESULTS		(8*8*8*8)
#endif





struct vec8
{
	unsigned operator [](int index)
	{
		return ((unsigned char*)(this))[index];
	}
	unsigned char x, y, z, w, a, b, c, d;
};

struct vec4
{
	vec4() {}
	vec4(unsigned char _x,unsigned char _y,unsigned char _z,unsigned char _w)
		: x(_x),y(_y),z(_z),w(_w) {}
	bool operator ==( vec4 const& v )
	{
		return (x==v.x && y == v.y && z == v.z && w == v.w);
	}
	unsigned operator [](int index)
	{
		return ((unsigned char*)(this))[index];
	}
	unsigned char x, y, z, w;
};

// ALLOWABLE PS3 FUNCTIONS //

vec4 MergeXY( vec4 a, vec4 b );
vec4 MergeZW( vec4 a, vec4 b );
vec4 vsldoi( vec4 a, vec4 b, unsigned int shift );
vec4 SplatX( vec4 in );
vec4 SplatY( vec4 in );
vec4 SplatZ( vec4 in );
vec4 SplatW( vec4 in );

// ALLOWABLE XBOX360 FUNCTIONS //

// <ALL PS3, AS WELL AS:>
vec4 vpermwi( vec4 in, unsigned immed );
vec4 vrlimi( vec4 in1, vec4 in2, unsigned wMask, unsigned rotateAmt );


//======================================================
//======================================================
//======================================================
struct Result
{
	Result() {}
	Result(vec4 v, short prevfunc, short prevarg1, short prevarg2, short previmmedarg1, short previmmedarg2 )
		:	vec(v),
			prevFunc(prevfunc),
			prevArg1Index(prevarg1),
			prevArg2Index(prevarg2),
			prevImmedArg1(previmmedarg1),
			prevImmedArg2(previmmedarg2)
	{}

	vec4 vec;					// The vector

	/// Some stuff to link us back to where we got this answer.
	short prevFunc;			// -1 if unused, >=0 if not.
	short prevArg1Index;	// NULL if this Result is one of the original inputs.
	short prevArg2Index;	// NULL if this Result is either one of the original inputs, or the prev result was from a function only taking 1 arg.
	short prevImmedArg1;	// -1 if unused, >=0 if not.
	short prevImmedArg2;	// -1 if unused, >=0 if not.
};
//======================================================
//======================================================
//======================================================

string GetVarName( vector<Result> const& resultsAllVec, unsigned index, bool clear = true ); // Recursive function that generates the body of a function from a Result.

bool TryAdd( bitset<NUM_RESULTS>& resultsAllBitvec, vector<Result>& resultsAllVec, Result const& newResult );	// Tries to add a vector to the final vector collection.
bool AreAllResultsFound( bitset<NUM_RESULTS> const& resultsAllBitvec );		// Checks whether all rResults[i].found's == true;

int GetPermuteIndex( vec4 const& v );									// Computes the index into resultsAll from the values in the current vector (if we're doing Permute()).
string GetPermuteString( int index );									// Computes the template-argument string for the Permute<...>()/PermuteTwo<...>() functions.

////////////////////////
// MAIN
////////////////////////
int main()
{
	// The original vector.
	vec4 origVec1(0, 1, 2, 3);
#if PERMUTETWO_SOLUTION
	vec4 origVec2(4, 5, 6, 7);
#endif

	// Collect the results, where each result has:
	//	(1) The resulting vector.
	//	(2) Information about the last func+args used to make the vector.
	vector<Result> resultsAll;
	bitset<NUM_RESULTS> resultsAllBitvec;
	
	resultsAll.resize( NUM_RESULTS );

	// A set containing all the vectors we have available to use during this iteration.
	vector<Result> vecSet;
	
	// Add the initial vector(s) we have to the set.
	Result res1 = Result( origVec1, -1, -1, -1, -1, -1 );
	vecSet.push_back( res1 );
#if PERMUTETWO_SOLUTION
	Result res2 = Result( origVec2, -1, -1, -1, -1, -1 );
	vecSet.push_back( res2 );
#endif

	// Add it(them) also to the result vector.
	int index = GetPermuteIndex(res1.vec);
	resultsAll[index] = res1;
	resultsAllBitvec[index] = true;
#if PERMUTETWO_SOLUTION
	int index2 = GetPermuteIndex(res2.vec);
	resultsAll[index2] = res2;
	resultsAllBitvec[index2] = true;
#endif

	// Loop while we haven't found all the results.
	bool bAddToNewResults;
	unsigned numCalls = 0;
	while( !AreAllResultsFound( resultsAllBitvec ) )
	{
		numCalls++;
		Result newResult;
		vector<Result> newResultsToAdd;

		cout << "Main while loop iteration #" << numCalls << endl;

		//
		// Try all functions, with all combinations of vecs in 'vecSet' as input parameters.
		//

		// Functions with one input parameter
		if( numCalls == 1 ) // Optimization: no need to try splatting after the first iteration.
		{
			for( unsigned i = 0; i < vecSet.size(); i++ )
			{
				newResult.vec = SplatX( vecSet[i].vec );
				newResult.prevFunc = SPLATX;
				newResult.prevArg1Index = GetPermuteIndex( vecSet[i].vec );
				newResult.prevArg2Index = -1;
				newResult.prevImmedArg1 = -1;
				newResult.prevImmedArg2 = -1;
				bAddToNewResults = TryAdd( resultsAllBitvec, resultsAll, newResult );
				if( bAddToNewResults )
				{
					newResultsToAdd.push_back( newResult );
				}
				
				newResult.vec = SplatY( vecSet[i].vec );
				newResult.prevFunc = SPLATY;
				newResult.prevArg1Index = GetPermuteIndex( vecSet[i].vec );
				newResult.prevArg2Index = -1;
				newResult.prevImmedArg1 = -1;
				newResult.prevImmedArg2 = -1;
				bAddToNewResults = TryAdd( resultsAllBitvec, resultsAll, newResult );
				if( bAddToNewResults )
				{
					newResultsToAdd.push_back( newResult );
				}

				newResult.vec = SplatZ( vecSet[i].vec );
				newResult.prevFunc = SPLATZ;
				newResult.prevArg1Index = GetPermuteIndex( vecSet[i].vec );
				newResult.prevArg2Index = -1;
				newResult.prevImmedArg1 = -1;
				newResult.prevImmedArg2 = -1;
				bAddToNewResults = TryAdd( resultsAllBitvec, resultsAll, newResult );
				if( bAddToNewResults )
				{
					newResultsToAdd.push_back( newResult );
				}

				newResult.vec = SplatW( vecSet[i].vec );
				newResult.prevFunc = SPLATW;
				newResult.prevArg1Index = GetPermuteIndex( vecSet[i].vec );
				newResult.prevArg2Index = -1;
				newResult.prevImmedArg1 = -1;
				newResult.prevImmedArg2 = -1;
				bAddToNewResults = TryAdd( resultsAllBitvec, resultsAll, newResult );
				if( bAddToNewResults )
				{
					newResultsToAdd.push_back( newResult );
				}
			}
		}

		cout << ">>> Merging..." << endl;

		// Functions with two input parameters
		for( unsigned i = 0; i < vecSet.size(); i++ )
		{
		for( unsigned j = 0; j < vecSet.size(); j++ )
		{
			newResult.vec = MergeXY( vecSet[i].vec, vecSet[j].vec );
			newResult.prevFunc = MERGEXY;
			newResult.prevArg1Index = GetPermuteIndex( vecSet[i].vec );
			newResult.prevArg2Index = GetPermuteIndex( vecSet[j].vec );
			newResult.prevImmedArg1 = -1;
			newResult.prevImmedArg2 = -1;
			bAddToNewResults = TryAdd( resultsAllBitvec, resultsAll, newResult );
			if( bAddToNewResults )
			{
				newResultsToAdd.push_back( newResult );
			}

			newResult.vec = MergeZW( vecSet[i].vec, vecSet[j].vec );
			newResult.prevFunc = MERGEZW;
			newResult.prevArg1Index = GetPermuteIndex( vecSet[i].vec );
			newResult.prevArg2Index = GetPermuteIndex( vecSet[j].vec );
			newResult.prevImmedArg1 = -1;
			newResult.prevImmedArg2 = -1;
			bAddToNewResults = TryAdd( resultsAllBitvec, resultsAll, newResult );
			if( bAddToNewResults )
			{
				newResultsToAdd.push_back( newResult );
			}
		}
		}

		cout << ">>> vsldoi'ing..." << endl;

		// Functions with two input parameters and an immediate argument.
		for( unsigned i = 0; i < vecSet.size(); i++ )
		{
		for( unsigned j = 0; j < vecSet.size(); j++ )
		{
		for( unsigned k = 0; k <= 4; k++ )
		{
			newResult.vec = vsldoi( vecSet[i].vec, vecSet[j].vec, k );
			newResult.prevFunc = VSLDOI;
			newResult.prevArg1Index = GetPermuteIndex( vecSet[i].vec );
			newResult.prevArg2Index = GetPermuteIndex( vecSet[j].vec );
			newResult.prevImmedArg1 = k;
			newResult.prevImmedArg2 = -1;
			bAddToNewResults = TryAdd( resultsAllBitvec, resultsAll, newResult );
			if( bAddToNewResults )
			{
				newResultsToAdd.push_back( newResult );
			}
		}
		}
		}

#if XBOX360
		cout << ">>> vpermwi'ing..." << endl;

		// Function with one input parameter and an immediate argument.
		for( unsigned i = 0; i < vecSet.size(); i++ )
		{
		for( unsigned k = 0; k < 256; k++ )
		{
			newResult.vec = vpermwi( vecSet[i].vec, k );
			newResult.prevFunc = VPERMWI;
			newResult.prevArg1Index = GetPermuteIndex( vecSet[i].vec );
			newResult.prevArg2Index = -1;
			newResult.prevImmedArg1 = k;
			newResult.prevImmedArg2 = -1;
			bAddToNewResults = TryAdd( resultsAllBitvec, resultsAll, newResult );
			if( bAddToNewResults )
			{
				newResultsToAdd.push_back( newResult );
			}
		}
		}

		cout << ">>> vrlimi'ing..." << endl;

		// Function with two input parameters and two immediate arguments.
		for( unsigned i = 0; i < vecSet.size(); i++ )
		{
		for( unsigned j = 0; j < vecSet.size(); j++ )
		{
		for( unsigned k = 0; k < 16; k++ )
		{
		for( unsigned m = 0; m < 4; m++ )
		{
			newResult.vec = vrlimi( vecSet[i].vec, vecSet[j].vec, k, m );
			newResult.prevFunc = VRLIMI;
			newResult.prevArg1Index = GetPermuteIndex( vecSet[i].vec );
			newResult.prevArg2Index = GetPermuteIndex( vecSet[j].vec );
			newResult.prevImmedArg1 = k;
			newResult.prevImmedArg2 = m;
			bAddToNewResults = TryAdd( resultsAllBitvec, resultsAll, newResult );
			if( bAddToNewResults )
			{
				newResultsToAdd.push_back( newResult );
			}
		}
		}
		}
		}
#endif // XBOX360

		cout << "newResultsToAdd.size() = " << newResultsToAdd.size() << endl;

		// Add the new Results from 'newResultsToAdd' to 'vecSet'. There should be no duplicates here.
		for( unsigned i = 0; i < newResultsToAdd.size(); i++ )
		{
			vecSet.push_back( newResultsToAdd[i] );
		}
		newResultsToAdd.clear();

		cout << "vecSet.size() = " << vecSet.size() << endl;

		stringstream ss;
		ss << "C:/VECSET_SO_FAR_" << vecSet.size() << ".txt";
		ofstream vecset( ss.str().c_str() );
		for( unsigned i = 0; i < vecSet.size(); i++ )
			vecset << vecSet[i].vec[0] << " " << vecSet[i].vec[1] << " " << vecSet[i].vec[2] << " " << vecSet[i].vec[3] << endl;
		vecset.close();


	} // while( !AreAllResultsFound( resultsAll ) )
	
	// Final step: traverse the list and work backwards to generate code.
	// Print out all the 'resultsAll' results to a textfile.
#if PERMUTE_SOLUTION
	ofstream os0( "C:/v4perm0instruction_.inl" );
	ofstream os1( "C:/v4perm1instruction_.inl" );
	ofstream os2( "C:/v4perm2instructions_.inl" );
	ofstream os3( "C:/v4perm3instructions_.inl" );
	ofstream os4( "C:/v4perm4instructions_.inl" );
	ofstream os5( "C:/v4perm5instructions_.inl" );
	ofstream os6( "C:/v4perm6instructions_.inl" );
	ofstream os7plus( "C:/v4perm7plusinstructions_.inl" );
#elif PERMUTETWO_SOLUTION
	ofstream os0( "C:/v4permtwo0instruction_.inl" );
	ofstream os1( "C:/v4permtwo1instruction_.inl" );
	ofstream os2( "C:/v4permtwo2instructions_.inl" );
	ofstream os3( "C:/v4permtwo3instructions_.inl" );
	ofstream os4( "C:/v4permtwo4instructions_.inl" );
	ofstream os5( "C:/v4permtwo5instructions_.inl" );
	ofstream os6( "C:/v4permtwo6instructions_.inl" );
	ofstream os7plus( "C:/v4permtwo7plusinstructions_.inl" );
#endif
	for( unsigned i = 0; i < NUM_RESULTS; i++ )
	{
		
		string funcBody = GetVarName( resultsAll, i );

		// (1) Strip out redundant statements.
		vector<string> lines;
		string tempStr = funcBody;
		while( tempStr.find_first_of( '\n' ) != string::npos )
		{
			lines.push_back( tempStr.substr( 0, tempStr.find_first_of( '\n' ) ) );
			tempStr.erase( 0, tempStr.find_first_of( '\n' )+1 );
		}
		lines.push_back( tempStr );
		for( unsigned j = 0; j < lines.size(); j++ )
		{
			// Empty lines.
			if(lines[j].empty())
			{
				lines.erase(lines.begin()+j);
				j--;
			}
			// Duplicate lines.
			else
			{
				for( unsigned k = 0; k < lines.size(); k++ )
				{
					if(j==k)
						continue;
					else if( lines[j] == lines[k] )
					{
						lines.erase(lines.begin()+(k>j?k:j));
						j--;
						break;
					}
				}
			}
		}
		// Write the result back out.
		funcBody = string("\n") + lines[0];
		for( unsigned j = 1; j < lines.size(); j++ )
		{
			funcBody += string("\n") + lines[j];
		}

		// (2) Count the number of lines.
		unsigned numLines = (unsigned)lines.size();

		// (3) Print out to separate files, depending on the # of instructions used.
		stringstream finalBody;
#if PERMUTE_SOLUTION
		finalBody << "template <>\ninline Vector_4V_Out V4Permute<" << GetPermuteString(i) << ">( Vector_4V_In v )\n";
#elif PERMUTETWO_SOLUTION
		finalBody << "template <>\ninline Vector_4V_Out V4PermuteTwo<" << GetPermuteString(i) << ">( Vector_4V_In v1, Vector_4V_In v2 )\n";
#endif
		finalBody << "{";
		finalBody << funcBody;
		finalBody << "\n}\n";
		finalBody << "\n";
		switch( numLines )
		{
		case 0:
			os0 << finalBody.str();
			break;
		case 1:
			os1 << finalBody.str();
			break;
		case 2:
			os2 << finalBody.str();
			break;
		case 3:
			os3 << finalBody.str();
			break;
		case 4:
			os4 << finalBody.str();
			break;
		case 5:
			os5 << finalBody.str();
			break;
		case 6:
			os6 << finalBody.str();
			break;
		default:
			os7plus << finalBody.str();
			break;
		};
		finalBody.clear();
	}

	os0.close();
	os1.close();
	os2.close();
	os3.close();
	os4.close();
	os5.close();
	os6.close();
	os7plus.close();




	return 0;
}















// Return the var name.
string GetVarName( vector<Result> const& resultsAllVec, unsigned index, bool clear )
{
	static string body;
	if( clear )	body.clear();
	string thisName;
	Result r = resultsAllVec[index];

	// Ending condition: if we have an input variable.
	if( r.prevFunc == -1 )
	{
#if PERMUTE_SOLUTION
		if( index == GetPermuteIndex( vec4(0,1,2,3) ) )
			return string( "v" );
#elif PERMUTETWO_SOLUTION
		if( index == GetPermuteIndex( vec4(0,1,2,3) ) )
			return string( "v1" );
		else if( index == GetPermuteIndex( vec4(4,5,6,7) ) )
			return string( "v2" );
#endif
		else
			assert( !"ERROR!" );
	}

	// Else, we have a statement that depends on other variables.
	else
	{
		stringstream imm1, imm2;
		if( r.prevFunc == VSLDOI )
		{
			imm1 << r.prevImmedArg1*4;	// Convert to bytes.
			imm2 << r.prevImmedArg2*4;	//
		}
		else
		{
			imm1 << r.prevImmedArg1;
			imm2 << r.prevImmedArg2;
		}
		string var1name;
		string var2name;

		switch( r.prevFunc )
		{
		case MERGEXY:
			// Create the mangled variable names, by recursively fetching names.
			var1name = GetVarName( resultsAllVec, r.prevArg1Index, false );
			var2name = GetVarName( resultsAllVec, r.prevArg2Index, false );
			// Mangle this name from the other names.
			thisName = string("_MXY_") + var1name + "_" + var2name + "_";			
			// Append statement onto static 'body' string.
			if(!clear)
				body += "\n\tVector_4V " + thisName + " = V4MergeXY( " + var1name + ", " + var2name + " );";
			else
				body += "\n\treturn V4MergeXY( " + var1name + ", " + var2name + " );";
			break;
		case MERGEZW:
			// Create the mangled variable names, by recursively fetching names.
			var1name = GetVarName( resultsAllVec, r.prevArg1Index, false );
			var2name = GetVarName( resultsAllVec, r.prevArg2Index, false );
			// Mangle this name from the other names.
			thisName = string("_MZW_") + var1name + "_" + var2name + "_";			
			// Append statement onto static 'body' string.
			if(!clear)
				body += "\n\tVector_4V " + thisName + " = V4MergeZW( " + var1name + ", " + var2name + " );";
			else
				body += "\n\treturn V4MergeZW( " + var1name + ", " + var2name + " );";
			break;
		case VSLDOI:
			// Create the mangled variable names, by recursively fetching names.
			var1name = GetVarName( resultsAllVec, r.prevArg1Index, false );
			var2name = GetVarName( resultsAllVec, r.prevArg2Index, false );
			// Mangle this name from the other names.
			thisName = string("_DOI_") + var1name + "_" + var2name + "_" + imm1.str() + "_";
			// Append statement onto static 'body' string.
#if XBOX360
			if(!clear)
				body += "\n\tVector_4V " + thisName + " = __vsldoi( " + var1name + ", " + var2name + ", " + imm1.str() + " );";
			else
				body += "\n\treturn __vsldoi( " + var1name + ", " + var2name + ", " + imm1.str() + " );";
#elif PS3
			if(!clear)
				body += "\n\tVector_4V " + thisName + " = vec_sld( " + var1name + ", " + var2name + ", " + imm1.str() + " );";
			else
				body += "\n\treturn vec_sld( " + var1name + ", " + var2name + ", " + imm1.str() + " );";
#endif
			break;
		case SPLATX:
			// Create the mangled variable names, by recursively fetching names.
			var1name = GetVarName( resultsAllVec, r.prevArg1Index, false );
			// Mangle this name from the other names.
			thisName = string("_SX_") + var1name + "_";
			// Append statement onto static 'body' string.
			if(!clear)
				body += "\n\tVector_4V " + thisName + " = V4SplatX( " + var1name + " );";
			else
				body += "\n\treturn V4SplatX( " + var1name + " );";
			break;
		case SPLATY:
			// Create the mangled variable names, by recursively fetching names.
			var1name = GetVarName( resultsAllVec, r.prevArg1Index, false );
			// Mangle this name from the other names.
			thisName = string("_SY_") + var1name + "_";
			// Append statement onto static 'body' string.
			if(!clear)
				body += "\n\tVector_4V " + thisName + " = V4SplatY( " + var1name + " );";
			else
				body += "\n\treturn V4SplatY( " + var1name + " );";
			break;
		case SPLATZ:
			// Create the mangled variable names, by recursively fetching names.
			var1name = GetVarName( resultsAllVec, r.prevArg1Index, false );
			// Mangle this name from the other names.
			thisName = string("_SZ_") + var1name + "_";
			// Append statement onto static 'body' string.
			if(!clear)
				body += "\n\tVector_4V " + thisName + " = V4SplatZ( " + var1name + " );";
			else
				body += "\n\treturn V4SplatZ( " + var1name + " );";
			break;
		case SPLATW:
			// Create the mangled variable names, by recursively fetching names.
			var1name = GetVarName( resultsAllVec, r.prevArg1Index, false );
			// Mangle this name from the other names.
			thisName = string("_SW_") + var1name + "_";
			// Append statement onto static 'body' string.
			if(!clear)
				body += "\n\tVector_4V " + thisName + " = V4SplatW( " + var1name + " );";
			else
				body += "\n\treturn V4SplatW( " + var1name + " );";
			break;
		case VPERMWI:
			// Create the mangled variable names, by recursively fetching names.
			var1name = GetVarName( resultsAllVec, r.prevArg1Index, false );
			// Mangle this name from the other names.
			thisName = string("_PRM_") + var1name + "_" + imm1.str() + "_";
			// Append statement onto static 'body' string.
			if(!clear)
				body += "\n\tVector_4V " + thisName + " = __vpermwi( " + var1name + ", " + imm1.str() + " );";
			else
				body += "\n\treturn __vpermwi( " + var1name + ", " + imm1.str() + " );";
			break;
		case VRLIMI:
			// Create the mangled variable names, by recursively fetching names.
			var1name = GetVarName( resultsAllVec, r.prevArg1Index, false );
			var2name = GetVarName( resultsAllVec, r.prevArg2Index, false );
			// Mangle this name from the other names.
			thisName = string("_RLI_") + var1name + "_" + var2name + "_" + imm1.str() + "_" + imm2.str() + "_";
			// Append statement onto static 'body' string.
			if(!clear)
				body += "\n\tVector_4V " + thisName + " = __vrlimi( " + var1name + ", " + var2name + ", " + imm1.str() + ", " + imm2.str() + " );";
			else
				body += "\n\treturn __vrlimi( " + var1name + ", " + var2name + ", " + imm1.str() + ", " + imm2.str() + " );";
			break;
		default:
			assert( !"ERROR!" );
			break;
		};

		// Either return the variable name or the function body, depending on whether this is the outermost recursion or not.
		return (clear ? body : thisName);
	}

	assert(0); // Shouldn't reach here.
	return string("");
}

bool AreAllResultsFound( bitset<NUM_RESULTS> const& resultsAllBitvec )
{
	bool bResult = true;
	for( unsigned i = 0; i < resultsAllBitvec.size(); i++ )
	{
		bResult = bResult && resultsAllBitvec[i];
	}
	return bResult;
}

bool TryAdd( bitset<NUM_RESULTS>& resultsAllBitvec, vector<Result>& resultsAllVec, Result const& newResult )
{
	int index = GetPermuteIndex(newResult.vec);
	if( resultsAllBitvec[index] == false )
	{
		resultsAllBitvec[index] = true;
		resultsAllVec[index] = newResult;

		return true;
	}
	return false;
}


int GetPermuteIndex( vec4 const& v )
{
#if PERMUTE_SOLUTION
	return (64*v.x + 16*v.y + 4*v.z + 1*v.w);
#elif PERMUTETWO_SOLUTION
	return (512*v.x + 64*v.y + 8*v.z + 1*v.w);
#endif
}

string GetPermuteString( int index )
{
#if PERMUTE_SOLUTION
	static const string prefixes[] = {"X", "Y", "Z", "W"};
	int indexX = (index / 64) % 4;
	int indexY = (index / 16) % 4;
	int indexZ = (index / 4) % 4;
	int indexW = (index / 1) % 4;
	string retVal = prefixes[indexX] + "," + prefixes[indexY] + "," + prefixes[indexZ] + "," + prefixes[indexW];
	return retVal;
#elif PERMUTETWO_SOLUTION
	static const string prefixes[] = {"X1", "Y1", "Z1", "W1", "X2", "Y2", "Z2", "W2"};
	int indexX = (index / 512) % 8;
	int indexY = (index / 64) % 8;
	int indexZ = (index / 8) % 8;
	int indexW = (index / 1) % 8;
	string retVal = prefixes[indexX] + "," + prefixes[indexY] + "," + prefixes[indexZ] + "," + prefixes[indexW];
	return retVal;
#endif
}
















//==============================================================================


vec4 MergeXY( vec4 a, vec4 b )
{
	return vec4( a.x,b.x,a.y,b.y );
}

vec4 MergeZW( vec4 a, vec4 b )
{
	return vec4( a.z,b.z,a.w,b.w );
}

vec4 vsldoi( vec4 a, vec4 b, unsigned int shift )
{
	if( shift > 4 )
	{
		cout << "SHIFTED TOO MUCH" << endl;
		exit(1);
	}

	vec8 combined = {a.x,a.y,a.z,a.w,b.x,b.y,b.z,b.w};
	return vec4( combined[shift], combined[shift+1], combined[shift+2], combined[shift+3] );
}

vec4 SplatX( vec4 in )
{
	return vec4( in.x, in.x, in.x, in.x );
}

vec4 SplatY( vec4 in )
{
	return vec4( in.y, in.y, in.y, in.y );
}

vec4 SplatZ( vec4 in )
{
	return vec4( in.z, in.z, in.z, in.z );
}

vec4 SplatW( vec4 in )
{
	return vec4( in.w, in.w, in.w, in.w );
}

vec4 vpermwi( vec4 in, unsigned immed )
{
	unsigned xIndex = (immed & 0xC0) >> 6;
	unsigned yIndex = (immed & 0x30) >> 4;
	unsigned zIndex = (immed & 0x0C) >> 2;
	unsigned wIndex = (immed & 0x03);

	return vec4(in[xIndex], in[yIndex], in[zIndex], in[wIndex]);
}

vec4 vrlimi( vec4 in1, vec4 in2, unsigned wMask, unsigned rotateAmt )
{
	unsigned maskX = (wMask & 0x8);
	unsigned maskY = (wMask & 0x4);
	unsigned maskZ = (wMask & 0x2);
	unsigned maskW = (wMask & 0x1);

	vec4 rotatedVec( in2[(0+rotateAmt)%4], in2[(1+rotateAmt)%4], in2[(2+rotateAmt)%4], in2[(3+rotateAmt)%4] );
	vec4 outVec;
	outVec.x = maskX == 0 ? in1[0] : rotatedVec[0];
	outVec.y = maskY == 0 ? in1[1] : rotatedVec[1];
	outVec.z = maskZ == 0 ? in1[2] : rotatedVec[2];
	outVec.w = maskW == 0 ? in1[3] : rotatedVec[3];
	return outVec;
}



