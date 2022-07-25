#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in very few places
#include <objbase.h>
#include "App.hpp"

#define UNUSED(x) (void)(x);

bool DebugMain();

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int)
{
	UNUSED(applicationInstanceHandle);
	UNUSED(commandLineString);
	::CoInitialize(nullptr);

	g_theApp = new App();

	if (!DebugMain())
	{
		g_theApp->Startup();
		g_theApp->RunMainLoop();
		g_theApp->Shutdown();
	}
	
	delete g_theApp;
	g_theApp = nullptr;

	::CoUninitialize();
	return 0;
}

