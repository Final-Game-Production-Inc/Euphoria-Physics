//
// vector/colorvector3.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "colorvector3.h"

using namespace rage;

//=============================================================================
// ColorVector3


void ColorVector3::RGBtoHSV()
{
    float r, g, b;
    float dx;
    float m;
    float h;

    r = x;
    g = y;
    b = z;

    z = r; if (z < g) z = g; if (z < b) z = b;
    m = r; if (m > g) m = g; if (m > b) m = b;

    dx = z - m;

    if (dx == 0.0f)
    {
        x = 0.0f;
        y = 0.0f;
    }
    else
    {
        y = dx / z;
        if (r == z)
		{
			h = (z - b) / dx - (z - g) / dx;				//lint !e777 testing floats for equality
		}
        else if (g == z)
		{
			h = (z - r) / dx - (z - b) / dx + 2.0f;	//lint !e777 testing floats for equality
		}
        else
		{
			h = (z - g) / dx - (z - r) / dx + 4.0f;
		}

        if (h < 0.0f)
		{	
			h += 6.0f;
		}

        x = h / 6.0f;
    }
}



void ColorVector3::HSVtoRGB()
{
    float p;
    float q;
    float t;
    float h;
    float f;
    int i;

    if (y == 0.0f)
    {
        x = y = z;
    }
    else
    {
        h = fmodf(x, 1.0f) * 6.0f;
        i = (int) h;
        f = h - (float) i;
        p = z * (1.0f - y);
        q = z * (1.0f - (y * f));
        t = z * (1.0f - (y * (1.0f - f)));
        switch (i)
        {
            case 0 :
				x = z;
				y = t;
				z = p;
				break;
            case 1 :
				x = q;
				y = z;
				z = p;
				break;
            case 2 :
				x = p;
				y = z;
				z = t;
				break;
            case 3 :
				x = p;
				y = q;
				break;
            case 4 :
				x = t;
				y = p;
				break;
            case 5 :
				x = z;
				y = p;
				z = q;
				break;
			default:
				break;
        }
    }
}

// Returns zero if all is well. If the signal will go too high it returns how
// high it will go. If the signal will go too low it returns how low it
// will go - a negative number.
// It is not possible for the composite signal to simultaneously
// go off the top and bottom of the chart.
// This code is non-optimized, for clarity.
float ColorVector3::IsNtscSafeColor() const
{
	// http://www.gamasutra.com/features/20010622/dawson_03.htm
	// const float pi = 3.14159265358979323846f;
	float cos33 = 0.83867056794542402963759094180455f; // cosf(33 * pi / 180);
	float sin33 = 0.54463903501502708222408369208157f; // sinf(33 * pi / 180);

	// Specify the maximum amount you are willing to go
	// above 1.0 and below 0.0. This should be set no
	// higher than 0.2. 0.1 may produce better results.
	// This value assumes RGB values from 0.0 to 1.0.
	const float k_maxExcursion = 0.1f;

	// invalid color
	//if (x < 0 || x > 1 || y < 0 || y > 1 || z < 0 || z > 1)
	//	return true;

    // Convert from RGB to YUV space.
    float Y = 0.299f * x + 0.587f * y + 0.114f * z;
    float u = 0.492f * (z - Y);
    float v = 0.877f * (x - Y);

    // Convert from YUV to YIQ space. This could be combined with
    // the RGB to YIQ conversion for better performance.
    float i = cos33 * v - sin33 * u;
    float q = sin33 * v + cos33 * u;

    // Calculate the amplitude of the chroma signal.
    float c = sqrtf(i * i + q * q);
    // See if the composite signal will go too high or too low.
    float maxComposite = Y + c;
    float  minComposite = Y - c;
    if (maxComposite > 1.0f + k_maxExcursion)
	{
		return maxComposite;
	}
    else if (minComposite < -k_maxExcursion)
	{
		return minComposite;
	}
	else
	{
		return 0.0f;
	}
}


void ColorVector3::TransformToNtscSafeColor()
{
	float cos33 = 0.83867056794542402963759094180455f; // cosf(33 * pi / 180);
	float sin33 = 0.54463903501502708222408369208157f; // sinf(33 * pi / 180);

	// Specify the maximum amount you are willing to go
	// above 1.0 and below 0.0. This should be set no
	// higher than 0.2. 0.1 may produce better results.
	// This value assumes RGB values from 0.0 to 1.0.
	const float k_maxExcursion = 0.1f;

	// This epsilon value is necessary because the standard color space conversion
	// factors are only accurate to three decimal places.
	const float k_epsilon = 0.001f;

	// maxComposite is slightly misnamed - it may be min
	// composite or maxComposite.
	float maxComposite = IsNtscSafeColor();
	if (maxComposite > 0.0001f || maxComposite< -0.0001f)
	{
		// Convert into YIQ space, and convert c, the amplitude
		// of the chroma component.

		// Convert from RGB to YUV space.
		float tvY = 0.299f * x + 0.587f * y + 0.114f * z;
		float u = 0.492f * (z - tvY);
		float v = 0.877f * (x - tvY);

		// Convert from YUV to YIQ space. This could be combined with
		// the RGB to YIQ conversion for better performance.
		float i = cos33 * v - sin33 * u;
		float q = sin33 * v + cos33 * u;

		// Calculate the amplitude of the chroma signal.
		float c = sqrtf(i * i + q * q);
		float coolant;
		// Calculate the ratio between the maximum chroma range allowed
		// and the current chroma range. 
		if (maxComposite > 0.0f)
		{
			// The maximum chroma range is the maximum composite value
			// minus the luminance.
			coolant = (1.0f + k_maxExcursion - k_epsilon - tvY) / c;
		}
		else
		{
			// The maximum chroma range is the luminance minus the
			// minimum composite value.
			coolant = (tvY - -k_maxExcursion - k_epsilon) / c;
		}

		// Scale I and Q down, thus scaling chroma down and reducing the
		// saturation proportionally.
		i *= coolant;
		q *= coolant;

		// convert back to rgb
		float r = tvY + 0.956f * i + 0.620f * q;
		float g = tvY - 0.272f * i - 0.647f * q;
		float b = tvY - 1.108f * i + 1.705f * q;

		x=r;
		y=g;
		z=b;
	}
}
