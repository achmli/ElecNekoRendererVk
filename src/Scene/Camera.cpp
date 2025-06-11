#include "Camera.h"

#include "Math/LCP.h"
#include <cmath>
#include "Math/Matrix.h"
#include "Math/Vector.h"

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

    void Camera::Initialize(const Vec3& eye, const Vec3& lookAt, const float _fov, const float _aspect, const float near, const float far)
    {
        position = eye;
        pivot = lookAt;
        worldUp = (0, 1, 0);

        forward = (lookAt - eye).Normalize();

        yaw = Degrees(asin(forward.z));
        pitch = Degrees(atan2f(forward.y, forward.x));

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
        // yaw -= dx;

        yaw = fmod(yaw + dx, 360.f);
        pitch -= dy;

        pitch=std::clamp(pitch, -89.9f, 89.9f);

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

        // Vec3 lookat = forward + position;

        forward = Vec3(cos(pitchRad) * cos(yawRad), sin(pitchRad), cos(pitchRad) * sin(yawRad)).Normalize();
        right = forward.Cross(Vec3(0, 1, 0)).Normalize();
        up = right.Cross(forward).Normalize();
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

    void OrthoCamera::Initialize(const Vec3& eye, const Vec3& lookAt, const float _width, const float _height, const float near, const float far)
    {
        position = eye;

        forward = (lookAt - eye).Normalize();

        zNear = near;
        zFar = far;

        yaw = asin(forward.z);
        pitch = atan2f(forward.y, forward.x);

        zNear = near;
        zFar = far;

        width = _width;
        height = _height;

        // forward = Vec3(cos(pitch) * cos(yaw), sin(pitch), cos(pitch) * sin(yaw)).Normalize();
        right = forward.Cross(Vec3(0, 1, 0)).Normalize();
        up = right.Cross(forward).Normalize();
    }

    Mat4 OrthoCamera::ComputeViewMatrix()
    {
        Mat4 matView;

        matView.LookAt(position, position + forward, up);

        return matView.Transpose();
    }

    Mat4 OrthoCamera::ComputeProjctionMatrix()
    { 
        float halfWidth = 60.f;

        Mat4 matProj;

        matProj.OrthoVulkan(-halfWidth, halfWidth, -halfWidth, halfWidth, zNear, zFar);

        return matProj.Transpose();
    }

    void OrthoCamera::UpdateCamera()
    {
        forward = Vec3(cos(pitch) * cos(yaw), sin(pitch), cos(pitch) * sin(yaw)).Normalize();
        right = forward.Cross(Vec3(0, 1, 0)).Normalize();
        up = right.Cross(forward).Normalize();
    }

    void OrthoCamera::OffsetOrientation(const float dx, const float dy)
    {
        // yaw -= dx;

        yaw = fmod(yaw + dx, 360.f);
        pitch -= dy;

        pitch = std::clamp(pitch, -89.9f, 89.9f);

        UpdateCamera();
    }
}