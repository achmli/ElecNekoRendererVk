#pragma once

#include "Math/Matrix.h"
#include "RHI/Buffer.h"

namespace ElecNeko
{
    enum LightType
    {
        DistantLight,
        SphereLight,
        RectLight
    };

    struct Cam_t
    {
        Mat4 viewMat;
        Mat4 projMat;
    };

    struct LightParams_t
    {
        float position[3];
        float emission[3];
        float direction[3];
        float u[3];
        float v[3];
        int32_t lightType;

        float area;
        float radius;
        float padding0[2];
        float padding1[4];
        float padding2[4];
        float padding3[4];
    };

    class Light
    {
    public:
        virtual ~Light() = default;

        virtual void UpdateLight() = 0;

        bool MakeUBO(DeviceContext *device);
        bool UpdateUBO(DeviceContext *device);
        void Cleanup(DeviceContext *device);

    public:
        LightParams_t lightParams;

        Buffer uniformBuffer;
    };

    class PointLight : public Light
    {
    public:
        void UpdateLight() override;

    public:
        Vec3 position;
        Vec3 emission;
        float radius;
    };

    class QuadLight : public Light
    {
    public:
        void UpdateLight() override;

    public:
        Vec3 position;
        Vec3 emission;
        Vec3 u;
        Vec3 v;
        float area;
    };

    class DistantLight
    {
    public:
        void ComputeViewMatrix();
        void ComputeProjectionMatrix();
        void UpdateLight();

        bool MakeUBO(DeviceContext *device);
        bool UpdateUBO(DeviceContext *device);
        void Cleanup(DeviceContext *device);

    public:
        Vec3 position;
        Vec3 color;
        Vec3 direction;

        LightParams_t lightParams;
        Cam_t cam;

        Buffer uniformBuffer;
        Buffer matrixBuffer;
    };
} // namespace ElecNeko
