//
//  FrameBuffer.cpp
//
#include "FrameBuffer.h"
#include "../Fileio.h"
#include <assert.h>
#include <stdio.h>

#define DEPTH_FORMAT VK_FORMAT_D32_SFLOAT

/*
========================================================================================================

FrameBuffer

========================================================================================================
*/

/*
====================================================
FrameBuffer::Cleanup
====================================================
*/
void FrameBuffer::Cleanup( DeviceContext * device ) {
	if ( m_parms.hasDepth ) {
		m_imageDepth.Cleanup( device );
	}
	if ( m_parms.hasColor ) {
		m_imageColor.Cleanup( device );
	}

	vkDestroyFramebuffer( device->m_vkDevice, m_vkFrameBuffer, nullptr );
	vkDestroyRenderPass( device->m_vkDevice, m_vkRenderPass, nullptr );
}

/*
====================================================
FrameBuffer::Create
====================================================
*/
bool FrameBuffer::Create( DeviceContext * device, CreateParms_t & parms ) {
	VkResult result;

	m_parms = parms;

	std::vector< VkImageView > imageViews;

	//
	//	Create the color image
	//
	if ( parms.hasColor ) {
		Image::CreateParms_t parmsImage;
		parmsImage.width = m_parms.width;
		parmsImage.height = m_parms.height;
		parmsImage.depth = 1;
		parmsImage.format = VK_FORMAT_R8G8B8A8_UNORM;
		parmsImage.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if ( !m_imageColor.Create( device, parmsImage ) ) {
			printf( "ERROR: Failed to create color image\n" );
			assert( 0 );
			return false;
		}
		m_imageColor.TransitionLayout( device );
		imageViews.push_back( m_imageColor.m_vkImageView );
	}

	//
	//	Create the depth image
	//
	if ( parms.hasDepth ) {
		Image::CreateParms_t parmsImage;
		parmsImage.width = m_parms.width;
		parmsImage.height = m_parms.height;
		parmsImage.depth = 1;
		parmsImage.format = DEPTH_FORMAT;
		parmsImage.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if ( !m_imageDepth.Create( device, parmsImage ) ) {
			printf( "ERROR: Failed to create depth image\n" );
			assert( 0 );
			return false;
		}
		m_imageDepth.TransitionLayout( device );
		imageViews.push_back( m_imageDepth.m_vkImageView );
	}

	//
	//	Create RenderPass
	//
	if ( !CreateRenderPass( device ) ) {
		printf( "ERROR: Failed to create render pass\n" );
		assert( 0 );
		return false;
	}

	//
	//	Create Framebuffer
	//
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = m_vkRenderPass; 
	framebufferInfo.attachmentCount = (uint32_t)imageViews.size();
	framebufferInfo.pAttachments = imageViews.data();
	framebufferInfo.width = parms.width;
	framebufferInfo.height = parms.height;
	framebufferInfo.layers = 1;

	result = vkCreateFramebuffer( device->m_vkDevice, &framebufferInfo, nullptr, &m_vkFrameBuffer );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create framebuffer\n" );
		assert( 0 );
		return false;
	}

	return true;
}

/*
====================================================
FrameBuffer::CreateRenderPass
====================================================
*/
bool FrameBuffer::CreateRenderPass( DeviceContext * device ) {
	VkResult result;

	//
	//	Create color attachment descriptions
	//
	int numAttachments = 0;
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_imageColor.m_parms.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_imageColor.m_vkImageLayout = colorAttachment.finalLayout;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = numAttachments;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	if ( m_parms.hasColor ) {
		numAttachments++;
	}

	//
	//	Create depth attachment descriptions
	//
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = m_imageDepth.m_parms.format;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_imageDepth.m_vkImageLayout = depthAttachment.finalLayout;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = numAttachments;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	if ( m_parms.hasDepth ) {
		numAttachments++;
	}

	//
	//	Attachements
	//
	std::vector< VkAttachmentDescription > attachments;
	if ( m_parms.hasColor ) {
		attachments.push_back( colorAttachment );
	}
	if ( m_parms.hasDepth ) {
		attachments.push_back( depthAttachment );
	}

	//
	//	Subpasses
	//
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	if ( m_parms.hasColor ) {
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
	}
	if ( m_parms.hasDepth ) {
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
	}

	//
	//	Dependencies
	//
	VkSubpassDependency dependencies[ 2 ];
	if ( m_parms.hasColor ) {
		dependencies[ 0 ].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[ 0 ].dstSubpass = 0;
		dependencies[ 0 ].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[ 0 ].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[ 0 ].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[ 0 ].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[ 0 ].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[ 1 ].srcSubpass = 0;
		dependencies[ 1 ].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[ 1 ].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[ 1 ].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[ 1 ].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[ 1 ].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[ 1 ].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	}
	else {
		dependencies[ 0 ].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[ 0 ].dstSubpass = 0;
		dependencies[ 0 ].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[ 0 ].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[ 0 ].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[ 0 ].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[ 0 ].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[ 1 ].srcSubpass = 0;
		dependencies[ 1 ].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[ 1 ].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[ 1 ].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[ 1 ].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[ 1 ].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[ 1 ].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	}
	
	//
	//	Create
	//
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = (uint32_t)attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies;

	result = vkCreateRenderPass( device->m_vkDevice, &renderPassInfo, nullptr, &m_vkRenderPass );
	if ( VK_SUCCESS != result ) {
		printf( "ERROR: Failed to create render pass\n" );
		assert( 0 );
		return false;
	}

	return true;
}

/*
====================================================
FrameBuffer::BeginRenderPass
====================================================
*/
void FrameBuffer::BeginRenderPass( DeviceContext * device, const int cmdBufferIndex ) {
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_vkRenderPass;
	renderPassBeginInfo.framebuffer = m_vkFrameBuffer;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent.width = m_parms.width;
	renderPassBeginInfo.renderArea.extent.height = m_parms.height;

	std::vector< VkClearValue > clearValues;
	if ( m_parms.hasColor ) {
		VkClearValue value = {};
		value.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues.push_back( value );
	}
	if ( m_parms.hasDepth ) {
		VkClearValue value = {};
		value.depthStencil = { 1.0f, 0 };
		clearValues.push_back( value );
	}

	renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass( device->m_vkCommandBuffers[ cmdBufferIndex ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_parms.width;
	viewport.height = (float)m_parms.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport( device->m_vkCommandBuffers[ cmdBufferIndex ], 0, 1, &viewport );

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = m_parms.width;
	scissor.extent.height = m_parms.height;
	vkCmdSetScissor( device->m_vkCommandBuffers[ cmdBufferIndex ], 0, 1, &scissor );

	//
	//	If the frame buffer has no color attachment,
	//	then it's for shadow mapping. So set the
	//	bias and slope.
	//
	if ( m_parms.hasDepth && !m_parms.hasColor ) {
		float bias = 1.25f;
		float slope = 1.75f;
		vkCmdSetDepthBias( device->m_vkCommandBuffers[ cmdBufferIndex ], bias, 0.0f, slope );
	}
}

/*
====================================================
FrameBuffer::EndRenderPass
====================================================
*/
void FrameBuffer::EndRenderPass( DeviceContext * device, const int cmdBufferIndex ) {
	vkCmdEndRenderPass( device->m_vkCommandBuffers[ cmdBufferIndex ] );
}

namespace ElecNeko
{
	void GFrameBuffer::Cleanup(DeviceContext* device)
	{
	    m_imageAlbedo.Cleanup(device);

	    m_imageNormal.Cleanup(device);
	    m_imageDepth.Cleanup(device);

	    vkDestroyFramebuffer( device->m_vkDevice, m_vkFrameBuffer, nullptr );
	    vkDestroyRenderPass( device->m_vkDevice, m_vkRenderPass, nullptr );
	}

    bool GFrameBuffer::Create(DeviceContext *device, CreateParms_t &parms)
    {
        m_parms = parms;

	    std::vector<VkImageView> imageViews;

	    // Create albedo image
	    {
	        Image::CreateParms_t imageParms;
            imageParms.width = m_parms.width;
            imageParms.height = m_parms.height;
            imageParms.depth = 1;
            imageParms.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageParms.usageFlags =  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if(!m_imageAlbedo.Create(device,imageParms))
            {
                printf("ERROR: Failed to create Albedo image\n");
                assert(0);
                return false;
            }
	    }

	    m_imageAlbedo.TransitionLayout(device);
	    imageViews.push_back(m_imageAlbedo.m_vkImageView);

	    // create normal image
	    {
	        Image::CreateParms_t imageParms;
            imageParms.width = m_parms.width;
            imageParms.height = m_parms.height;
            imageParms.depth = 1;
            imageParms.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            imageParms.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if(!m_imageNormal.Create(device,imageParms))
            {
                printf("ERROR: Failed to create Normal image\n");
                assert(0);
                return false;
            }
	    }

	    m_imageNormal.TransitionLayout(device);
	    imageViews.push_back(m_imageNormal.m_vkImageView);

	    // create depth image
	    {
	        Image::CreateParms_t imageParms;
            imageParms.width = m_parms.width;
            imageParms.height = m_parms.height;
            imageParms.depth = 1;
            imageParms.format = DEPTH_FORMAT;
            imageParms.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            if(!m_imageDepth.Create(device,imageParms))
            {
                printf("ERROR: Failed to create Depth image\n");
                assert(0);
                return false;
            }
	    }

	    m_imageDepth.TransitionLayout(device);
	    imageViews.push_back(m_imageDepth.m_vkImageView);

	    // create render pass
        if(!CreateRenderPass(device))
        {
            printf("ERROR: Failed to create RenderPass\n");
            assert(0);
            return false;
        }
	    
	    // create framebuffer
	    VkFramebufferCreateInfo framebufferCreateInfo = {};
	    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	    framebufferCreateInfo.renderPass = m_vkRenderPass;
	    framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
	    framebufferCreateInfo.pAttachments = imageViews.data();
	    framebufferCreateInfo.width = m_parms.width;
	    framebufferCreateInfo.height = m_parms.height;
	    framebufferCreateInfo.layers = 1;

	    if(vkCreateFramebuffer(device->m_vkDevice,&framebufferCreateInfo,nullptr,&m_vkFrameBuffer) != VK_SUCCESS)
	    {
	        printf("ERROR: Failed to create framebuffer\n");
	        assert(0);
	        return false;
	    }

	    return true;
    }

    bool GFrameBuffer::CreateRenderPass(DeviceContext *device)
    {
        
	    int numAttachments = 3;
	    
	    // create normal attachment descriptions
	    VkAttachmentDescription normalAttachment = {};
	    normalAttachment.format=m_imageNormal.m_parms.format;
	    normalAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	    normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	    normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	    normalAttachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	    normalAttachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
	    normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	    normalAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	    m_imageNormal.m_vkImageLayout=normalAttachment.finalLayout;

	    VkAttachmentReference normalAttachmentRef = {};
	    normalAttachmentRef.attachment = numAttachments;
	    normalAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	    // create albedo attachment descriptions
	    VkAttachmentDescription colorAttachment = {};
	    colorAttachment.format=m_imageAlbedo.m_parms.format;
	    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	    colorAttachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	    colorAttachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
	    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	    m_imageAlbedo.m_vkImageLayout=colorAttachment.finalLayout;

	    VkAttachmentReference colorAttachmentRef = {};
	    colorAttachmentRef.attachment = numAttachments;
	    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	    VkAttachmentDescription depthAttachment = {};
	    depthAttachment.format=m_imageDepth.m_parms.format;
	    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	    depthAttachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	    depthAttachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
	    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	    m_imageDepth.m_vkImageLayout=depthAttachment.finalLayout;

	    VkAttachmentReference depthAttachmentRef = {};
	    depthAttachmentRef.attachment = numAttachments;
	    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	    // attachments
	    std::vector<VkAttachmentDescription> attachments;
	    std::vector<VkAttachmentReference> attachmentRefs;
	    {
	        attachments.push_back(normalAttachment);
	        attachmentRefs.push_back(normalAttachmentRef);
            attachments.push_back(colorAttachment);
            attachmentRefs.push_back(colorAttachmentRef);
	        attachments.push_back(depthAttachment);
	    }

	    // subpasses
	    VkSubpassDescription subpass = {};
	    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	    subpass.colorAttachmentCount = 2;
	    subpass.pColorAttachments = attachmentRefs.data();
	    subpass.pDepthStencilAttachment = &depthAttachmentRef;

	    // dependencies
	    VkSubpassDependency subpassDependency[2];
	    subpassDependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	    subpassDependency[0].dstSubpass = 0;
	    subpassDependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	    subpassDependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    subpassDependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	    subpassDependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	    subpassDependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	    
	    subpassDependency[1].srcSubpass = 0;
	    subpassDependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	    subpassDependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    subpassDependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	    subpassDependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	    subpassDependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	    subpassDependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	    // create
	    VkRenderPassCreateInfo renderPassInfo = {};
	    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	    renderPassInfo.pAttachments = attachments.data();
	    renderPassInfo.subpassCount = 1;
	    renderPassInfo.pSubpasses = &subpass;
	    renderPassInfo.dependencyCount=2;
	    renderPassInfo.pDependencies = subpassDependency;

	    if(vkCreateRenderPass(device->m_vkDevice,&renderPassInfo,nullptr,&m_vkRenderPass))
	    {
	        printf( "ERROR: Failed to create render pass\n" );
	        assert( 0 );
	        return false;
	    }

	    return true;
    }

    void GFrameBuffer::BeginRenderPass(DeviceContext *device, const int cmdBufferIndex)
    {
        VkRenderPassBeginInfo renderPassInfo = {};
	    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	    renderPassInfo.renderPass = m_vkRenderPass;
	    renderPassInfo.framebuffer = m_vkFrameBuffer;
	    renderPassInfo.renderArea.offset = { 0, 0 };
	    renderPassInfo.renderArea.extent.width = m_parms.width;
	    renderPassInfo.renderArea.extent.height = m_parms.height;

	    std::vector<VkClearValue> clearValues;
        {
            VkClearValue clearValue = {};
            clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValues.push_back(clearValue);
        }
        {
            VkClearValue clearValue = {};
            clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValues.push_back(clearValue);
        }
	    {
            VkClearValue clearValue = {};
            clearValue.depthStencil = {1.0f, 0};
            clearValues.push_back(clearValue);
	    }

	    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	    renderPassInfo.pClearValues = clearValues.data();

	    vkCmdBeginRenderPass(device->m_vkCommandBuffers[cmdBufferIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	    VkViewport viewport = {};
	    viewport.x = 0.0f;
	    viewport.y = 0.0f;
	    viewport.width = static_cast<float>(m_parms.width);
	    viewport.height = static_cast<float>(m_parms.height);
	    viewport.minDepth = 0.0f;
	    viewport.maxDepth = 1.0f;
	    vkCmdSetViewport(device->m_vkCommandBuffers[cmdBufferIndex], 0, 1, &viewport);

	    VkRect2D scissor = {};
	    scissor.offset = { 0, 0 };
	    scissor.extent.width = m_parms.width;
	    scissor.extent.height = m_parms.height;
	    vkCmdSetScissor(device->m_vkCommandBuffers[cmdBufferIndex], 0, 1, &scissor);
    }

    void GFrameBuffer::EndRenderPass(DeviceContext *device, const int cmdBufferIndex)
	{
	    vkCmdEndRenderPass(device->m_vkCommandBuffers[cmdBufferIndex]);
	}
}