#include "Light.h"

#include <iostream>

namespace ElecNeko
{
    bool Light::MakeUBO(DeviceContext* device)
    { 
        int bufferSizeU = sizeof(lightParams);

        if (!uniformBuffer.Allocate(device, nullptr, bufferSizeU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
        {
            std::cerr << "failed to allocate light uniforms\n";
            return false;
        }

        return true;
    }

    bool Light::UpdateUBO(DeviceContext* device) 
    { 
        UpdateLight();

        {
            uint8_t *mappedBuffer = (uint8_t *) uniformBuffer.MapBuffer(device);
            
            memcpy(mappedBuffer, &lightParams, sizeof(lightParams));

            uniformBuffer.UnmapBuffer(device);
        }

        return true;
    }

    void Light::Cleanup(DeviceContext* device)
    {
        uniformBuffer.Cleanup(device);
    }

    void PointLight::UpdateLight() 
    {
        lightParams.position[0] = position.x;
        lightParams.position[1] = position.y;
        lightParams.position[2] = position.z;

        lightParams.emission[0] = emission.x;
        lightParams.emission[1] = emission.y;
        lightParams.emission[2] = emission.z;

        lightParams.direction[0] = 0;
        lightParams.direction[1] = 0;
        lightParams.direction[2] = 0;

        lightParams.u[0] = 0;
        lightParams.u[1] = 0;
        lightParams.u[2] = 0;

        lightParams.v[0] = 0;
        lightParams.v[1] = 0;
        lightParams.v[2] = 0;

        lightParams.lightType = LightType::SphereLight;

        lightParams.area = 0;

        lightParams.radius = radius;
    }

    void QuadLight::UpdateLight()
    {
        lightParams.position[0] = position.x;
        lightParams.position[1] = position.y;
        lightParams.position[2] = position.z;

        lightParams.emission[0] = emission.x;
        lightParams.emission[1] = emission.y;
        lightParams.emission[2] = emission.z;

        lightParams.direction[0] = 0;
        lightParams.direction[1] = 0;
        lightParams.direction[2] = 0;

        lightParams.u[0] = u.x;
        lightParams.u[1] = u.y;
        lightParams.u[2] = u.z;

        lightParams.v[0] = v.x;
        lightParams.v[1] = v.y;
        lightParams.v[2] = v.z;

        lightParams.lightType = LightType::RectLight;
        lightParams.area = area;
    }

    void DistantLight::ComputeViewMatrix()
    { 
        Vec3 pos = position;
        Vec3 lookAt = position + direction;

        Vec3 forward = direction.Normalize();

        Vec3 right = forward.Cross(Vec3(0, 1, 0)).Normalize();
        Vec3 up = right.Cross(forward).Normalize();
        right = forward.Cross(up).Normalize();

        cam.viewMat.LookAt(position, lookAt, up);
        cam.viewMat = cam.viewMat.Transpose();
    }

    void DistantLight::ComputeProjectionMatrix() 
    { 
        cam.projMat.OrthoVulkan(-60.f, 60.f, -60.f, 60.f, 25, 175);
        cam.projMat = cam.projMat.Transpose();
    }

    void DistantLight::UpdateLight() 
    { 
        lightParams.position[0] = position.x;
        lightParams.position[1] = position.y;
        lightParams.position[2] = position.z;

        lightParams.emission[0] = color.x;
        lightParams.emission[1] = color.y;
        lightParams.emission[2] = color.z;

        lightParams.direction[0] = direction.x;
        lightParams.direction[1] = direction.y;
        lightParams.direction[2] = direction.z;

        lightParams.u[0] = 0;
        lightParams.u[1] = 0;
        lightParams.u[2] = 0;

        lightParams.v[0] = 0;
        lightParams.v[1] = 0;
        lightParams.v[2] = 0;

        lightParams.lightType = LightType::DistantLight;
    }

    bool DistantLight::MakeUBO(DeviceContext* device)
    {
        {
            int bufferSize = sizeof(lightParams);
            
            if (!uniformBuffer.Allocate(device, nullptr, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
            {
                std::cerr << "failed to allocate light uniforms\n";
                return false;
            }
        }

        {
            int bufferSize = sizeof(cam);

            if (!matrixBuffer.Allocate(device, nullptr, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
            {
                std::cerr << "failed to allocate light shadow marices\n";
                return false;
            }
        }

        return true;
    }

    bool DistantLight::UpdateUBO(DeviceContext* device)
    { 
        UpdateLight();

        ComputeViewMatrix();
        ComputeProjectionMatrix();

        {
            uint8_t *mappedData = (uint8_t *) uniformBuffer.MapBuffer(device);
            memcpy(mappedData, &lightParams, sizeof(lightParams));
            uniformBuffer.UnmapBuffer(device);
        }

        {
            uint8_t *mappedData = (uint8_t *) matrixBuffer.MapBuffer(device);
            memcpy(mappedData, &cam, sizeof(cam));
            matrixBuffer.UnmapBuffer(device);
        }

        return true;
    }

    void DistantLight::Cleanup(DeviceContext* device)
    { 
        uniformBuffer.Cleanup(device);
        matrixBuffer.Cleanup(device);
    }
}