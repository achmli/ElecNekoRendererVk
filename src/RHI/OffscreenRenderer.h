//
//  OffscreenRenderer.h
//
#pragma once
#include <vector>

class DeviceContext;
class Buffer;
struct RenderModel;

bool InitOffscreen( DeviceContext * device, int width, int height );
bool CleanupOffscreen( DeviceContext * device );

void DrawOffscreen( DeviceContext * device, int cmdBufferIndex, Buffer * uniforms, const RenderModel * renderModels, const int numModels );

namespace ElecNeko
{
    class Mesh;
    class SkyBox;

    void DrawOffscreen(DeviceContext *device, int cmdBufferIndex, Buffer *uniforms, SkyBox& skyBox, std::vector<Mesh*> mesh);
}