#include <gl/glew.h>
#include "gl.h"
#include "GameWindow.h"
#include "GameView.h"
#include <string>
#include <cstring>
#include "MathUtils.h"
#include "../controller/GameController.h"
#include "../model/ObjectGroup.h"
#include "../LogWriter.h"
#include "../ThreadPool.h"
#include "../Module.h"
#include "../Ruler.h"
#include "../OSSpecific.h"
#include "CameraStrategy.h"
#include "../UI/UIElement.h"

using namespace std;
using namespace placeholders;

shared_ptr<CGameView> CGameView::m_instanse = NULL;
bool CGameView::m_visible = true;

weak_ptr<CGameView> CGameView::GetInstance()
{
	if (!m_instanse.get())
	{
		m_instanse.reset(new CGameView());
		m_instanse->Init();
	}
	weak_ptr<CGameView> pView(m_instanse);

	return pView;
}

void CGameView::FreeInstance()
{
	m_instanse.reset();
}

CGameView::~CGameView()
{
	ThreadPool::CancelAll();
	DisableShadowMap();
}

CGameView::CGameView(void)
	: m_textWriter(m_renderer)
	, m_particles(m_renderer)
	, m_gameModel(make_unique<CGameModel>())
	, m_modelManager(m_renderer, *m_gameModel)
{
	m_ui = make_unique<CUIElement>(m_renderer);
	m_ui->SetTheme(make_shared<CUITheme>(CUITheme::defaultTheme));
}

void CGameView::Init()
{
	setlocale(LC_ALL, ""); 
	setlocale(LC_NUMERIC, "english");

	m_window = make_unique<CGameWindow>();
	
	m_vertexLightning = false;
	m_shadowMap = false;
	memset(m_lightPosition, 0, sizeof(float)* 3);
	m_gpuSkinning = false;
	m_camera = make_unique<CCameraStrategy>(0.0, 0.0, 2.8, 0.5);
	m_tableList = 0;
	m_tableListShadow = 0;

	m_gameController = make_unique<CGameController>(*m_gameModel);
	m_gameController->Init();
	m_soundPlayer.Init();

	InitInput();

	m_window->DoOnDrawScene([this] {
		DrawShadowMap();
		m_window->Clear();
		Update();
	});
	m_window->DoOnResize([this](int width, int height) {m_ui->Resize(height, width);});
	m_window->DoOnShutdown(FreeInstance);

	m_window->Init();
}

void WindowCoordsToWorldVector(int x, int y, double & startx, double & starty, double & startz, double & endx, double & endy, double & endz)
{
	//Get model, projection and viewport matrices
	double matModelView[16], matProjection[16];
	int viewport[4];
	glGetDoublev(GL_MODELVIEW_MATRIX, matModelView);
	glGetDoublev(GL_PROJECTION_MATRIX, matProjection);
	glGetIntegerv(GL_VIEWPORT, viewport);
	//Set OpenGL Windows coordinates
	double winX = (double)x;
	double winY = viewport[3] - (double)y;

	//Cast a ray from eye to mouse cursor;
	gluUnProject(winX, winY, 0.0, matModelView, matProjection,
		viewport, &startx, &starty, &startz);
	gluUnProject(winX, winY, 1.0, matModelView, matProjection,
		viewport, &endx, &endy, &endz);
}

void WindowCoordsToWorldCoords(int windowX, int windowY, double & worldX, double & worldY, double worldZ = 0)
{
	double startx, starty, startz, endx, endy, endz;
	WindowCoordsToWorldVector(windowX, windowY, startx, starty, startz, endx, endy, endz);
	double a = (worldZ - startz) / (endz - startz);
	worldX = a * (endx - startx) + startx;
	worldY = a * (endy - starty) + starty;
}

static const string g_controllerTag = "controller";

void CGameView::InitInput()
{
	m_window->ResetInput();
	auto& input = m_window->GetInput();
	m_camera->SetInput(input);
	//UI
	input.DoOnLMBDown([this](int x, int y) {
		return m_ui->LeftMouseButtonDown(x, y);
	}, 0);
	input.DoOnLMBUp([this](int x, int y) {
		return m_ui->LeftMouseButtonUp(x, y);
	}, 0);
	input.DoOnCharacter([this](unsigned int key) {
		return m_ui->OnCharacterInput(key);
	}, 0);
	input.DoOnKeyDown([this](int key, int modifiers) {
		return m_ui->OnKeyPress(key, modifiers);
	}, 0);
	input.DoOnMouseMove([this](int x, int y) {
		m_ui->OnMouseMove(x, y);
		return false;
	}, 9);
	//Ruler
	input.DoOnLMBDown([this](int x, int y) {
		double wx, wy;
		WindowCoordsToWorldCoords(x, y, wx, wy);
		if (m_ruler.IsVisible())
		{
			m_ruler.Hide();
		}
		else
		{
			if (m_ruler.IsEnabled())
			{
				m_ruler.SetBegin(wx, wy);
				return true;
			}
		}
		return false;
	}, 2);
	input.DoOnLMBUp([this](int x, int y) {
		double wx, wy;
		WindowCoordsToWorldCoords(x, y, wx, wy);
		m_ruler.SetEnd(wx, wy);
		return false;
	}, 2);
	input.DoOnRMBDown([this](int, int) {
		if (m_ruler.IsVisible())
		{
			m_ruler.Hide();
		}
		return false;
	}, 2);
	input.DoOnMouseMove([this](int x, int y) {
		double wx, wy;
		WindowCoordsToWorldCoords(x, y, wx, wy);
		if (m_ruler.IsEnabled())
		{
			m_ruler.SetEnd(wx, wy);
		}
		return false;
	}, 2);
	//Game Controller
	input.DoOnLMBDown([&](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin.x, begin.y, begin.z, end.x, end.y, end.z);
		bool result = m_gameController->OnLeftMouseDown(begin, end, input.GetModifiers());
		auto object = m_gameModel->GetSelectedObject();
		if (result && object)
		{
			m_ruler.SetBegin(object->GetX(), object->GetY());
		}
		return result;
	}, 5, g_controllerTag);
	input.DoOnLMBUp([&](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin.x, begin.y, begin.z, end.x, end.y, end.z);
		bool result = m_gameController->OnLeftMouseUp(begin, end, input.GetModifiers());
		if (result && !m_ruler.IsEnabled())
		{
			m_ruler.Hide();
		}
		m_ruler.Disable();
		return result;
	}, 5, g_controllerTag);
	input.DoOnMouseMove([&](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin.x, begin.y, begin.z, end.x, end.y, end.z);
		bool result = m_gameController->OnMouseMove(begin, end, input.GetModifiers());
		auto object = m_gameModel->GetSelectedObject();
		if (result && object)
		{
			m_ruler.SetEnd(object->GetX(), object->GetY());
		}
		return result;
	}, 5, g_controllerTag);
	input.DoOnRMBDown([&](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin.x, begin.y, begin.z, end.x, end.y, end.z);
		return m_gameController->OnRightMouseDown(begin, end, input.GetModifiers());
	}, 5, g_controllerTag);
	input.DoOnRMBUp([&](int x, int y) {
		CVector3d begin, end;
		WindowCoordsToWorldVector(x, y, begin.x, begin.y, begin.z, end.x, end.y, end.z);
		return m_gameController->OnRightMouseUp(begin, end, input.GetModifiers());
	}, 5, g_controllerTag);
}

void CGameView::DrawUI()
{
	m_window->Enter2DMode();
	m_ui->Draw();
	m_window->Leave2DMode();
}

void DrawBBox(IBounding* ibox, double x, double y, double z, double rotation, IRenderer & renderer)
{
	if (dynamic_cast<CBoundingCompound*>(ibox) != NULL)
	{
		CBoundingCompound * bbox = (CBoundingCompound *)ibox;
		for (size_t i = 0; i < bbox->GetChildCount(); ++i)
		{
			DrawBBox(bbox->GetChild(i), x, y, z, rotation, renderer);
		}
		return;
	}
	CBoundingBox * bbox = (CBoundingBox *)ibox;
	if (!bbox) return;
	renderer.PushMatrix();
	renderer.Translate(x, y, z);
	renderer.Rotate(rotation, 0.0, 0.0, 1.0);
	renderer.Scale(bbox->GetScale());
	renderer.SetColor(0.0f, 0.0f, 255.0f);
	const double * min = bbox->GetMin();
	const double * max = bbox->GetMax();
	renderer.RenderArrays(RenderMode::LINE_LOOP, { CVector3d(min[0], min[1], min[2]), { min[0], max[1], min[2] }, { min[0], max[1], max[2] }, { min[0], min[1], max[2] } }, {}, {});//Left
	renderer.RenderArrays(RenderMode::LINE_LOOP, { CVector3d(min[0], min[1], min[2]), { min[0], min[1], max[2] }, { max[0], min[1], max[2] }, { max[0], min[1], min[2] } }, {}, {});//Back
	renderer.RenderArrays(RenderMode::LINE_LOOP, { CVector3d(max[0], min[1], min[2]), { max[0], max[1], min[2] }, { max[0], max[1], max[2] }, { max[0], min[1], max[2] } }, {}, {});//Right
	renderer.RenderArrays(RenderMode::LINE_LOOP, { CVector3d(min[0], max[1], min[2]), { min[0], max[1], max[2] }, { max[0], max[1], max[2] }, { max[0], max[1], min[2] } }, {}, {}); //Front
	renderer.SetColor(255.0f, 255.0f, 255.0f);
	renderer.PopMatrix();
}

void CGameView::DrawBoundingBox()
{
	shared_ptr<IObject> object = m_gameModel->GetSelectedObject();
	if(object)
	{
		if (CGameModel::IsGroup(object.get()))
		{
			CObjectGroup * group = (CObjectGroup *)object.get();
			for(size_t i = 0; i < group->GetCount(); ++i)
			{
				object = group->GetChild(i);
				if(object)
				{
					auto bbox = m_gameModel->GetBoundingBox(object->GetPathToModel());
					if (bbox)
					{
						DrawBBox(bbox.get(), object->GetX(), object->GetY(), object->GetZ(), object->GetRotation(), m_renderer);
					}
				}
			}
		}
		else
		{
			auto bbox = m_gameModel->GetBoundingBox(object->GetPathToModel());
			if(bbox) DrawBBox(bbox.get(), object->GetX(), object->GetY(), object->GetZ(), object->GetRotation(), m_renderer);
		}
	}
}

void CGameView::Update()
{
	ThreadPool::Update();
	const double * position = m_camera->GetPosition();
	const double * direction = m_camera->GetDirection();
	const double * up = m_camera->GetUpVector();
	m_soundPlayer.SetListenerPosition(CVector3d(position), CVector3d(direction));
	m_soundPlayer.Update();
	if (m_skybox) m_skybox->Draw(-direction[0], -direction[1], -direction[2], m_camera->GetScale());
	m_renderer.ResetViewMatrix();
	gluLookAt(position[0], position[1], position[2], direction[0], direction[1], direction[2], up[0], up[1], up[2]);
	m_gameController->Update();
	DrawObjects();
	DrawBoundingBox();
	DrawRuler();
	DrawUI();
}

void CGameView::DrawRuler()
{
	if (m_ruler.IsVisible())
	{
		m_renderer.SetColor(255.0f, 255.0f, 0.0f);
		m_renderer.RenderArrays(RenderMode::LINES, { m_ruler.GetBegin(),m_ruler.GetEnd() }, {}, {});
		m_renderer.SetColor(255.0f, 255.0f, 255.0f);
		char str[10];
		sprintf(str, "%0.2f", m_ruler.GetDistance());
		DrawText3D(m_ruler.GetEnd(), str);
	}
}

void CGameView::ResetTable()
{
	m_tableList.reset();
	m_tableListShadow.reset();
}

void CGameView::DrawTable(bool shadowOnly)
{	
	auto list = m_renderer.CreateDrawingList([this, shadowOnly] {
		CLandscape const& landscape = m_gameModel->GetLandscape();
		double x1 = -landscape.GetWidth() / 2.0;
		double x2 = landscape.GetWidth() / 2.0;
		double y1 = -landscape.GetDepth() / 2.0;
		double y2 = landscape.GetDepth() / 2.0;
		double xstep = landscape.GetWidth() / (landscape.GetPointsPerWidth() - 1);
		double ystep = landscape.GetDepth() / (landscape.GetPointsPerDepth() - 1);
		m_renderer.SetTexture(landscape.GetTexture());
		unsigned int k = 0;
		for (double x = x1; x <= x2 - xstep; x += xstep)
		{
			vector<CVector3d> vertex;
			vector<CVector2d> texCoord;
			for (double y = y1; y <= y2; y += ystep, k++)
			{
				texCoord.push_back({ (x + x2) / landscape.GetHorizontalTextureScale(), (y + y2) / landscape.GetVerticalTextureScale() });
				vertex.push_back({ x, y, landscape.GetHeight(k) });
				texCoord.push_back({ (x + x2 + xstep) / landscape.GetHorizontalTextureScale(), (y + y2) / landscape.GetVerticalTextureScale() });
				vertex.push_back({ x + xstep, y, landscape.GetHeight(k + 1) });
			}
			m_renderer.RenderArrays(RenderMode::TRIANGLE_STRIP, vertex, {}, texCoord);
		}
		m_renderer.SetTexture("");
		for (size_t i = 0; i < landscape.GetStaticObjectCount(); i++)
		{
			CStaticObject const& object = landscape.GetStaticObject(i);
			if (!shadowOnly || object.CastsShadow())
			{
				m_renderer.PushMatrix();
				m_renderer.Translate(object.GetX(), object.GetY(), 0.0);
				m_renderer.Rotate(object.GetRotation(), 0.0, 0.0, 1.0);
				m_modelManager.DrawModel(object.GetPathToModel(), nullptr, shadowOnly, m_gpuSkinning);
				m_renderer.PopMatrix();
			}
		}
		if (!shadowOnly)//Down't draw decals because they don't cast shadows
		{
			for (size_t i = 0; i < landscape.GetNumberOfDecals(); ++i)
			{
				sDecal const& decal = landscape.GetDecal(i);
				m_renderer.SetTexture(decal.texture);
				m_renderer.PushMatrix();
				m_renderer.Translate(decal.x, decal.y, 0.0);
				m_renderer.Rotate(decal.rotation, 0.0, 0.0, 1.0);
				m_renderer.RenderArrays(RenderMode::TRIANGLE_STRIP, {
					CVector3d(-decal.width / 2, -decal.depth / 2, landscape.GetHeight(decal.x - decal.width / 2, decal.y - decal.depth / 2) + 0.0001),
					{ -decal.width / 2, decal.depth / 2, landscape.GetHeight(decal.x - decal.width / 2, decal.y + decal.depth / 2) + 0.0001 },
					{ decal.width / 2, -decal.depth / 2, landscape.GetHeight(decal.x + decal.width / 2, decal.y - decal.depth / 2) + 0.0001 },
					{ decal.width / 2, decal.depth / 2, landscape.GetHeight(decal.x + decal.width / 2, decal.y + decal.depth / 2) + 0.0001 }
					}, {},{ CVector2d(0.0, 0.0), { 0.0, 1.0 }, { 1.0, 0.0 }, { 1.0, 1.0 } });
				m_renderer.PopMatrix();
			}
		}
		m_renderer.SetTexture("");
	});
	if (shadowOnly)
	{
		m_tableListShadow = move(list);
	}
	else
	{
		m_tableList = move(list);
	}
}

void CGameView::DrawObjects(void)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	m_shader.BindProgram();
	if (m_vertexLightning)
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glEnable(GL_LIGHTING);
	}
	if (m_shadowMap) SetUpShadowMapDraw();
	if (!m_tableList) DrawTable(false);
	m_tableList->Draw();
	size_t countObjects = m_gameModel->GetObjectCount();
	for (size_t i = 0; i < countObjects; i++)
	{
		shared_ptr<IObject> object = m_gameModel->Get3DObject(i);
		m_renderer.PushMatrix();
		m_renderer.Translate(object->GetX(), object->GetY(), 0.0);
		m_renderer.Rotate(object->GetRotation(), 0.0, 0.0, 1.0);
		m_modelManager.DrawModel(object->GetPathToModel(), object, false, m_gpuSkinning);
		size_t secondaryModels = object->GetSecondaryModelsCount();
		for (size_t j = 0; j < secondaryModels; ++j)
		{
			m_modelManager.DrawModel(object->GetSecondaryModel(j), object, false, m_gpuSkinning);
		}
		m_renderer.PopMatrix();
	}
	m_shader.UnBindProgram();
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	for (size_t i = 0; i < m_gameModel->GetProjectileCount(); i++)
	{
		CProjectile const& projectile = m_gameModel->GetProjectile(i);
		m_renderer.PushMatrix();
		m_renderer.Translate(projectile.GetX(), projectile.GetY(), projectile.GetZ());
		m_renderer.Rotate(projectile.GetRotation(), 0.0, 0.0, 1.0);
		if (!projectile.GetPathToModel().empty())
			m_modelManager.DrawModel(projectile.GetPathToModel(), nullptr, false, m_gpuSkinning);
		if (!projectile.GetParticle().empty())
			m_particles.DrawEffect(projectile.GetParticle(), projectile.GetTime());
		m_renderer.PopMatrix();
	}
	m_particles.DrawParticles();
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glDisable(GL_DEPTH_TEST);
}

void CGameView::SetUpShadowMapDraw()
{
	float cameraModelViewMatrix[16];
	float cameraInverseModelViewMatrix[16];
	float lightMatrix[16];
	m_renderer.GetViewMatrix(cameraModelViewMatrix);
	InvertMatrix(cameraModelViewMatrix, cameraInverseModelViewMatrix);

	m_renderer.PushMatrix();
	m_renderer.ResetViewMatrix();
	m_renderer.Translate(0.5, 0.5, 0.5); // + 0.5
	m_renderer.Scale(0.5); // * 0.5
	glMultMatrixf(m_lightProjectionMatrix);
	glMultMatrixf(m_lightModelViewMatrix);
	glMultMatrixf(cameraInverseModelViewMatrix);
	m_renderer.GetViewMatrix(lightMatrix);
	m_renderer.PopMatrix();

	m_shader.SetUniformMatrix4("lightMatrix", 1, lightMatrix);
}

void CGameView::DrawShadowMap()
{
	if (!m_shadowMap) return;
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	m_renderer.ResetViewMatrix();
	gluPerspective(m_shadowAngle, 1.0, 3.0, 300.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	m_renderer.ResetViewMatrix();
	gluLookAt(m_lightPosition[0], m_lightPosition[1], m_lightPosition[2], 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0, 500.0);

	glGetFloatv(GL_PROJECTION_MATRIX, m_lightProjectionMatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, m_lightModelViewMatrix);

	if (!m_tableListShadow) DrawTable(true);
	m_tableListShadow->Draw();

	size_t countObjects = m_gameModel->GetObjectCount();
	for (size_t i = 0; i < countObjects; i++)
	{
		shared_ptr<IObject> object = m_gameModel->Get3DObject(i);
		if (!object->CastsShadow()) continue;
		glPushMatrix();
		glTranslated(object->GetX(), object->GetY(), 0);
		glRotated(object->GetRotation(), 0.0, 0.0, 1.0);
		m_modelManager.DrawModel(object->GetPathToModel(), object, true, m_gpuSkinning);
		size_t secondaryModels = object->GetSecondaryModelsCount();
		for (size_t j = 0; j < secondaryModels; ++j)
		{
			m_modelManager.DrawModel(object->GetSecondaryModel(j), object, true, m_gpuSkinning);
		}
		glPopMatrix();
	}

	glPolygonOffset(0.0f, 0.0f);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glPopAttrib();
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
}

void CGameView::CreateSkybox(double size, string const& textureFolder)
{
	m_skybox.reset(new CSkyBox(size, size, size, textureFolder, m_renderer));
}

CGameController& CGameView::GetController()
{
	return *m_gameController;
}

CGameModel& CGameView::GetModel()
{
	return *m_gameModel;
}

void CGameView::ResetController()
{
	m_window->GetInput().DeleteAllSignalsByTag(g_controllerTag);
	m_gameController.reset();
	m_gameModel = make_unique<CGameModel>();
	m_gameController = make_unique<CGameController>(*m_gameModel);
}

ICamera * CGameView::GetCamera()
{
	return m_camera.get();
}

void CGameView::SetCamera(ICamera * camera)
{
	m_camera.reset(camera);
	m_camera->SetInput(m_window->GetInput());
}

CModelManager& CGameView::GetModelManager()
{
	return m_modelManager;
}

IUIElement * CGameView::GetUI() const
{
	return m_ui.get();
}

CParticleSystem& CGameView::GetParticleSystem()
{
	return m_particles;
}

CTextWriter& CGameView::GetTextWriter()
{
	return m_textWriter;
}

ISoundPlayer& CGameView::GetSoundPlayer()
{
	return m_soundPlayer;
}

CTranslationManager& CGameView::GetTranslationManager()
{
	return m_translationManager;
}

CRuler& CGameView::GetRuler()
{
	return m_ruler;
}

IRenderer& CGameView::GetRenderer()
{
	return m_renderer;
}

void CGameView::ResizeWindow(int height, int width)
{
	m_window->ResizeWindow(width, height);
}

void CGameView::NewShaderProgram(string const& vertex, string const& fragment, string const& geometry)
{
	m_shader.NewProgram(vertex, fragment, geometry);
}

void CGameView::EnableVertexLightning(bool enable)
{ 
	m_vertexLightning = enable;
	if (enable)
		glEnable(GL_NORMALIZE);
	else
		glDisable(GL_NORMALIZE);
}

void CGameView::EnableShadowMap(int size, float angle)
{
	if (m_shadowMap) return;
	if (!GLEW_ARB_depth_buffer_float)
	{
		LogWriter::WriteLine("GL_ARB_depth_buffer_float is not supported, shadow maps cannot be enabled");
		return;
	}
	if (!GLEW_EXT_framebuffer_object)
	{
		LogWriter::WriteLine("GL_EXT_framebuffer_object is not supported, shadow maps cannot be enabled");
		return;
	}
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &m_shadowMapTexture);
	glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size, size,
		0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE, 0);
	glGenFramebuffers(1, &m_shadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
	glDrawBuffer(GL_NONE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMapTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogWriter::WriteLine("Cannot enable shadowmaps. Error creating framebuffer.");
		glDeleteTextures(1, &m_shadowMapTexture);
		return;
	}
	m_shadowMap = true;
	m_shadowMapSize = size;
	m_shadowAngle = angle;
	
}

void CGameView::DisableShadowMap()
{
	if (!m_shadowMap) return;
	glDeleteTextures(1, &m_shadowMapTexture);
	glDeleteFramebuffersEXT(1, &m_shadowMapFBO);
	m_shadowMap = false;
}

void CGameView::SetLightPosition(int index, float* pos)
{
	glLightfv(GL_LIGHT0 + index, GL_POSITION, pos);
	if(index == 0) memcpy(m_lightPosition, pos, sizeof(float)* 3);
}

void CGameView::EnableMSAA(bool enable)
{
	if (GLEW_ARB_multisample)
	{
		if (enable)
			glEnable(GL_MULTISAMPLE_ARB);
		else
			glDisable(GL_MULTISAMPLE_ARB);
	}
	else
	{
		LogWriter::WriteLine("MSAA is not supported");
	}
}

float CGameView::GetMaxAnisotropy()
{
	float aniso = 1.0f;
	if (GLEW_EXT_texture_filter_anisotropic)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
	return aniso;
}

void CGameView::ClearResources()
{
	m_modelManager = CModelManager(m_renderer, *m_gameModel);
	m_renderer.GetTextureManager().Reset();
	if (m_skybox)
	{
		m_skybox->ResetList();
	}
	ResetTable();
}

void CGameView::SetWindowTitle(string const& title)
{
	m_window->SetTitle(title + " - Wargame Engine");
}

CShaderManager const* CGameView::GetShaderManager() const
{
	return &m_shader;
}

void CGameView::Preload(string const& image)
{
	if (!image.empty())
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_window->Enter2DMode();
		m_renderer.SetTexture(image);
		float width = 640.0f;//glutGet(GLUT_WINDOW_WIDTH);
		float height = 480.0f;//glutGet(GLUT_WINDOW_HEIGHT);
		m_renderer.RenderArrays(RenderMode::TRIANGLE_STRIP, { CVector2f(0.0f, 0.0f), { 0.0f, height }, { width, 0.0f }, { width, height } }, { CVector2f(0.0f, 0.0f), { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } });
		//glutSwapBuffers();
		m_window->Leave2DMode();
	}
	size_t countObjects = m_gameModel->GetObjectCount();
	for (size_t i = 0; i < countObjects; i++)
	{
		shared_ptr<const IObject> object = m_gameModel->Get3DObject(i);
		m_modelManager.LoadIfNotExist(object->GetPathToModel());
	}
	m_renderer.SetTexture("");
}

void CGameView::LoadModule(string const& module)
{
	ThreadPool::CancelAll();
	sModule::Load(module);
	ChangeWorkingDirectory(sModule::folder);
	m_vertexLightning = false;
	m_shadowMap = false;
	memset(m_lightPosition, 0, sizeof(float) * 3);
	ThreadPool::QueueCallback([this]() {
		ResetController();
		ClearResources();
		m_ui->ClearChildren();
		GetController().Init();
		InitInput();
	});
}

void CGameView::ToggleFullscreen() const 
{
	m_window->ToggleFullscreen();
}

void CGameView::DrawText3D(CVector3d const& pos, string const& text)
{
	glRasterPos3d(pos.x, pos.y, pos.z); // location to start printing text
	for (size_t i = 0; i < text.size(); i++) // loop until i is greater then l
	{
		//glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, text[i]); // Print a character on the screen
	}
}

void CGameView::EnableLight(size_t index, bool enable)
{
	if (enable)
	{
		glEnable(GL_LIGHT0 + index);
	}
	else
	{
		glDisable(GL_LIGHT0 + index);
	}
}

static const map<LightningType, GLenum> lightningTypesMap = {
	{ LightningType::DIFFUSE, GL_DIFFUSE },
	{ LightningType::AMBIENT, GL_AMBIENT },
	{ LightningType::SPECULAR, GL_SPECULAR }
};

void CGameView::SetLightColor(size_t index, LightningType type, float * values)
{
	glLightfv(GL_LIGHT0 + index, lightningTypesMap.at(type), values);
}

void CGameView::EnableGPUSkinning(bool enable)
{
	m_gpuSkinning = enable;
}