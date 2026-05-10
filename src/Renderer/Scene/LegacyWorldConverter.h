#pragma once

#include <memory>

class DeviceContext;

namespace ElecNeko
{
    class World;
    class RenderScene;

    // 临时转换层：
    // 用旧 World 作为加载器，把它转换成新的 RenderScene。
    //
    // 之后流程会变成：
    //
    // World::LoadSceneFromFile(...)
    //        ↓
    // ConvertLegacyWorldToRenderScene(...)
    //        ↓
    // RenderScene::BuildDrawLists()
    //
    // 等新渲染路径稳定后，再删除旧 Scene 合批路径。
    std::unique_ptr<RenderScene> ConvertLegacyWorldToRenderScene(DeviceContext *device, World *world);
} // namespace ElecNeko
