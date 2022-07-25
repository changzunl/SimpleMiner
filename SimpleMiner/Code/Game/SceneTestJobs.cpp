#include "SceneTestJobs.hpp"

#include "Game.hpp"
#include "SceneAttract.hpp"

#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/StringUtils.hpp"


extern RandomNumberGenerator rng;

TestSleepJob::TestSleepJob() 
	: Job(JOB_TYPE_TEST_SLEEP)
{
	m_destroyAfterFinished = false;
}

TestSleepJob::TestSleepJob(int sleepTime) 
	: Job(JOB_TYPE_TEST_SLEEP)
    , m_sleepTime(sleepTime)
{
	m_destroyAfterFinished = false;
}

void TestSleepJob::Execute()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepTime));
}

void TestSleepJob::OnFinished()
{
	m_finished = true;
}

SceneTestJobs::SceneTestJobs(Game* game)
	: Scene(game)
{
}

SceneTestJobs::~SceneTestJobs()
{
}

void SceneTestJobs::Initialize()
{
	DebugRenderClear();

	for (auto& ptr : m_jobs)
		ptr = new TestSleepJob();

	for (auto& ptr : m_jobs)
		ptr->m_sleepTime = rng.RollRandomIntInRange(50, 3000);

	for (auto& ptr : m_jobs)
		g_theJobSystem->QueueJob(ptr);
}

void SceneTestJobs::Shutdown()
{
	for (auto& ptr : m_jobs)
		ptr->m_sleepTime = 1;

	std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	g_theJobSystem->FinishUpJobsOfType(JOB_TYPE_TEST_SLEEP);

	for (auto& ptr : m_jobs)
	{
		delete ptr;
		ptr = nullptr;
	}
}

void SceneTestJobs::Update()
{
	HandleInput();

	DebugAddInfo();
}

void SceneTestJobs::UpdateCamera()
{
	m_worldCamera[0].SetPerspectiveView(float(g_theWindow->GetClientDimensions().x) / float(g_theWindow->GetClientDimensions().y), 60.0f, 0.1f, 100.0f);
	m_worldCamera[0].SetRenderTransform(Vec3(0, -1, 0), Vec3(0, 0, 1), Vec3(1, 0, 0));
	m_worldCamera[0].SetViewTransform(Vec3::ZERO, EulerAngles(0, 0, GetLifeTime() * 50));

	m_screenCamera[0].SetOrthoView(g_theRenderer->GetViewport().m_mins, g_theRenderer->GetViewport().m_maxs);

	Scene::UpdateCamera();
}

void SceneTestJobs::RenderWorld() const
{

}

void SceneTestJobs::RenderUI() const
{
	AABB2 viewport = g_theRenderer->GetViewport();

	static VertexList verts;
	verts.clear();

	float width = viewport.GetDimensions().x / 40.0f;
	float height = viewport.GetDimensions().y / 20.0f;

	AABB2 box;
	box.SetDimensions(Vec2(width, height) * 0.95f);

	for (size_t y = 0; y < 20; y++)
		for (size_t x = 0; x < 40; x++)
		{
			auto* ptr = m_jobs[x + y * 40];

			Rgba8 color;

			switch (ptr->GetState())
			{
			case JobState::QUEUED:
				color = Rgba8::RED;
				break;
			case JobState::EXECUTING:
				color = Rgba8(255, 255, 0);
				break;
			case JobState::FINISHED:
				color = ptr->m_finished ? Rgba8::BLUE : Rgba8::GREEN;
				break;
			}

			box.SetCenter(Vec2(width * 0.5f, height * 0.5f) + Vec2(width * (float)x, height * (float)y));
			AddVertsForAABB2D(verts, box, color);
		}

	g_theRenderer->DrawVertexArray(verts);

	DebugRenderScreen(GetScreenCamera());
}

void SceneTestJobs::HandleInput()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		SceneAttract* attract = new SceneAttract(m_game);
		m_game->LoadScene(attract);
		m_game->m_clickSound->PlaySound();
		return;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_1))
	{
		g_theJobSystem->FinishUpJobsOfType(JOB_TYPE_TEST_SLEEP, 1);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_2))
	{
		g_theJobSystem->FinishUpJobsOfType(JOB_TYPE_TEST_SLEEP);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_R))
	{
		Shutdown();
		Initialize();
	}
}

void SceneTestJobs::DebugAddInfo() const
{
	std::string text = Stringf("Debug jobs visualizer: 1 = retrieve 1 job, 2 = retrieve all jobs, R = reload, reload and quit freeze for 5 seconds");
	DebugAddMessage(text, 0.0f, Rgba8::WHITE, Rgba8::WHITE);
}
