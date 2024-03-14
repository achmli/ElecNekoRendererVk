#pragma once

#include <string>

#include "math/MathUtils.h"

class Scene;
class RenderOptions;

bool LoadSceneFromFile(const std::string& filename, Scene* scene, RenderOptions& renderOptions, glm::mat4 xform, bool binary);