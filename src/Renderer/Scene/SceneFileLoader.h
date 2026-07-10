// src/Renderer/Scene/SceneFileLoader.h
#pragma once

#include "Renderer/Scene/SceneLoadDesc.h"

#include <filesystem>

namespace ElecNeko
{
    class SceneFileLoader
    {
    public:
        static bool Load(const std::filesystem::path &sceneFile, SceneLoadDesc &outDesc);
    };
} // namespace ElecNeko
