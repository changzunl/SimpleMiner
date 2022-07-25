#include "Scene.hpp"

#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Audio/AudioSystem.hpp"

class XboxController;

constexpr int START_MAX_VERTS = 3;

class SceneAttract : public Scene
{
public:
	SceneAttract(Game* game);
	virtual ~SceneAttract();
	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual void Update() override;
	virtual void UpdateCamera() override;
	virtual void RenderWorld() const override;
	virtual void RenderUI() const override;


private:
	void RenderUILogoText() const;
	void RenderUIMenuButton() const;
	void HandleInput();
	void HandleMenuCursor();
	void HandleMenuSelection();
	void SelectCurrentMenu();

private:
	Vertex_PCU  m_startVertices[START_MAX_VERTS]    = {};
	int         m_menuSelectionIdx                  = 0;
	bool        m_isQuitting                        = false;
	float       m_quitTimeCounter                   = 0.0f;
	XboxController* m_controller = nullptr;
};

