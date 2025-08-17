#pragma once


namespace ElecNeko
{
	struct RenderOption
	{
		RenderOption()
		{ 
			skyBox = false;
            simpleRealSky = true;
            sunDirection[0] = 1.f;
            sunDirection[1] = 1.f;
            sunDirection[2] = 1.f;
            sunIntensity = 8.f;
            sunColor[0] = 1.5f;
            sunColor[1] = 1.2f;
            sunColor[2] = 1.f;
            sunAngularRadius = .53f;
            sunGlowSpread = .07f;
            explosure = 1.2f;
		}

		bool skyBox;
        bool simpleRealSky;
        float sunDirection[3];
        float sunIntensity;
        float sunColor[3];
        float sunAngularRadius;
        float sunGlowSpread;
        float explosure;
	};
}