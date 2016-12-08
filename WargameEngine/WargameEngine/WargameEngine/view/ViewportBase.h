#pragma once
#include "IViewport.h"
#include <map>

class IViewHelper;
class IOcclusionQuery;
class IObject;

class CViewportBase : public IViewport
{
public:
	CViewportBase(int x, int y, int width, int height, float fieldOfView, IViewHelper & renderer);
	~CViewportBase();
	virtual ICamera& GetCamera() override;
	virtual ICamera const& GetCamera() const override;
	virtual void SetCamera(std::unique_ptr<ICamera> && camera) override;

	virtual ICachedTexture const& GetTexture() const override;

	virtual IOcclusionQuery & GetOcclusionQuery(const IBaseObject* object) override;

	virtual Matrix4F GetProjectionMatrix() const override;
	virtual Matrix4F GetViewMatrix() const override;

	virtual void SetPolygonOffset(bool enable, float factor = 0.0f, float units = 0.0f) override;
	virtual void SetClippingPlanes(double near = 1.0, double far = 1000.0) override;

	virtual bool PointIsInViewport(int x, int y) const override;

	virtual void Resize(int width, int height) override;

	virtual int GetX() const override { return m_x; }
	virtual int GetY() const override { return m_y; }
	virtual int GetWidth() const override { return m_width; }
	virtual int GetHeight() const override { return m_height; }

	virtual void SetShadowViewport(IViewport* viewport) override { m_shadowMapViewport = viewport; }
	virtual void SetUpShadowMap() const override;
	virtual IViewport* GetShadowViewport() const override { return m_shadowMapViewport; }
protected:

	std::unique_ptr<ICamera> m_camera;
	int m_x;
	int m_y;
	int m_width;
	int m_height;
	float m_fieldOfView;
	IViewHelper & m_renderer;
	Matrix4F m_projectionMatrix;
	Matrix4F m_viewMatrix;
	float m_polygonOffsetFactor = 0.0f;
	float m_polygonOffsetUnits = 0.0f;
	double m_nearPane = 1.0;
	double m_farPane = 1000.0;
	IViewport* m_shadowMapViewport = nullptr;
	std::map<const IBaseObject*, std::unique_ptr<IOcclusionQuery>> m_occlusionQueries;
};