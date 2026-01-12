
#include "mat33v_soa.h"

#if !__NO_OUTPUT

namespace rage
{
	void SoA_Mat33V::Print() const
	{
		Printf( "[[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f]]\n",
			Vec::GetX(m_00),Vec::GetY(m_00),Vec::GetZ(m_00),Vec::GetW(m_00),
			Vec::GetX(m_01),Vec::GetY(m_01),Vec::GetZ(m_01),Vec::GetW(m_01),
			Vec::GetX(m_02),Vec::GetY(m_02),Vec::GetZ(m_02),Vec::GetW(m_02) );
		Printf( "[[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f]]\n",
			Vec::GetX(m_10),Vec::GetY(m_10),Vec::GetZ(m_10),Vec::GetW(m_10),
			Vec::GetX(m_11),Vec::GetY(m_11),Vec::GetZ(m_11),Vec::GetW(m_11),
			Vec::GetX(m_12),Vec::GetY(m_12),Vec::GetZ(m_12),Vec::GetW(m_12) );
		Printf( "[[%f,%f,%f,%f],[%f,%f,%f,%f],[%f,%f,%f,%f]]\n",
			Vec::GetX(m_20),Vec::GetY(m_20),Vec::GetZ(m_20),Vec::GetW(m_20),
			Vec::GetX(m_21),Vec::GetY(m_21),Vec::GetZ(m_21),Vec::GetW(m_21),
			Vec::GetX(m_22),Vec::GetY(m_22),Vec::GetZ(m_22),Vec::GetW(m_22) );
	}

	void SoA_Mat33V::PrintHex() const
	{
		union {
			float f;
			unsigned int i;
		} _0x,_0y,_0z,_0w,_1x,_1y,_1z,_1w,_2x,_2y,_2z,_2w;

		_0x.f = Vec::GetX(m_00); _0y.f = Vec::GetY(m_00); _0z.f = Vec::GetZ(m_00); _0w.f = Vec::GetW(m_00);
		_1x.f = Vec::GetX(m_01); _1y.f = Vec::GetY(m_01); _1z.f = Vec::GetZ(m_01); _1w.f = Vec::GetW(m_01);
		_2x.f = Vec::GetX(m_02); _2y.f = Vec::GetY(m_02); _2z.f = Vec::GetZ(m_02); _2w.f = Vec::GetW(m_02);
		Printf( "[[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X]]\n",
			_0x.i,_0y.i,_0z.i,_0w.i,
			_1x.i,_1y.i,_1z.i,_1w.i,
			_2x.i,_2y.i,_2z.i,_2w.i );
		_0x.f = Vec::GetX(m_10); _0y.f = Vec::GetY(m_10); _0z.f = Vec::GetZ(m_10); _0w.f = Vec::GetW(m_10);
		_1x.f = Vec::GetX(m_11); _1y.f = Vec::GetY(m_11); _1z.f = Vec::GetZ(m_11); _1w.f = Vec::GetW(m_11);
		_2x.f = Vec::GetX(m_12); _2y.f = Vec::GetY(m_12); _2z.f = Vec::GetZ(m_12); _2w.f = Vec::GetW(m_12);
		Printf( "[[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X]]\n",
			_0x.i,_0y.i,_0z.i,_0w.i,
			_1x.i,_1y.i,_1z.i,_1w.i,
			_2x.i,_2y.i,_2z.i,_2w.i );
		_0x.f = Vec::GetX(m_20); _0y.f = Vec::GetY(m_20); _0z.f = Vec::GetZ(m_20); _0w.f = Vec::GetW(m_20);
		_1x.f = Vec::GetX(m_21); _1y.f = Vec::GetY(m_21); _1z.f = Vec::GetZ(m_21); _1w.f = Vec::GetW(m_21);
		_2x.f = Vec::GetX(m_22); _2y.f = Vec::GetY(m_22); _2z.f = Vec::GetZ(m_22); _2w.f = Vec::GetW(m_22);
		Printf( "[[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X],[0x%X,0x%X,0x%X,0x%X]]\n",
			_0x.i,_0y.i,_0z.i,_0w.i,
			_1x.i,_1y.i,_1z.i,_1w.i,
			_2x.i,_2y.i,_2z.i,_2w.i );
	}
} // namespace rage

#endif
