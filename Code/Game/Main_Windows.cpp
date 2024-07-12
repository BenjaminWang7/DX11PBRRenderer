#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in VERY few places (and .CPPs only)
#include <math.h>
#include <cassert>
#include <crtdbg.h>
#include "Engine/Math/Vec2.hpp"
#include "App.hpp"
//-----------------------------------------------------------------------------------------------
// #SD1ToDo: Once we have a Renderer class, we'll remove most OpenGL references out of Main_Win32.cpp
// Both of the following lines should be moved to the top of Engine/Renderer/Renderer.cpp
//
	// Link in the OpenGL32.lib static library
#include "../../../Engine/Code/Engine/Core/Vertex_PCU.hpp"
#include "../../../Engine/Code/Engine/Renderer/Renderer.hpp"
#include <time.h>
#include "Engine/Input/InputSystem.hpp"
//-----------------------------------------------------------------------------------------------
// #SD1ToDo: Move this useful macro to a more central place, e.g. Engine/Core/EngineCommon.hpp
//
#define UNUSED(x) (void)(x);


//-----------------------------------------------------------------------------------------------
// #SD1ToDo: This will go away once we add a Window engine class later on.
// 
 // We are requesting a 1:1 aspect (square) window area


//-----------------------------------------------------------------------------------------------
// #SD1ToDo: Move each of these items to its proper place, once that place is established
// 
bool g_isQuitting = false;							// ...becomes App::m_isQuitting instead
//HWND g_hWnd = nullptr;								// ...becomes void* WindowContext::m_windowHandle
//HDC g_displayDeviceContext = nullptr;				// ...becomes void* WindowContext::m_displayContext
///*HGLRC g_openGLRenderingContext = nullptr;*/			// ...becomes void* Renderer::m_apiRenderingContext
//const char* APP_NAME = "SD1-A3: Starship Prototype";	// ...becomes ??? (Change this per project!)

App* g_theApp = nullptr;

extern InputSystem* g_input;

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain( _In_ HINSTANCE applicationInstanceHandle, _In_opt_ HINSTANCE previousIntance, _In_ LPSTR commandLineString, _In_ int nShowCmd)
{
	UNUSED( commandLineString );
	UNUSED( applicationInstanceHandle );
	UNUSED( previousIntance );
	UNUSED( nShowCmd );

	g_theApp = new App();
	g_theApp->Startup();
	g_theApp->Run();
	g_theApp->Shutdown();
	delete g_theApp;
	g_theApp = nullptr;

	return 0;
}


