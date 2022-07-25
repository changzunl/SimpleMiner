#include "Scene.hpp"

#include "GameCommon.hpp"
#include <string>

class World;

class ScenePlaying : public Scene
{
public:
	ScenePlaying(Game* game);
	virtual ~ScenePlaying();

	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual void Update() override;
	virtual void UpdateCamera() override;
	virtual void RenderWorld() const override;
	virtual void RenderUI() const override;

private:
	void InitializeEntities();
	void InitializeMap();
	void InitializeDebugRender();

	void RenderHUD() const;
	void HandleInput();
	void HandleCheatInput();
	void HandleLightInput();

private:
	bool    m_isSlowMo               = false;
	World*  m_map                    = nullptr;

};

