//
//  OffscreenRenderer.h
//
#pragma once
#include <vector>

class DeviceContext;
class Buffer;
struct RenderModel;
struct RenderOption;

namespace ElecNeko
{
    class Mesh;
    class SkyBox;
    class World;
    class Scene;
    class CascadeShadow;

    bool InitOffscreen(DeviceContext *device, const RenderOption &renderOption, int width, int height);
    bool CleanupOffscreen(DeviceContext *device, const RenderOption &renderOption);

    bool ReinitializeSky(DeviceContext *device, const RenderOption &renderOption);

    void DrawOffscreen(DeviceContext *device, int cmdBufferIndex, Buffer *uniforms, SkyBox &skyBox, Scene *scene, RenderOption &renderOption,
                       CascadeShadow &csm);
} // namespace ElecNeko
