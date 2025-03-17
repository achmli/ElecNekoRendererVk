#pragma once

#include "Math/LCP.h"

namespace ElecNeko
{
    class Camera
    {
    public:
        Camera(const Vec3 &eye, const Vec3 &lookAt, float fieldOfView);
        Camera(const Camera &other) = default;
        Camera &operator=(const Camera &other);

        void OffsetOrientation(float dx, float dy);
        void Strafe(float dx, float dy);
        void SetRadius(float dr);
        void ComputeViewProjectionMatrix(Mat4 &view, Mat4 &projection, float ratio) const;
        void SetFov(float val);

    public:
        Vec3 position;
        Vec3 up;
        Vec3 right;
        Vec3 forward;

        float focalDist;
        float aperture;
        float fov;

    private:
        void UpdateCamera();

        Vec3 worldUp;
        Vec3 pivot;

        float pitch;
        float radius;
        float yaw;
    };
} // namespace ElecNeko
