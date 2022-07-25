#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Camera.hpp"

class App;
class Game;

extern App* g_theApp;

class App
{
public:
    App();
    ~App();

    // lifecycle
    void Startup();
    void RunMainLoop();
    void Shutdown();
    void RunFrame();

    // utility interfaces
    void     Quit();
    bool     IsQuitting() const               { return m_isQuitting; }
    bool     IsRebooting() const              { return m_isRebooting; }

private:
    // private lifecycle
    void    BeginFrame();
    void    Update();
    void    Render() const;
    void    EndFrame();

    // utilities
    void    PopulateGameConfigBlackboard();
    void    HandleGeneralInputEvents();

private:
    bool      m_isQuitting       = false;
    bool      m_isRebooting      = false;
};

