#pragma once
#include "Vector3.h"
#include "ITextureHelper.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "IShaderManager.h"
#include "../model/TeamColor.h"

enum class RenderMode
{
	TRIANGLES,
	TRIANGLE_STRIP,
	RECTANGLES,
	LINES,
	LINE_LOOP
};

enum class CachedTextureType
{
	RGBA,
	ALPHA,
	DEPTH
};

enum class Feature
{
	INSTANSING,
};

class IDrawingList
{
public:
	virtual void Draw() const = 0;

	virtual ~IDrawingList() {}
};

class IVertexBuffer
{
public:
	virtual void SetIndexBuffer(unsigned int * indexPtr, size_t indexesSize) = 0;
	virtual void Bind() const = 0;
	virtual void DrawIndexes(size_t begin, size_t count) = 0;
	virtual void DrawAll(size_t count) = 0;
	virtual void DrawInstanced(size_t size, size_t instanceCount) = 0;
	virtual void UnBind() const = 0;

	virtual ~IVertexBuffer() {}
};

class IOcclusionQuery
{
public:
	virtual void Query(std::function<void()> const& handler, bool renderToScreen) = 0;
	virtual bool IsVisible() const = 0;

	virtual ~IOcclusionQuery() {}
};

class IRenderer
{
public:
	virtual void RenderArrays(RenderMode mode, std::vector<CVector3f> const& vertices, std::vector<CVector3f> const& normals, std::vector<CVector2f> const& texCoords) = 0;
	virtual void RenderArrays(RenderMode mode, std::vector<CVector2i> const& vertices, std::vector<CVector2f> const& texCoords) = 0;
	virtual void SetColor(const float r, const float g, const float b, const float a = 1.0f) = 0;
	virtual void SetColor(const int r, const int g, const int b, const int a = UCHAR_MAX) = 0;
	virtual void SetColor(const float * color) = 0;
	virtual void SetColor(const int * color) = 0;

	virtual void PushMatrix() = 0;
	virtual void PopMatrix() = 0;
	virtual void Translate(const float dx, const float dy, const float dz) = 0;
	virtual void Translate(const double dx, const double dy, const double dz) = 0;
	virtual void Translate(const int dx, const int dy, const int dz) = 0;
	virtual void Rotate(const double angle, const double x, const double y, const double z) = 0;
	virtual void Scale(const double scale) = 0;
	virtual void GetViewMatrix(float * matrix) const = 0;
	virtual void ResetViewMatrix() = 0;
	virtual void LookAt(CVector3f const& position, CVector3f const& direction, CVector3f const& up) = 0;

	virtual void SetTexture(std::wstring const& texture, bool forceLoadNow = false, int flags = 0) = 0;
	virtual void SetTexture(std::wstring const& texture, TextureSlot slot, int flags = 0) = 0;
	virtual void SetTexture(std::wstring const& texture, const std::vector<sTeamColor> * teamcolor, int flags = 0) = 0;
	virtual std::unique_ptr<ICachedTexture> RenderToTexture(std::function<void()> const& func, unsigned int width, unsigned int height) = 0;
	virtual std::unique_ptr<ICachedTexture> CreateTexture(const void * data, unsigned int width, unsigned int height, CachedTextureType type = CachedTextureType::RGBA) = 0;

	virtual void SetMaterial(const float * ambient, const float * diffuse, const float * specular, const float shininess) = 0;

	virtual std::unique_ptr<IDrawingList> CreateDrawingList(std::function<void()> const& func) = 0;

	virtual std::unique_ptr<IVertexBuffer> CreateVertexBuffer(const float * vertex = nullptr, const float * normals = nullptr, const float * texcoords = nullptr, size_t size = 0, bool temp = false) = 0;

	virtual std::unique_ptr<IOcclusionQuery> CreateOcclusionQuery() = 0;

	virtual std::string GetName() const = 0;

	virtual bool SupportsFeature(Feature feature) const = 0;

	virtual IShaderManager& GetShaderManager() = 0;

	virtual ~IRenderer() {}
};