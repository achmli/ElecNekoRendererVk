//
//  Descriptor.h
//
#pragma once
#include <vulkan/vulkan.hpp>

class DeviceContext;
class Buffer;
class Pipeline;
class Image;
struct RenderModel;

namespace RHI
{
    class Buffer;
    class Texture;
    class Sampler;
    class BufferBinding;
    class SampledTextureBinding;
    class BindingSetDesc;
    class BindingSet;
} // namespace RHI

namespace ElecNeko
{
    class ElecNekoPipeline;
}

/*
====================================================
Descriptor
====================================================
*/
class Descriptor
{
public:
    Descriptor();
    ~Descriptor() {}

    void BindImage(VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler, int slot);
    void BindBuffer(Buffer *uniformBuffer, int offset, int size, int slot);
    void BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, Pipeline *pso);
    void BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, ElecNeko::ElecNekoPipeline *pso);

    friend class Descriptors;

private:
    Descriptors *m_parent;

    int m_id; // the id of the descriptor set to be used

    int m_numBuffers;
    static const int MAX_BUFFERS = 16;
    VkDescriptorBufferInfo m_bufferInfo[MAX_BUFFERS];

    int m_numImages;
    static const int MAX_IMAGEINFO = 16;
    VkDescriptorImageInfo m_imageInfo[MAX_IMAGEINFO];
};

/*
====================================================
DescriptorSets
====================================================
*/
class Descriptors
{
public:
    Descriptors() : m_numDescriptorUsed(0) {}
    ~Descriptors() {}

    // This structure creates the layout
    struct CreateParms_t
    {
        int numUniformsVertex;
        int numUniformsFragment;
        int numImageSamplers;
    };
    CreateParms_t m_parms;

    bool Create(DeviceContext *device, const CreateParms_t &parms);
    bool ElecNekoCreate(DeviceContext *device, const CreateParms_t &parms);
    void Cleanup(DeviceContext *device);

    static const int MAX_DESCRIPTOR_SETS = 256;

    VkDescriptorPool m_vkDescriptorPool;
    VkDescriptorSetLayout m_vkDescriptorSetLayout;

    // The individual descriptor sets
    int m_numDescriptorUsed;
    VkDescriptorSet m_vkDescriptorSets[MAX_DESCRIPTOR_SETS];


    Descriptor GetFreeDescriptor()
    {
        Descriptor descriptor;
        descriptor.m_parent = this;
        descriptor.m_id = m_numDescriptorUsed % MAX_DESCRIPTOR_SETS;
        m_numDescriptorUsed++;
        return descriptor;
    }
};


namespace ElecNeko
{
    class ElecNekoDescriptor
    {
    public:
        ElecNekoDescriptor() : m_parent(nullptr), m_id(-1) {}
        ~ElecNekoDescriptor() = default;

        //
        void BindUniformBuffer(int bindingPoint, Buffer *buffer, int offset, int size);
        void BindUniformBuffer(int bindingPoint, RHI::Buffer *buffer, int offset, int size);
        void BindUniformBuffer(const RHI::BufferBinding &binding);
        void BindStorageBuffer(int bindingPoint, Buffer *buffer, int offset, int size);
        void BindStorageBuffer(const RHI::BufferBinding &binding);
        // void BindStorageBuffer(uint32_t binding, RHI::Buffer *buffer, VkDeviceSize offset, VkDeviceSize range);
        void BindStorageBuffer(int bindingPoint, RHI::Buffer *buffer, int offset, int size);
        void BindImage(int bindingPoint, VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler);
        void BindImage(int bindingPoint, VkImageLayout imageLayout, RHI::Texture *texture, VkSampler sampler);
        void BindImage(int bindingPoint, VkImageLayout imageLayout, RHI::Texture *texture, RHI::Sampler *sampler);
        void BindSampledTexture(int bindingPoint, RHI::Texture *texture, RHI::Sampler *sampler);
        void BindSampledTexture(const RHI::SampledTextureBinding &binding);
        void BindBindingSet(const RHI::BindingSetDesc &desc);
        //
        void BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, Pipeline *pso);
        void BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, ElecNekoPipeline *pso);

        friend class ElecNekoDescriptors;

    private:
        //
        struct DescriptorBinding
        {
            int bindingPoint;
            VkDescriptorType type;
            union
            {
                VkDescriptorBufferInfo bufferInfo;
                VkDescriptorImageInfo imageInfo;
            };
            bool isImage;
        };

        ElecNekoDescriptors *m_parent;
        int m_id;

        std::vector<DescriptorBinding> m_bindings;
    };

    class ElecNekoDescriptors
    {
    public:
        ElecNekoDescriptors() : m_numDescriptorUsed(0) {}
        ~ElecNekoDescriptors() = default;

        struct CreateParms_t
        {
            int numUniformsVertex;
            int numUniformsFragment;
            int numStorageVertex;
            int numStorageFragment;
            int numImageSamplers;
        };
        CreateParms_t m_parms;

        bool Create(DeviceContext *device, const CreateParms_t &parms);
        void Cleanup(DeviceContext *device);

        static const int MAX_DESCRIPTOR_SETS = 256;

        VkDescriptorPool m_vkDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_vkDescriptorSetLayout = VK_NULL_HANDLE;


        int m_numDescriptorUsed;
        VkDescriptorSet m_vkDescriptorSets[MAX_DESCRIPTOR_SETS];

        ElecNekoDescriptor GetFreeDescriptor()
        {
            ElecNekoDescriptor descriptor;
            descriptor.m_parent = this;
            descriptor.m_id = m_numDescriptorUsed % MAX_DESCRIPTOR_SETS;
            m_numDescriptorUsed++;
            return descriptor;
        }
    };

    class ElecNekoDescriptorCompute
    {
    public:
        ElecNekoDescriptorCompute() : m_parent(nullptr), m_id(-1) {}
        ~ElecNekoDescriptorCompute() = default;

        void BindingUniform(int bindingPoint, Buffer *buffer, int offset, int size);
        void BindingStorageBuffer(int bindingPoint, Buffer *buffer, int offset, int size);
        void BindingStorageImage(int bindingPoint, VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler);
        void BindingSampledImage(int bindingPoint, VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler);

        void BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, ElecNekoPipeline *pso);

        friend class ElecNekoDescriptorsCompute;

    private:
        struct DescriptorBindings
        {
            int bindingPoint;
            VkDescriptorType type;
            union
            {
                VkDescriptorBufferInfo bufferInfo;
                VkDescriptorImageInfo imageInfo;
            };
            bool isImage;
        };

        ElecNekoDescriptorsCompute *m_parent = nullptr;
        int m_id;

        std::vector<DescriptorBindings> m_bindings;
    };

    class ElecNekoDescriptorsCompute
    {
    public:
        ElecNekoDescriptorsCompute() = default;
        ~ElecNekoDescriptorsCompute() = default;

        struct CreateParms_t
        {
            int numUniforms = 0;
            int numStorageBuffers = 0;
            int numStorageImages = 0;
            int numSampledImages = 0;
            int maxSets = 64;
        };

        bool Create(DeviceContext *device, const CreateParms_t &parms);
        void Cleanup(DeviceContext *device);

        ElecNekoDescriptorCompute GetFreeDescriptor();

    public:
        CreateParms_t m_parms;

        static const int MAX_DESCRIPTOR_SETS = 256;

        VkDescriptorPool m_vkDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_vkDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet m_vkDescriptorSets[MAX_DESCRIPTOR_SETS];
        int m_totalBindings = 0;
        int m_allocatedCount = 0;
    };
} // namespace ElecNeko
