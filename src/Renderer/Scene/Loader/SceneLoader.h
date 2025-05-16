#pragma once

#include "Renderer/Scene/World.h"

namespace ElecNeko
{
    bool LoadSceneFromFile(const std::string &filename, std::unique_ptr<Scene> scene, RenderOption &renderOption);
}
