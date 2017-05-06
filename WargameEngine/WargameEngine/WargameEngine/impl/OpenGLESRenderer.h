#pragma once
#include "../view/IViewHelper.h"
#include "MatrixManagerGLM.h"
#include "ShaderManagerOpenGLES.h"
#include <vector>

class CShaderManagerOpenGLES;

class COpenGLESRenderer : public IViewHelper
{
public:
	COpenGLESRenderer();
	~COpenGLESRenderer();

	//IRenderer
	void RenderArrays(RenderMode mode, array_view<CVector3f> const& vertices, array_view<CVector3f> const& normals, array_view<CVector2f> const& texCoords) override;
	void RenderArrays(RenderMode mode, array_view<CVector2i> const& vertices, array_view<CVector2f> const& texCoords) override;
	void DrawIndexes(IVertexBuffer& buffer, size_t begin, size_t count) override;
	void DrawAll(IVertexBuffer& buffer, size_t count) override;
	void DrawInstanced(IVertexBuffer& buffer, size_t size, size_t instanceCount) override;
	void SetIndexBuffer(IVertexBuffer& buffer, const unsigned int* indexPtr, size_t indexesSize) override;

	void PushMatrix() override;
	void PopMatrix() override;
	void Translate(const CVector3f& delta) override;
	void Translate(int dx, int dy, int dz = 0) override;
	void Rotate(float angle, const CVector3f& axis) override;
	void Rotate(const CVector3f& rotations) override;
	void Scale(float scale) override;
	const float* GetViewMatrix() const override;
	void LookAt(CVector3f const& position, CVector3f const& direction, CVector3f const& up) override;

	void SetTexture(const Path& texture, bool forceLoadNow = false, int flags = 0) override;
	void SetTexture(const Path& texture, TextureSlot slot, int flags = 0) override;
	void SetTexture(const Path& texture, const std::vector<sTeamColor>* teamcolor, int flags = 0) override;
	void SetTexture(const ICachedTexture& texture, TextureSlot slot = TextureSlot::eDiffuse) override;
	void UnbindTexture(TextureSlot slot = TextureSlot::eDiffuse) override;
	void RenderToTexture(std::function<void()> const& func, ICachedTexture& texture, unsigned int width, unsigned int height) override;
	std::unique_ptr<ICachedTexture> CreateTexture(const void* data, unsigned int width, unsigned int height, CachedTextureType type = CachedTextureType::RGBA) override;
	ICachedTexture* GetTexturePtr(const Path& texture) const override;

	void SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0xff) override;
	void SetColor(const float* color) override;
	void SetMaterial(const float* ambient, const float* diffuse, const float* specular, float shininess) override;

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
	const float* GetProjectionMatrix() const override;
	void EnableDepthTest(bool enable) override;
	void EnableBlending(bool enable) override;
	void SetUpViewport(unsigned int viewportX, unsigned int viewportY, unsigned int viewportWidth, unsigned int viewportHeight, float viewingAngle, float nearPane = 1.0f, float farPane = 1000.0f) override;
	void EnablePolygonOffset(bool enable, float factor = 0.0f, float units = 0.0f) override;
	void ClearBuffers(bool color = true, bool depth = true) override;
	void DrawIn2D(std::function<void()> const& drawHandler) override;

	void SetVersion(int version);
	void Init(int width, int height);
	void SetVrViewport(float x, float y, float width, float height, float fov);
	void SetVrViewMatrices(std::vector<const float*> const& matrices);
	void DisableClear(bool disable);
	void BindVAO(unsigned vao, unsigned indexBuffer);

private:
	CTextureManager* m_textureManager;
	CShaderManagerOpenGLES m_shaderManager;
	int m_viewport[4];
	int m_version;
	CMatrixManagerGLM m_matrixManager;
	float m_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	;
	std::unique_ptr<IShaderProgram> m_defaultProgram;
	unsigned int m_vao;
	unsigned int m_activeVao = 0;
	unsigned int m_indexBuffer = 0;
	float m_vrViewport[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	float m_vrFovOverride = 0.0f;
	std::vector<unsigned> m_currentTextures;
	bool m_disableClear = false;
};