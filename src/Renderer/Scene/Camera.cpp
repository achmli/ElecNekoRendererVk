//
// Created by ElecNekoSurface on 25-3-17.
//

#include "Camera.h"
#include <iostream>
#include <string>

namespace ElecNeko
{
    Camera::Camera(const Vec3 &eye, const Vec3 &lookAt, const float fieldOfView)
    {
        position = eye;
        pivot = lookAt;
        worldUp = Vec3(0, 1, 0);

        Vec3 dir = (pivot - position).Normalize();
        pitch = Degrees(asin(dir.y));
        yaw = Degrees(atan2(dir.z, dir.x));

        radius = (eye - lookAt).GetMagnitude();

        fov = Radians(fieldOfView);
        focalDist = .1f;
        aperture = 0.f;
        UpdateCamera();
    }

    Camera &Camera::operator=(const Camera &other)
    {
        if (this != &other)
        {
            position = other.position;
            up = other.up;
            right = other.right;
            forward = other.forward;
            worldUp = other.worldUp;
            pivot = other.pivot;
            pitch = other.pitch;
            yaw = other.yaw;
            radius = other.radius;
            focalDist = other.focalDist;
            aperture = other.aperture;
            fov = other.fov;
        }

        return *this;
    }

    void Camera::OffsetOrientation(const float dx, const float dy)
    {
        pitch -= dy;
        yaw += dx;
        UpdateCamera();
    }

    void Camera::Strafe(const float dx, const float dy)
    {
        const Vec3 translation = right * -dx + up * dy;
        pivot = pivot + translation;
        UpdateCamera();
    }

    void Camera::SetRadius(const float dr)
    {
        radius += dr;
        UpdateCamera();
    }

    void Camera::SetFov(const float val) { fov = Radians(val); }

    void Camera::UpdateCamera()
    {
        Vec3 forwardTemp;
        forwardTemp.x = cos(Radians(yaw)) * cos(Radians(pitch));
        forwardTemp.y = sin(Radians(pitch));
        forwardTemp.z = sin(Radians(yaw)) * cos(Radians(pitch));

        forward = forwardTemp.Normalize();
        position = pivot + (forward * -1.f) * radius;

        right = forward.Cross(worldUp).Normalize();
        up = right.Cross(forward).Normalize();
    }

    void Camera::ComputeViewProjectionMatrix(Mat4 &view, Mat4 &projection, const float ratio) const
    {
        view.Zero();
        Vec3 at = position + forward;
        view.LookAt(position, at, up);

        projection.Zero();
        const float fov_v = (1.f / ratio) * tanf(fov / 2.f);
        projection.PerspectiveVulkan(fov_v, ratio, 0.1f, 1000.f);
    }
} // namespace ElecNeko
