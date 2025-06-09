#pragma once

#include "Math/Matrix.h"

#include"RHI/Buffer.h"

namespace ElecNeko
{
    struct CameraUBO
    {
        Mat4 view;
        Mat4 proj;
    };

	class Camera
	{
    public:
        Camera() = default;
        Camera(const Vec3 &eye, const Vec3 &looKAt, const float _fov, const float _aspect, const float near, const float far);
        Camera(const Camera &other);
        Camera &operator=(const Camera &other);

        void Initialize(const Vec3 &eye, const Vec3 &looKAt, const float _fov, const float _aspect, const float near, const float far);

        void OffsetOrientation(const float dx, const float dy);
        void Strafe(const float dx, const float dy);
        void SetRadius(const float dr);

        Mat4 ComputeViewMatrix();
        Mat4 ComputeProjectionMatrix();

        void UpdateCamera();

    public:
        Vec3 position;
        Vec3 up;
        Vec3 right;
        Vec3 forward;

        Vec3 worldUp;
        Vec3 pivot;

        float focalDist;
        float aperture;
        float fov;
        float aspect;

        float zNear;
        float zFar;

        float yaw;
        float pitch;
        float radius;
	};
}