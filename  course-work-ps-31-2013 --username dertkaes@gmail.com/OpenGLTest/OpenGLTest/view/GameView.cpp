#include "GameView.h"
#include <GL\glut.h>
#include <string>
#include "..\SelectionTools.h"
#include "..\UI\UICheckBox.h"
#include "..\controller\CommandHandler.h"
#include "..\LUA\LUARegisterFunctions.h"
#include "..\LUA\LUARegisterUI.h"
#include "..\LUA\LUARegisterObject.h"
#include "..\model\ObjectGroup.h"
#include "DirectLight.h"

using namespace std;

shared_ptr<CGameView> CGameView::m_instanse = NULL;

weak_ptr<CGameView> CGameView::GetIntanse()
{
	if (!m_instanse.get())
	{
		m_instanse.reset(new CGameView());
		m_instanse->Init();
	}
	weak_ptr<CGameView> pView(m_instanse);

	return pView;
}

void CGameView::CreateTable(float width, float height, std::string const& texture)
{
	m_table.reset(new CTable(width, height, texture));
}

void CGameView::CreateSkybox(double size, std::string const& textureFolder)
{
	m_skybox.reset(new CSkyBox(size, size, size, textureFolder));
}

CGameView::CGameView(void)
{
	m_gameModel = CGameModel::GetIntanse();
	m_ui.reset(new CUIElement());
}

void CGameView::OnTimer(int value)
{
	glutPostRedisplay();
	glutTimerFunc(10, OnTimer, 0);
}

void CGameView::Init()
{
	setlocale(LC_ALL, ""); 
	int argc = 0;
	char* argv[] = {""};
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(600, 600);
	glutCreateWindow("GLUT test");
	glEnable(GL_NORMALIZE);
	glDepthFunc(GL_LESS);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.01f);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LIGHT0);
	
	glutDisplayFunc(CGameView::OnDrawScene);
	glutTimerFunc(10, OnTimer, 0);
	glutReshapeFunc(&OnReshape);
	glutKeyboardFunc(&CInput::OnKeyboard);
	glutSpecialFunc(&CInput::OnSpecialKeyPress);
	glutMouseFunc(&CInput::OnMouse);
	glutMotionFunc(&CInput::OnMouseMove);
	glutPassiveMotionFunc(&CInput::OnPassiveMouseMove);
	glutMotionFunc(&CInput::OnMouseMove);

	m_lua.reset(new CLUAScript());
	RegisterFunctions(*m_lua.get());
	RegisterUI(*m_lua.get());
	RegisterObject(*m_lua.get());
	m_lua->RunScript("main.lua");
	
	glutMainLoop();
}

void CGameView::OnDrawScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	CGameView::GetIntanse().lock()->Update();
	glutSwapBuffers();
}

void CGameView::DrawUI() const
{
	glEnable(GL_BLEND);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0,glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT),0,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	m_ui->Draw();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glDisable(GL_BLEND);
}

void CGameView::DrawBoundingBox()
{
	std::shared_ptr<IObject> object = m_gameModel.lock()->GetSelectedObject();
	if(object)
	{
		if (CGameModel::IsGroup(object.get()))
		{
			CObjectGroup * group = (CObjectGroup *)object.get();
			for(unsigned int i = 0; i < group->GetCount(); ++i)
			{
				object = group->GetChild(i);
				if(object)
				{
					m_modelManager.GetBoundingBox(object->GetPathToModel())->Draw(object->GetX(), 
						object->GetY(), object->GetZ(), object->GetRotation());
				}
			}
		}
		else
		{
			m_modelManager.GetBoundingBox(object->GetPathToModel())->Draw(object->GetX(), 
				object->GetY(), object->GetZ(), object->GetRotation());
		}
	}
}

void CGameView::Update()
{
	if(m_updateCallback) m_updateCallback();
	if(m_singleCallback)
	{
		m_singleCallback();
		m_singleCallback = std::function<void()>();
		Sleep(50);
	}
	m_camera.Update();
	if(m_skybox) m_skybox->Draw(m_camera.GetTranslationX(), m_camera.GetTranslationY(), 0, m_camera.GetScale());
	if(m_table) m_table->Draw();
	glEnable(GL_DEPTH_TEST);
	DrawObjects();
	glDisable(GL_DEPTH_TEST);
	DrawBoundingBox();
	m_ruler.Draw();
	DrawUI();
}

void CGameView::DrawObjects(void)
{
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_LIGHTING);
	CDirectLight light(CVector3f(0, 0, 1000));
	light.SetAmbientIntensity(0.0, 0.0, 0.0, 1.0);
	light.SetDiffuseIntensity(1.0, 1.0, 1.0, 1.0);
	light.SetSpecularIntensity(1.0, 1.0, 1.0, 1.0);
	light.SetLight(GL_LIGHT0);
	unsigned long countObjects = m_gameModel.lock()->GetObjectCount();
	for (unsigned long i = 0; i < countObjects; i++)
	{
		shared_ptr<const IObject> object = m_gameModel.lock()->Get3DObject(i);
		glPushMatrix();
		glTranslated(object->GetX(), object->GetY(), 0);
		glRotated(object->GetRotation(), 0.0, 0.0, 1.0);
		m_modelManager.DrawModel(object->GetPathToModel(), &object->GetHiddenMeshes());
		glPopMatrix();
	}
	glDisable(GL_LIGHTING);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void CGameView::OnReshape(int width, int height) 
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLdouble aspect = (GLdouble)width / (GLdouble)height;
	gluPerspective(60, aspect, 0.5, 100.0);
	glMatrixMode(GL_MODELVIEW);
	CGameView::GetIntanse().lock()->m_ui->Resize(height, width);
}

void CGameView::FreeInstance()
{
	m_instanse.reset();
}

void CGameView::CameraSetLimits(double maxTransX, double maxTransY, double maxScale, double minScale)
{
	m_camera.SetLimits(maxTransX, maxTransY, maxScale, minScale);
}

void CGameView::CameraZoomIn()
{
	m_camera.ZoomIn();
}

void CGameView::CameraZoomOut()
{
	m_camera.ZoomOut();
}

void CGameView::CameraRotate(int rotZ, int rotX)
{
	m_camera.Rotate((double)rotZ / 10, (double)rotX / 5);
}

void CGameView::CameraReset()
{
	m_camera.Reset();
}

void CGameView::CameraTranslateLeft()
{
	m_camera.Translate(CCamera::TRANSLATE, 0.0);
}

void CGameView::CameraTranslateRight()
{
	m_camera.Translate(-CCamera::TRANSLATE, 0.0);
}

void CGameView::CameraTranslateDown()
{
	m_camera.Translate(0.0, CCamera::TRANSLATE);
}

void CGameView::CameraTranslateUp()
{
	m_camera.Translate(0.0, -CCamera::TRANSLATE);
}

void CGameView::SelectObjectGroup(int beginX, int beginY, int endX, int endY)//Works only for Z = 0 plane and select object only if its center is within selection rectangle, needs better algorithm
{
	double beginWorldX, beginWorldY, endWorldX, endWorldY;
	WindowCoordsToWorldCoords(beginX, beginY, beginWorldX, beginWorldY);
	WindowCoordsToWorldCoords(endX, endY, endWorldX, endWorldY);
	double minX = (beginWorldX < endWorldX)?beginWorldX:endWorldX;
	double maxX = (beginWorldX > endWorldX)?beginWorldX:endWorldX;
	double minY = (beginWorldY < endWorldY)?beginWorldY:endWorldY;
	double maxY = (beginWorldY > endWorldY)?beginWorldY:endWorldY;
	CObjectGroup* group = new CObjectGroup();
	CGameModel * model = CGameModel::GetIntanse().lock().get();
	for(unsigned long i = 0; i < model->GetObjectCount(); ++i)
	{
		shared_ptr<IObject> object = model->Get3DObject(i);
		if(object->GetX() > minX && object->GetX() < maxX && object->GetY() > minY && object->GetY() < maxY && object->IsSelectable())
		{
			group->AddChildren(object);
		}
	}
	switch(group->GetCount())
	{
	case 0:
		{
			model->SelectObject(NULL);
			delete group;
		}break;
	case 1:
		{
			model->SelectObject(group->GetChild(0));
			delete group;
		}break;
	default:
		{
			model->SelectObject(std::shared_ptr<IObject>(group));
		}break;
	}
	if(m_selectionCallback) m_selectionCallback();
}

shared_ptr<IObject> CGameView::GetNearestObject(int x, int y)
{
	std::shared_ptr<IObject> selectedObject = NULL;
	double minDistance = 10000000.0;
	CGameModel * model = CGameModel::GetIntanse().lock().get();
	double start[3];
	double end[3];
	WindowCoordsToWorldVector(x, y, start[0], start[1], start[2], end[0], end[1], end[2]);
	for(unsigned long i = 0; i < model->GetObjectCount(); ++i)
	{
		std::shared_ptr<IObject> object = model->Get3DObject(i);
		if(m_modelManager.GetBoundingBox(object->GetPathToModel())->IsIntersectsRay(start, end, object->GetX(), object->GetY(), object->GetZ(), object->GetRotation(), m_selectedObjectCapturePoint))
		{
			double distance = sqrt(object->GetX() * object->GetX() + object->GetY() * object->GetY() + object->GetZ() * object->GetZ());
			if(distance < minDistance)
			{
				selectedObject = object;
				minDistance = distance;
				m_selectedObjectCapturePoint.x -= selectedObject->GetX();
				m_selectedObjectCapturePoint.y -= selectedObject->GetY();
				m_selectedObjectCapturePoint.z -= selectedObject->GetZ();
			}
		}
	}
	return selectedObject;
}

void CGameView::SelectObject(int x, int y, bool shiftPressed)
{
	std::shared_ptr<IObject> selectedObject = GetNearestObject(x, y);
	if(selectedObject && !selectedObject->IsSelectable())
	{
		return;
	}
	std::shared_ptr<IObject> object = m_gameModel.lock()->GetSelectedObject();
	if(CGameModel::IsGroup(object.get()))
	{
		CObjectGroup * group = (CObjectGroup *)object.get();
		if(shiftPressed)
		{
			if(group->ContainsChildren(selectedObject))
			{
				group->RemoveChildren(selectedObject);
				if(group->GetCount() == 1)//Destroy group
				{
					m_gameModel.lock()->SelectObject(group->GetChild(0));
				}
			}
			else
			{
				group->AddChildren(selectedObject);
			}
		}
		else
		{
			if(!group->ContainsChildren(selectedObject))
			{
				m_gameModel.lock()->SelectObject(selectedObject);
			}
			else
			{
				group->SetCurrent(selectedObject);
			}
		}
	}
	else
	{
		if(shiftPressed && object != NULL)
		{
			CObjectGroup * group = new CObjectGroup();
			group->AddChildren(object);
			group->AddChildren(selectedObject);
			m_gameModel.lock()->SelectObject(std::shared_ptr<IObject>(group));
		}
		else
		{
			m_gameModel.lock()->SelectObject(selectedObject);
		}
	}
	if(m_selectionCallback) m_selectionCallback();
}

void CGameView::RulerBegin(double x, double y)
{
	m_ruler.SetBegin(x, y);
}

void CGameView::RulerEnd(double x, double y)
{
	m_ruler.SetEnd(x, y);
}

void CGameView::RulerHide()
{
	m_ruler.Hide();
}

void CGameView::TryMoveSelectedObject(int x, int y)
{
	std::shared_ptr<IObject> object = m_gameModel.lock()->GetSelectedObject();
	if (!object)
	{
		return;
	}
	double worldX, worldY;
	WindowCoordsToWorldCoords(x, y, worldX, worldY, m_selectedObjectCapturePoint.z);
	if (m_table->isCoordsOnTable(worldX, worldY))
	{
		object->SetCoords(worldX - m_selectedObjectCapturePoint.x, worldY - m_selectedObjectCapturePoint.y, 0);
	}
}

bool CGameView::UILeftMouseButtonDown(int x, int y)
{
	return m_ui->LeftMouseButtonDown(x, y);
}

bool CGameView::UILeftMouseButtonUp(int x, int y)
{
	return m_ui->LeftMouseButtonUp(x, y);
}

bool CGameView::UIKeyPress(unsigned char key)
{
	return m_ui->OnKeyPress(key);
}

bool CGameView::UISpecialKeyPress(int key)
{
	return m_ui->OnSpecialKeyPress(key);
}

void CGameView::SetUI(IUIElement * ui)
{
	m_ui.reset(ui);
}

IUIElement * CGameView::GetUI() const
{
	return m_ui.get();
}

void CGameView::SetSelectionCallback(callback(onSelect))
{
	m_selectionCallback = onSelect;
}

void CGameView::SetUpdateCallback(callback(onUpdate))
{
	m_updateCallback = onUpdate;
}

void CGameView::SetSingleCallback(callback(onSingleUpdate))
{
	m_singleCallback = onSingleUpdate;
}