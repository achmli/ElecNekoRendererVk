//
//  OffscreenRenderer.h
//
#pragma once
#include <vector>

class DeviceContext;
class Buffer;
struct RenderModel;
struct RenderOption;

bool InitOffscreen( DeviceContext * device, int width, int height );
bool CleanupOffscreen( DeviceContext * device );

void DrawOffscreen( DeviceContext * device, int cmdBufferIndex, Buffer * uniforms, const RenderModel * renderModels, const int numModels );

namespace ElecNeko
{
    class Mesh;
    class SkyBox;
    class World;

    bool InitOffscreen(DeviceContext *device, const RenderOption &renderOption, int width, int height);
    bool CleanupOffscreen(DeviceContext *device, const RenderOption &renderOption);

    bool ReinitializeSky(DeviceContext *device, const RenderOption &renderOption);

    void DrawOffscreen(DeviceContext *device, int cmdBufferIndex, Buffer *uniforms, SkyBox &skyBox, std::vector<Mesh *> mesh, const RenderOption &renderOption);
    void DrawOffscreen(DeviceContext *device, int cmdBufferIndex, Buffer *uniforms, SkyBox &skyBox, World *world, const RenderOption &renderOption);
}