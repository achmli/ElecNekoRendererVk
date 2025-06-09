#include "Camera.h"

#include "Math/LCP.h"

#include <cassert>

namespace ElecNeko
{
    Camera::Camera(const Vec3& eye, const Vec3& lookAt, const float _fov, const float _aspect, const float near, const float far)
    { 
        position = eye;
        pivot = lookAt;
        worldUp = (0, 1, 0);

        /*Vec3 dir = (pivot - position).Normalize();
        pitch = Degrees(asin(dir.y));
        yaw = Degrees(atan2f(dir.z, dir.x));*/

        forward = (lookAt - position).Normalize();

        yaw = Degrees(asin(forward.z));
        pitch = Degrees(atanf(forward.y / forward.x));

        radius = (eye - lookAt).GetMagnitude();

        fov = _fov;
        aspect = _aspect;
        focalDist = .1f;
        aperture = 0;

        zNear = near;
        zFar = far;

        UpdateCamera();
    }

    Camera::Camera(const Camera& other)
    { 
        position = other.position;
        up = other.up;
        right = other.right;
        forward = other.forward;

        worldUp = other.worldUp;
        pivot = other.pivot;

        focalDist = other.focalDist;
        aperture = other.aperture;
        fov = other.fov;
        aspect = other.aspect;

        zNear = other.zNear;
        zFar = other.zFar;

        yaw = other.yaw;
        pitch = other.pitch;
        radius = other.radius;

        UpdateCamera();
    }

    Camera& Camera::operator=(const Camera& other)
    {
        position = other.position;
        up = other.up;
        right = other.right;
        forward = other.forward;

        worldUp = other.worldUp;
        pivot = other.pivot;

        focalDist = other.focalDist;
        aperture = other.aperture;
        fov = other.fov;
        aspect = other.aspect;

        zNear = other.zNear;
        zFar = other.zFar;

        yaw = other.yaw;
        pitch = other.pitch;
        radius = other.radius;

        UpdateCamera();

        return *this;
    }

    void Camera::Initialize(const Vec3 &eye, const Vec3 &lookAt, const float _fov, const float _aspect, const float near, const float far)
    {
        position = eye;
        pivot = lookAt;
        worldUp = (0, 1, 0);

        /*Vec3 dir = (pivot - position).Normalize();
        pitch = Degrees(asin(dir.y));
        yaw = Degrees(atan2f(dir.z, dir.x));*/

        forward = (lookAt - position).Normalize();

        yaw = Degrees(asin(forward.z));
        pitch = Degrees(atanf(forward.y / forward.x));

        radius = (eye - lookAt).GetMagnitude();

        fov = _fov;
        aspect = _aspect;
        focalDist = .1f;
        aperture = 0;

        zNear = near;
        zFar = far;

        UpdateCamera();
    }

    void Camera::OffsetOrientation(const float dx, const float dy)
    { 
        yaw -= dx;
        pitch -= dy;

        pitch=std::clamp(pitch, -89.f, 89.f);

        UpdateCamera();
    }

    void Camera::Strafe(const float dx, const float dy) 
    { 
        pivot += (right * -dx) + up * dy;
        UpdateCamera();
    }

    void Camera::SetRadius(const float dr)
    { 
        radius += dr;
        UpdateCamera();
    }

    void Camera::UpdateCamera() 
    { 
        float yawRad = Radians(yaw);
        float pitchRad = Radians(pitch);

        forward = Vec3(cos(pitchRad) * cos(yawRad), cos(pitchRad) * sin(yawRad), sin(pitchRad)).Normalize();
        right = forward.Cross(Vec3(0, 0, 1)).Normalize();
        up = right.Cross(forward).Normalize();

        // position = pivot + (forward * -1.f) * radius;
    }

    Mat4 Camera::ComputeViewMatrix()
    { 
        Mat4 matView;

        matView.LookAt(position, position + forward, up);
        //matView = matView.Transpose();

        return matView.Transpose();
    }

    Mat4 Camera::ComputeProjectionMatrix()
    { 
        Mat4 matProj;

        matProj.PerspectiveVulkan(fov, aspect, zNear, zFar);
        
        return matProj.Transpose();
    }
}