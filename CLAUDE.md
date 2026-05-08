# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Instructions

**Prerequisites:**
- CMake 3.28+
- Vulkan SDK (included in `thirdParty/vulkan`)
- C++20 compiler (MSVC on Windows)

**Build steps:**
```bash
mkdir build && cd build
cmake ..
cmake --build . --config RelWithDebInfo
```

The build system will automatically copy `shaderc_shared.dll` to the output directory on Windows.

**Key build configurations:**
- `RelWithDebInfo` - Recommended for development (optimizations + debug symbols)
- `Debug` - Full debugging
- `Release` - Maximum optimizations

## Architecture Overview

ElecNekoRendererVk is a Vulkan-based deferred renderer implementing advanced rendering techniques. The architecture follows a modular design with clear separation between the RHI layer, scene management, and render passes.

### Core Layer Structure

**RHI Layer (`src/RHI/`)** - Vulkan abstraction providing:
- `DeviceContext` - Core Vulkan device/instance management, command buffer recording
- `Buffer`, `Image` - GPU resource management with automatic memory transitions
- `Pipeline`, `Descriptor` - Graphics pipeline and descriptor set management
- `SwapChain`, `FrameBuffer` - Presentation and framebuffer management
- `OffscreenRenderer` - Off-screen rendering for deferred shading

**Scene System (`src/Scene/`)** - Two parallel scene implementations:
1. **Legacy System** (`World.cpp`, `Scene.cpp`) - Traditional immediate-mode rendering with direct buffer uploads
2. **Modern System** (`ElecNekoWorld.cpp`, `ElecNekoScene.cpp`) - GPU-driven with instancing and indirect draws

Currently, the modern system is excluded from compilation (see CMakeLists.txt:27). The legacy system is the primary implementation.

**Rendering Pipeline:**
1. **Shadow Pass** - Cascaded Shadow Mapping (CSM) for directional lights
2. **G-Buffer Pass** - Deferred geometry pass (position, normal, albedo, material properties)
3. **Lighting Pass** - Screen-space shading with PBR
4. **Post-Processing** - Screen Space Reflections (SSR), Ambient Occlusion (AO), tonemapping
5. **Sky Pass** - Physical sky model or cubemap skybox
6. **Present** - Copy to swapchain

### Key Systems

**Shaders (`src/shaders/`)** - GLSL/HLSL shaders compiled at build time using shaderc:
- Geometry passes (G-buffer generation)
- Shadow passes (CSM)
- Compute shaders (SSR, AO)
- Post-processing (tonemapping)
- Sky rendering (physical atmosphere model)

**Resource Loading (`src/Loader/`)**:
- `Texture` - Image loading with caching (stb_image)
- `Mesh` - Model loading via Assimp (supports .obj, .gltf)
- `Material` - PBR material properties

**Math Library (`src/Math/`)** - Custom math implementation:
- `Vector` - Vec2, Vec3, Vec4 operations
- `Matrix` - Mat4 transformations (column-major)
- `Quat` - Quaternion rotations

**Physics (`src/Physics/`)** - Minimal rigid body physics:
- GJK collision detection
- Constraint solving (mostly incomplete, used sparingly)

### Render Options

Runtime configurable rendering features (`src/RenderOption.h`):
- Deferred vs forward rendering toggle
- Sky rendering modes (cubemap, physical sky, simple sky)
- Tonemapping (ACES fit, custom exposure)
- Screen Space Reflections (steps, distance, thickness, roughness cutoff)
- Ambient Occlusion toggle
- Sun/sky parameters (direction, intensity, color, atmospheric scattering)

**ImGui Integration:** All render options are exposed via ImGui debug overlay for real-time tuning.

## Development Notes

**Scene Management:**
- The `World` class manages the scene graph and render batches
- Scene files are loaded from `res/scenes/` at runtime
- Models are loaded from `res/models/` and `res/CubeMaps/`
- Hot-reloading: Changing scene files at runtime is supported via the UI

**Vulkan Patterns:**
- Explicit synchronization with pipeline barriers
- Descriptor indexing for bindless textures (in modern system)
- Manual command buffer recording (no command pool reuse)
- Single-threaded rendering loop

**Shader Compilation:**
- Shaders are compiled from GLSL to SPIR-V at build time
- Shader files must be listed in CMakeLists.txt for inclusion
- Compile-time define: `USE_SHADERC=1`

**Current Development State:**
- The modern scene system (`ElecNekoWorld`, `ElecNekoScene`) is disabled in CMakeLists.txt
- Active development uses the legacy `World`/`Scene` system
- Recent work focuses on SSR and cascaded shadow mapping improvements
- Branch naming: `ElecNeko0.06` indicates iterative development

**Code Style:**
- C++20 features (smart pointers, RAII, move semantics)
- Assertion-based error handling (no exceptions)
- Member variables prefixed with `m_`
- Parameters passed by const reference when possible

## Testing and Debugging

**Debug Controls:**
- WASD - Camera movement
- Mouse - Camera rotation (right-click drag)
- Scroll - Camera zoom
- Space - Pause/unpause
- UI toggles in ImGui overlay for all render features

**Performance Considerations:**
- Batch opaque geometry for efficient rendering
- Minimize descriptor set updates
- Use pipeline barriers only when necessary
- Profile with RelWithDebInfo builds

## Common Tasks

**Add a new render pass:**
1. Create shader files in `src/shaders/`
2. Add to CMakeLists.txt glob patterns
3. Create pipeline/descriptor setup in `Application::InitializeVulkan()`
4. Record commands in `Application::DrawFrame()`

**Modify render options:**
- Add fields to `RenderOption` struct in `src/RenderOption.h`
- Expose in ImGui controls (search for ImGui::Checkbox/Slider calls)

**Load new models:**
- Place in `res/models/`
- Use Assimp-compatible formats (.obj, .gltf/.glb)
- Load via `Loader::Mesh` class
