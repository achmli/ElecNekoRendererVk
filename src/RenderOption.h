#pragma once


namespace ElecNeko
{
    struct RenderOption
    {
        RenderOption()
        {
            skyBox = false;
            simpleRealSky = true;
            tonemapping = true;
            ACESFit = false;
            simpleACESFit = false;
            enableExplosure = false;
            isDeferred = true;

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
            skyColor[0] = .36f;
            skyColor[1] = .6f;
            skyColor[2] = 1.f;

            expStrengthA = 0.08f;
            expAAttenuationB = -0.35f;
            baseConstantC = 0.22f;
            expGammaAttenuationD = .1f;
            expAttenuationSpeedE = -2.2f;
            gammaScatteringF = 0.18f;
            chiContributeG = 0.02f;
            chiParmH = 0.75f;
            thetaFixI = 0.1f;
            Lm = 1.5f;
        }

        bool skyBox;
        bool simpleRealSky;
        bool tonemapping;
        bool ACESFit;
        bool simpleACESFit;
        bool enableExplosure;
        bool isDeferred;

        float sunDirection[3];
        float sunIntensity;
        float sunColor[3];
        float sunAngularRadius;
        float sunGlowSpread;
        float explosure;
        float skyColor[3];

        float expStrengthA;
        float expAAttenuationB;
        float baseConstantC;
        float expGammaAttenuationD;
        float expAttenuationSpeedE;
        float gammaScatteringF;
        float chiContributeG;
        float chiParmH;
        float thetaFixI;
        float Lm;
    };
} // namespace ElecNeko
