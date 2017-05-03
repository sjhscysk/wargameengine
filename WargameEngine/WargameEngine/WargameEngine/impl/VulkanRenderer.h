#pragma once
#include "IOpenGLRenderer.h"
#include "MatrixManagerGLM.h"
#include "VulkanHelpers.h"
#include "VulkanShaderManager.h"
#include <deque>
#include <map>
#include <vulkan/vulkan.h>
#include "VulkanPipelineManager.h"

class CVulkanRenderer;

class CCommandBufferWrapper
{
public:
	CCommandBufferWrapper(VkCommandPool pool, CVulkanRenderer& renderer);
	CCommandBufferWrapper(const CCommandBufferWrapper& other) = delete;
	CCommandBufferWrapper(CCommandBufferWrapper&& other) = default;
	~CCommandBufferWrapper();
	operator VkCommandBuffer() const { return m_commandBuffer; }

	void WaitFence();
	void Begin();
	void End();

	VkFence GetFence() const { return m_fence; }
	VkSemaphore GetImageAvailibleSemaphore() const { return m_imageAvailibleSemaphore; }
	VkSemaphore GetRenderingFinishedSemaphore() const { return m_renderingFinishedSemaphore; }
	VkFramebuffer GetFrameBuffer() const { return m_frameBuffer; }
	void SetFrameBuffer(VkFramebuffer buffer) { m_frameBuffer = buffer; }
	CVulkanSmartBuffer& GetVertexBuffer() { return m_vertexBuffer; }
	CVulkanSmartBuffer& GetUniformBuffer() { return m_uniformBuffer; }

private:
	VkCommandBuffer m_commandBuffer;
	CHandleWrapper<VkSemaphore, vkDestroySemaphore> m_imageAvailibleSemaphore;
	CHandleWrapper<VkSemaphore, vkDestroySemaphore> m_renderingFinishedSemaphore;
	CHandleWrapper<VkFence, vkDestroyFence> m_fence;
	VkDevice m_device;
	VkCommandPool m_pool;
	CHandleWrapper<VkFramebuffer, vkDestroyFramebuffer> m_frameBuffer;
	CVulkanSmartBuffer m_vertexBuffer;
	CVulkanSmartBuffer m_uniformBuffer;
};

class CVulkanCachedTexture : public ICachedTexture
{
public:
	CVulkanCachedTexture(CVulkanRenderer& renderer);
	~CVulkanCachedTexture();
	void Init(uint32_t width, uint32_t height, CVulkanMemoryManager& memoryManager, CachedTextureType type = CachedTextureType::RGBA, int flags = 0);
	void Upload(const void* data, CVulkanMemoryManager& memoryManager, VkCommandBuffer commandBuffer);
	operator VkImage() const { return m_image; }
	VkImageView GetImageView() const { return m_imageView; }
	VkSampler GetSampler() const { return m_sampler; }
	VkFormat GetFormat() const { return m_format; }
	void TransferTo(VkImageLayout newLayout, VkCommandBuffer commandBuffer) { TransferImageLayout(m_image, commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, newLayout); }

private:
	static void TransferImageLayout(VkImage stageImage, VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);
	std::pair<VkImage, std::unique_ptr<CVulkanMemory>> CreateTexture(bool deviceLocal, CVulkanMemoryManager& memoryManager);
	VkImage m_image = VK_NULL_HANDLE;
	std::unique_ptr<CVulkanMemory> m_memory;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;
	VkExtent3D m_extent = { 0, 0, 0 };
	VkDevice m_device;
	uint32_t m_components = 0;
	CVulkanRenderer* m_renderer = nullptr;
	VkFormat m_format;
	VkImageUsageFlags m_usageFlags;
};

class CSwapchainWrapper
{
public:
	void Init(VkSwapchainKHR swapchain, VkDevice device, VkExtent2D extent, VkFormat format, CVulkanRenderer* renderer);
	~CSwapchainWrapper();
	operator VkSwapchainKHR() const { return m_swapchain; }
	size_t GetImagesCount() const { return m_images.size(); }
	const std::vector<VkImage>& GetImages() const { return m_images; }
	VkExtent2D GetExtent() const { return m_extent; }
	VkImageView GetImageView(size_t index) const { return m_imageViews[index]; }
	VkFormat GetFormat() const { return m_format; }
	CVulkanCachedTexture& GetDepthTexture() const { return *m_depthTexture; }
	void DestroyDepthTexture() { m_depthTexture.reset(); }

private:
	CHandleWrapper<VkSwapchainKHR, vkDestroySwapchainKHR> m_swapchain;
	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;
	std::unique_ptr<CVulkanCachedTexture> m_depthTexture;
	VkExtent2D m_extent;
	VkFormat m_format;
	CVulkanRenderer* m_renderer;
};

class CVulkanOcclusionQuery : public IOcclusionQuery
{
public:
	void Query(std::function<void()> const& handler, bool /*renderToScreen*/) override { handler(); }
	bool IsVisible() const override { return true; }
};

class CVulkanRenderer : public IOpenGLRenderer
{
public:
	CVulkanRenderer(const std::vector<const char*>& instanceExtensions);
	~CVulkanRenderer();

	VkInstance GetInstance() const;
	VkDevice GetDevice() const { return m_device; }
	VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
	CVulkanMemoryManager& GetMemoryManager() { return *m_memoryManager; }
	void SetSurface(VkSurfaceKHR surface);
	void Resize();
	void AcquireImage();
	void Present();
	VkCommandBuffer GetCommandBuffer() const { return *m_activeCommandBuffer; }
	CVulkanPipelineManager& GetPipelineHelper() { return m_pipelineHelper; }
	VkBuffer GetEmptyBuffer() const { return *m_emptyBuffer; }
	CVulkanSmartBuffer& GetVertexBuffer() const { return m_activeCommandBuffer->GetVertexBuffer(); }
	void DestroyImage(CVulkanCachedTexture* texture, VkImage image = VK_NULL_HANDLE, VkImageView view = VK_NULL_HANDLE);
	void DestroyBuffer(VkBuffer buffer);

	//IRenderer
	void RenderArrays(RenderMode mode, array_view<CVector3f> const& vertices, array_view<CVector3f> const& normals, array_view<CVector2f> const& texCoords) override;
	void RenderArrays(RenderMode mode, array_view<CVector2i> const& vertices, array_view<CVector2f> const& texCoords) override;
	void DrawIndexes(IVertexBuffer& buffer, size_t begin, size_t count) override;
	void DrawAll(IVertexBuffer& buffer, size_t count) override;
	void DrawInstanced(IVertexBuffer& buffer, size_t size, size_t instanceCount) override;

	void PushMatrix() override;
	void PopMatrix() override;
	void Translate(const float dx, const float dy, const float dz) override;
	void Translate(const double dx, const double dy, const double dz) override;
	void Translate(const int dx, const int dy, const int dz) override;
	void Rotate(const double angle, const double x, const double y, const double z) override;
	void Scale(const double scale) override;
	void GetViewMatrix(float* matrix) const override;
	void LookAt(CVector3f const& position, CVector3f const& direction, CVector3f const& up) override;

	void SetTexture(const Path& texture, bool forceLoadNow = false, int flags = 0) override;
	void SetTexture(const Path& texture, TextureSlot slot, int flags = 0) override;
	void SetTexture(const Path& texture, const std::vector<sTeamColor>* teamcolor, int flags = 0) override;
	void SetTexture(ICachedTexture const& texture, TextureSlot slot = TextureSlot::eDiffuse) override;
	void UnbindTexture(TextureSlot slot = TextureSlot::eDiffuse) override;
	void RenderToTexture(std::function<void()> const& func, ICachedTexture& texture, unsigned int width, unsigned int height) override;
	std::unique_ptr<ICachedTexture> CreateTexture(const void* data, unsigned int width, unsigned int height, CachedTextureType type = CachedTextureType::RGBA) override;
	ICachedTexture* GetTexturePtr(const Path& texture) const override;

	void SetColor(const float r, const float g, const float b, const float a = 1.0f) override;
	void SetColor(const int r, const int g, const int b, const int a = UCHAR_MAX) override;
	void SetColor(const float* color) override;
	void SetColor(const int* color) override;
	void SetMaterial(const float* ambient, const float* diffuse, const float* specular, const float shininess) override;

	std::unique_ptr<IVertexBuffer> CreateVertexBuffer(const float* vertex = nullptr, const float* normals = nullptr, const float* texcoords = nullptr, size_t size = 0, bool temp = false) override;
	std::unique_ptr<IOcclusionQuery> CreateOcclusionQuery() override;
	std::string GetName() const override;
	bool SupportsFeature(Feature feature) const override;
	IShaderManager& GetShaderManager() override;

	//ITextureHelper
	std::unique_ptr<ICachedTexture> CreateEmptyTexture(bool cubemap = false) override;
	void SetTextureAnisotropy(float value = 1.0f) override;
	void UploadTexture(ICachedTexture& texture, unsigned char* data, size_t width, size_t height, unsigned short bpp, int flags, TextureMipMaps const& mipmaps = TextureMipMaps()) override;
	void UploadCompressedTexture(ICachedTexture& texture, unsigned char* data, size_t width, size_t height, size_t size, int flags, TextureMipMaps const& mipmaps = TextureMipMaps()) override;
	void UploadCubemap(ICachedTexture& texture, TextureMipMaps const& sides, unsigned short bpp, int flags) override;

	bool Force32Bits() const override;
	bool ForceFlipBMP() const override;
	bool ConvertBgra() const override;

	//IViewHelper
	std::unique_ptr<IFrameBuffer> CreateFramebuffer() const override;
	void SetTextureManager(CTextureManager& textureManager) override;

	void WindowCoordsToWorldVector(IViewport& viewport, int x, int y, CVector3f& start, CVector3f& end) const override;
	void WorldCoordsToWindowCoords(IViewport& viewport, CVector3f const& worldCoords, int& x, int& y) const override;
	void SetNumberOfLights(size_t count) override;
	void SetUpLight(size_t index, CVector3f const& position, const float* ambient, const float* diffuse, const float* specular) override;
	float GetMaximumAnisotropyLevel() const override;
	void GetProjectionMatrix(float* matrix) const override;
	void EnableDepthTest(bool enable) override;
	void EnableBlending(bool enable) override;
	void SetUpViewport(unsigned int viewportX, unsigned int viewportY, unsigned int viewportWidth, unsigned int viewportHeight, float viewingAngle, float nearPane = 1.0f, float farPane = 1000.0f) override;
	void EnablePolygonOffset(bool enable, float factor = 0.0f, float units = 0.0f) override;
	void ClearBuffers(bool color = true, bool depth = true) override;
	void DrawIn2D(std::function<void()> const& drawHandler) override;

	void EnableMultisampling(bool enable) override;

private:
	void CreateDeviceAndQueues();
	void CreateSwapchain();
	void CreateCommandBuffers();
	VkRenderPass CreateRenderPass(VkFormat format, VkFormat depthFormat = VK_FORMAT_UNDEFINED);
	void InitFramebuffer(bool useDepth);
	void FreeResources(bool force);
	void BeginServiceCommandBuffer();
	void BeforeDraw(VkPrimitiveTopology topology);

	CInstanceWrapper<VkInstance, vkDestroyInstance> m_instance;
	CDestructor m_debugCallbackDestructor;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	CInstanceWrapper<VkDevice, vkDestroyDevice> m_device;
	std::unique_ptr<CVulkanMemoryManager> m_memoryManager;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue m_presentQueue = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	CDestructor m_surfaceDestructor;
	CSwapchainWrapper m_swapchain;
	CHandleWrapper<VkCommandPool, vkDestroyCommandPool> m_commandPool;
	CHandleWrapper<VkRenderPass, vkDestroyRenderPass> m_renderPass;
	CHandleWrapper<VkRenderPass, vkDestroyRenderPass> m_serviceRenderPass;
	std::vector<CCommandBufferWrapper> m_commandBuffers;
	std::unique_ptr<CCommandBufferWrapper> m_serviceCommandBuffer;
	bool m_serviceBufferIsActive = false;
	CCommandBufferWrapper* m_activeCommandBuffer = nullptr;
	CVulkanDescriptorSetManager m_descriptorSetManager;
	VkDebugReportCallbackEXT m_debugCallback;
	std::unique_ptr<CVulkanCachedTexture> m_emptyTexture;
	VkImage m_currentImage;
	uint32_t m_currentImageIndex = 0;
	uint32_t m_graphicsQueueFamilyIndex = 0;
	uint32_t m_presentQueueFamilyIndex = 0;
	size_t m_currentCommandBufferIndex = 0;
	CVulkanShaderManager m_shaderManager;
	CVulkanPipelineManager m_pipelineHelper;
	std::unique_ptr<IShaderProgram> m_defaultProgram;
	std::unique_ptr<CVulkanVertexAttribCache> m_emptyBuffer;
	VkViewport m_viewport;
	CTextureManager* m_textureManager = nullptr;
	CMatrixManagerGLM m_matrixManager;
	std::deque<std::pair<VkImage, int>> m_imagesToDestroy;
	std::deque<std::pair<VkImageView, int>> m_imageViewsToDestroy;
	std::deque<std::pair<VkSampler, int>> m_samplersToDestroy;
	std::deque<std::pair<VkDescriptorSet, int>> m_descriptorsToDestroy;
	std::deque<std::pair<VkBuffer, int>> m_buffersToDestroy;
	std::deque<std::pair<std::unique_ptr<CCommandBufferWrapper>, int>> m_commandBuffersToDestroy;
};