
#include "mat44v_soa.h"

#if !__NO_OUTPUT

namespace rage
{

	void SoA_Mat44V::Print() const
	{
		Printf( "[[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f]]\n",
			Vec::GetX(m_00),Vec::GetY(m_00),Vec::GetZ(m_00),Vec::GetW(m_00),
			Vec::GetX(m_01),Vec::GetY(m_01),Vec::GetZ(m_01),Vec::GetW(m_01),
			Vec::GetX(m_02),Vec::GetY(m_02),Vec::GetZ(m_02),Vec::GetW(m_02),
			Vec::GetX(m_03),Vec::GetY(m_03),Vec::GetZ(m_03),Vec::GetW(m_03) );
		Printf( "[[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f]]\n",
			Vec::GetX(m_10),Vec::GetY(m_10),Vec::GetZ(m_10),Vec::GetW(m_10),
			Vec::GetX(m_11),Vec::GetY(m_11),Vec::GetZ(m_11),Vec::GetW(m_11),
			Vec::GetX(m_12),Vec::GetY(m_12),Vec::GetZ(m_12),Vec::GetW(m_12),
			Vec::GetX(m_13),Vec::GetY(m_13),Vec::GetZ(m_13),Vec::GetW(m_13) );
		Printf( "[[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f]]\n",
			Vec::GetX(m_20),Vec::GetY(m_20),Vec::GetZ(m_20),Vec::GetW(m_20),
			Vec::GetX(m_21),Vec::GetY(m_21),Vec::GetZ(m_21),Vec::GetW(m_21),
			Vec::GetX(m_22),Vec::GetY(m_22),Vec::GetZ(m_22),Vec::GetW(m_22),
			Vec::GetX(m_23),Vec::GetY(m_23),Vec::GetZ(m_23),Vec::GetW(m_23) );
		Printf( "[[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f]]\n",
			Vec::GetX(m_30),Vec::GetY(m_30),Vec::GetZ(m_30),Vec::GetW(m_30),
			Vec::GetX(m_31),Vec::GetY(m_31),Vec::GetZ(m_31),Vec::GetW(m_31),
			Vec::GetX(m_32),Vec::GetY(m_32),Vec::GetZ(m_32),Vec::GetW(m_32),
			Vec::GetX(m_33),Vec::GetY(m_33),Vec::GetZ(m_33),Vec::GetW(m_33) );
	}

	void SoA_Mat44V::PrintHex() const
	{
		union {
			float f;
			unsigned int i;
		} _0x,_0y,_0z,_0w,_1x,_1y,_1z,_1w,_2x,_2y,_2z,_2w,_3x,_3y,_3z,_3w;

		_0x.f = Vec::GetX(m_00); _0y.f = Vec::GetY(m_00); _0z.f = Vec::GetZ(m_00); _0w.f = Vec::GetW(m_00);
		_1x.f = Vec::GetX(m_01); _1y.f = Vec::GetY(m_01); _1z.f = Vec::GetZ(m_01); _1w.f = Vec::GetW(m_01);
		_2x.f = Vec::GetX(m_02); _2y.f = Vec::GetY(m_02); _2z.f = Vec::GetZ(m_02); _2w.f = Vec::GetW(m_02);
		_3x.f = Vec::GetX(m_03); _3y.f = Vec::GetY(m_03); _3z.f = Vec::GetZ(m_03); _3w.f = Vec::GetW(m_03);
		Printf( "[[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X]]\n",
			_0x.i,_0y.i,_0z.i,_0w.i,
			_1x.i,_1y.i,_1z.i,_1w.i,
			_2x.i,_2y.i,_2z.i,_2w.i,
			_3x.i,_3y.i,_3z.i,_3w.i );
		_0x.f = Vec::GetX(m_10); _0y.f = Vec::GetY(m_10); _0z.f = Vec::GetZ(m_10); _0w.f = Vec::GetW(m_10);
		_1x.f = Vec::GetX(m_11); _1y.f = Vec::GetY(m_11); _1z.f = Vec::GetZ(m_11); _1w.f = Vec::GetW(m_11);
		_2x.f = Vec::GetX(m_12); _2y.f = Vec::GetY(m_12); _2z.f = Vec::GetZ(m_12); _2w.f = Vec::GetW(m_12);
		_3x.f = Vec::GetX(m_13); _3y.f = Vec::GetY(m_13); _3z.f = Vec::GetZ(m_13); _3w.f = Vec::GetW(m_13);
		Printf( "[[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X]]\n",
			_0x.i,_0y.i,_0z.i,_0w.i,
			_1x.i,_1y.i,_1z.i,_1w.i,
			_2x.i,_2y.i,_2z.i,_2w.i,
			_3x.i,_3y.i,_3z.i,_3w.i );
		_0x.f = Vec::GetX(m_20); _0y.f = Vec::GetY(m_20); _0z.f = Vec::GetZ(m_20); _0w.f = Vec::GetW(m_20);
		_1x.f = Vec::GetX(m_21); _1y.f = Vec::GetY(m_21); _1z.f = Vec::GetZ(m_21); _1w.f = Vec::GetW(m_21);
		_2x.f = Vec::GetX(m_22); _2y.f = Vec::GetY(m_22); _2z.f = Vec::GetZ(m_22); _2w.f = Vec::GetW(m_22);
		_3x.f = Vec::GetX(m_23); _3y.f = Vec::GetY(m_23); _3z.f = Vec::GetZ(m_23); _3w.f = Vec::GetW(m_23);
		Printf( "[[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X]]\n",
			_0x.i,_0y.i,_0z.i,_0w.i,
			_1x.i,_1y.i,_1z.i,_1w.i,
			_2x.i,_2y.i,_2z.i,_2w.i,
			_3x.i,_3y.i,_3z.i,_3w.i );
		_0x.f = Vec::GetX(m_30); _0y.f = Vec::GetY(m_30); _0z.f = Vec::GetZ(m_30); _0w.f = Vec::GetW(m_30);
		_1x.f = Vec::GetX(m_31); _1y.f = Vec::GetY(m_31); _1z.f = Vec::GetZ(m_31); _1w.f = Vec::GetW(m_31);
		_2x.f = Vec::GetX(m_32); _2y.f = Vec::GetY(m_32); _2z.f = Vec::GetZ(m_32); _2w.f = Vec::GetW(m_32);
		_3x.f = Vec::GetX(m_33); _3y.f = Vec::GetY(m_33); _3z.f = Vec::GetZ(m_33); _3w.f = Vec::GetW(m_33);
		Printf( "[[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X]]\n",
			_0x.i,_0y.i,_0z.i,_0w.i,
			_1x.i,_1y.i,_1z.i,_1w.i,
			_2x.i,_2y.i,_2z.i,_2w.i,
			_3x.i,_3y.i,_3z.i,_3w.i );
	}

} // namespace rage

#endif
