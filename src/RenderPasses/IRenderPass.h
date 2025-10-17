#pragma once

#include <memory>
#include "RHI/ElecNekoShader.h"
#include "RHI/Pipeline.h"

namespace ElecNeko
{
    template<typename RT>
    class IRenderPass
    {
    public:
        std::unique_ptr<RT> m_renderTarget;

        ElecNekoPipeline m_pipeline;
        ElecNekoShader m_shader;
    };
} // namespace ElecNeko
