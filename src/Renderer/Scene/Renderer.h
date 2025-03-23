#pragma once

#include <memory>


#include "Scene.h"

namespace ElecNeko
{
    class Renderer
    {
    protected:
        std::unique_ptr<Scene> scene;
    };
} // namespace ElecNeko
