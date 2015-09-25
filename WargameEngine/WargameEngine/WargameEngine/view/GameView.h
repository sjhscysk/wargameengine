#include <memory>

#include "ICamera.h"
#include "IRenderer.h"
#include "../ISoundPlayer.h"
#include "IInput.h"
#include "../UI/IUI.h"
#include "../controller/GameController.h"
#include "../model/GameModel.h"
#include "IGameWindow.h"
#include "ModelManager.h"
#include "ParticleSystem.h"
#include "TextWriter.h"
#include "SkyBox.h"
#include "../Ruler.h"
#include "../TranslationManager.h"

enum class LightningType
{
	DIFFUSE,
	AMBIENT,
	SPECULAR
};

class CGameView
{
public:
	static std::weak_ptr<CGameView> GetInstance();
	static void FreeInstance();
	~CGameView();

	void ResetTable();
	void CreateSkybox(double size, std::string const& textureFolder);
	CGameController& GetController();
	CGameModel& GetModel();
	IUIElement * GetUI() const;
	ICamera * GetCamera();
	void SetCamera(ICamera * camera);
	CModelManager& GetModelManager();
	CParticleSystem& GetParticleSystem();
	CTextWriter& GetTextWriter();
	ISoundPlayer& GetSoundPlayer();
	CTranslationManager& GetTranslationManager();
	CRuler& GetRuler();
	IRenderer& GetRenderer();
	const IShaderManager & GetShaderManager() const;
	void ResizeWindow(int height, int width);
	void NewShaderProgram(std::string const& vertex = "", std::string const& fragment = "", std::string const& geometry = "");
	void EnableVertexLightning(bool enable);
	void EnableGPUSkinning(bool enable);
	void EnableShadowMap(int size, float angle);
	void DisableShadowMap();
	void SetLightPosition(int index, float* pos);
	void EnableMSAA(bool enable);
	static float GetMaxAnisotropy();
	void ClearResources();
	void SetWindowTitle(std::string const& title);
	void Preload(std::string const& image);
	void LoadModule(std::string const& module);
	void ToggleFullscreen() const;

	void EnableLight(size_t index, bool enable = true);
	void SetLightColor(size_t index, LightningType type, float * values);
	
private:
	void DrawTable(bool shadowOnly = false);
	void DrawUI();
	void Update();
	void DrawRuler();
	void DrawObjects(void);
	void DrawBoundingBox();
	void DrawShadowMap();
	void SetUpShadowMapDraw();
	void Init();
	void InitInput();
	void ResetController();
	void DrawText3D(CVector3d const& pos, std::string const& text);
	CGameView(void);
	CGameView(CGameView const&) = delete;
	CGameView& operator=(const CGameView&) = delete;

	static std::shared_ptr<CGameView> m_instanse;
	std::unique_ptr<IGameWindow> m_window;
	std::unique_ptr<CGameModel> m_gameModel;
	std::unique_ptr<CGameController> m_gameController;
	std::unique_ptr<IShaderManager> m_shaderManager;
	std::unique_ptr<ISoundPlayer> m_soundPlayer;
	std::unique_ptr<IRenderer> m_renderer;
	std::unique_ptr<ICamera> m_camera;
	std::unique_ptr<CSkyBox> m_skybox;
	std::unique_ptr<IUIElement> m_ui;
	CModelManager m_modelManager;
	CParticleSystem m_particles;
	CTextWriter m_textWriter;
	CRuler m_ruler;
	CTranslationManager m_translationManager;

	bool m_vertexLightning;
	bool m_shadowMap;
	unsigned int m_shadowMapTexture;
	unsigned int m_shadowMapFBO;
	int m_shadowMapSize;
	float m_lightProjectionMatrix[16];
	float m_lightModelViewMatrix[16];
	float m_lightPosition[3];
	float m_shadowAngle;
	static bool m_visible;
	bool m_gpuSkinning;

	std::unique_ptr<IDrawingList> m_tableList;
	std::unique_ptr<IDrawingList> m_tableListShadow;
};