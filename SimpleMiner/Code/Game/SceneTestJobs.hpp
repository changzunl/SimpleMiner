#include "Scene.hpp"

#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Core/JobSystem.hpp"


constexpr int JOB_TYPE_TEST_SLEEP = 998;

class TestSleepJob : public Job
{
public:
	TestSleepJob();
	TestSleepJob(int sleepTime);

private:
	virtual void Execute() override;
	virtual void OnFinished() override;

public:
	std::atomic_int m_sleepTime    = 0;
	bool            m_finished     = false;
};

class SceneTestJobs : public Scene
{
public:
	SceneTestJobs(Game* game);
	virtual ~SceneTestJobs();
	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual void Update() override;
	virtual void UpdateCamera() override;
	virtual void RenderWorld() const override;
	virtual void RenderUI() const override;

private:
	void HandleInput();
	void DebugAddInfo() const;

private:
	TestSleepJob* m_jobs[800] = {};
	bool m_quit = false;
};

