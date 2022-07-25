#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

bool EpsilonEquals(float a, float b)
{
	// constexpr float epsilon = 1.192092896e-07f;
	constexpr float epsilon = 0.001f;

	return fabsf(a - b) < epsilon;
}

bool EpsilonEquals(const Vec3& a, const Vec3& b)
{
	return EpsilonEquals(a.x, b.x)
		&& EpsilonEquals(a.y, b.y)
		&& EpsilonEquals(a.z, b.z);
}

bool EpsilonEquals(const EulerAngles& a, const EulerAngles& b)
{
	return EpsilonEquals(a.m_yawDegrees  , b.m_yawDegrees  )
		&& EpsilonEquals(a.m_pitchDegrees, b.m_pitchDegrees)
		&& EpsilonEquals(a.m_rollDegrees , b.m_rollDegrees );
}

void DebugEulerToVec()
{
	// 	DebuggerPrintf("Test for euler angles conversion:\n");
	// 
	// 	EulerAngles rotation(360, 0, 0);
	// 
	// 	EulerAngles rotation2 = DirectionToRotation(rotation.GetVectorXForward());
	// 
	// 	DebuggerPrintf(EpsilonEquals(rotation.GetVectorXForward(), rotation2.GetVectorXForward()) ? "true" : "false");
	// 	DebuggerPrintf("\n");
}

#include "Networking.hpp"
#include "Engine/Core/ByteBuffer.hpp"

void DebugBuffer()
{
// 	constexpr unsigned int bufferSize = 64;
// 
// 	PacketBuffer buffer;
// 
// 	buffer.Write(123321123321ui64);
// 
// 	PacketBufferObject clientBuffer(bufferSize);
// 
// 	HPKT pkt0 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData());
// 
// 	UCHAR(*pData)[bufferSize] = (UCHAR(*)[bufferSize]) clientBuffer.ResolveData(pkt0);
// 
// 	clientBuffer.FreePacket(pkt0);
// 
// 	buffer.Write(321123ui32);
// 	HPKT pkt1 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData());
// 	buffer.Write(12345ui16);
// 	HPKT pkt2 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData());
// 	buffer.Write(132ui8);
// 	HPKT pkt3 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData());
// 	buffer.Write(Rgba8(123, 231, 213, 132));
// 	HPKT pkt4 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData()); // full error
// 	clientBuffer.FreePacket(pkt1);
// 	clientBuffer.FreePacket(pkt2);
// 	clientBuffer.CleanAndRoll(); // gc
// 	clientBuffer.FreePacket(pkt3);
// 	clientBuffer.CleanAndRoll(); // gc
// 	HPKT pkt5 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData());
// 	buffer.Write(Vec3(1.0f, 12.0f, 123.0f));
// 	HPKT pkt6 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData()); // full error
// 
// 	UCHAR* p1 = clientBuffer.ResolveData(pkt1);
// 	UCHAR* p2 = clientBuffer.ResolveData(pkt2);
// 	UCHAR* p3 = clientBuffer.ResolveData(pkt3);
// 	UCHAR* p4 = clientBuffer.ResolveData(pkt4);
// 	UCHAR* p5 = clientBuffer.ResolveData(pkt5);
// 	UCHAR* p6 = clientBuffer.ResolveData(pkt6);
}

#include "Engine/Core/JobSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

void DebugTestJobSystem()
{
	class TestJob : public Job
	{
	public:
		TestJob(int runDurationMs)
			: m_runDurationMs(runDurationMs)
		{
		}

	private:
		virtual void Execute() override
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(m_runDurationMs));
			DebuggerPrintf("[Job-%d] Slept for %dms\n", m_id, m_runDurationMs);
		}

		virtual void OnFinished() override
		{
			DebuggerPrintf("[Job-%d] Finished\n", m_id);
		}

	private:
		int m_runDurationMs;
	};

	RandomNumberGenerator rng;

	JobSystem* system = new JobSystem(JobSystemConfig());

	system->Startup();

	for (int i = 0; i < 5; i++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(rng.RollRandomIntInRange(1, 50)));

		for (int j = 0; j < 10; j++)
			system->QueueJob(new TestJob(rng.RollRandomIntInRange(50, 100)));

		system->FinishUpJobs();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(5000));


	for (int i = 0; i < 5; i++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(rng.RollRandomIntInRange(1, 50)));

		for (int j = 0; j < 10; j++)
			system->QueueJob(new TestJob(rng.RollRandomIntInRange(50, 100)));

		system->FinishUpJobs();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	system->Shutdown();

	delete system;
}

bool DebugMain()
{
// 	DebugEulerToVec();
// 	DebugBuffer();
//	DebugTestJobSystem();

	return false;
}

