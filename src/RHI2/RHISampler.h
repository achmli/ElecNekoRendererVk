#pragma once

#include "RHI2/RHICommon.h"

namespace RHI
{
    enum class Filter
    {
        Nearest,
        Linear
    };

    enum class SamplerMipmapMode
    {
        Nearest,
        Linear
    };

    enum class AddressMode
    {
        Repeat,
        ClampToEdge,
        ClampToBorder
    };

    enum class CompareOp
    {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum class BorderColor
    {
        FloatTransparentBlack,
        IntTransparentBlack,
        FloatOpaqueBlack,
        IntOpaqueBlack,
        FloatOpaqueWhite,
        IntOpaqueWhite
    };

    struct SamplerDesc
    {
        Filter minFilter = Filter::Linear;
        Filter magFilter = Filter::Linear;
        SamplerMipmapMode mipmapMode = SamplerMipmapMode::Linear;

        AddressMode addressU = AddressMode::Repeat;
        AddressMode addressV = AddressMode::Repeat;
        AddressMode addressW = AddressMode::Repeat;

        bool anisotropyEnable = false;
        float maxAnisotropy = 1.0f;

        bool compareEnable = false;
        CompareOp compareOp = CompareOp::Always;

        float minLod = 0.0f;
        float maxLod = 0.0f;
        float mipLodBias = 0.0f;

        BorderColor borderColor = BorderColor::IntOpaqueBlack;

        const char *debugName = nullptr;
    };

    class Sampler
    {
    public:
        virtual ~Sampler() = default;

        virtual const SamplerDesc &GetDesc() const = 0;
    };
} // namespace RHI
