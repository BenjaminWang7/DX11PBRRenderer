#define UNUSED(x) (void)(x);
#include <math.h>
#include <windows.h>
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/SimpleTriangleFont.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/NamedString.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/OBJMesh.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/Player.hpp"

enum class XboxButtonID;
InputSystem* g_input = nullptr;
AudioSystem* g_audio = nullptr;
Renderer* g_theRenderer = nullptr;
SoundID g_soundArray[SOUND_MAX_NUM] = {};
SoundPlaybackID g_playbackID[SOUND_MAX_NUM] = {};
Window* g_theWindow = nullptr;
RandomNumberGenerator* rng = nullptr;
BitmapFont* g_bitMapFont = nullptr;
Texture* g_testTex = nullptr;
Game* g_theGame = nullptr;
Texture* g_cubeTestTex = nullptr;
Texture* g_normalMap[(int)PBRTexutreType::TypeNum];
Texture* g_albedoMap[(int)PBRTexutreType::TypeNum];
Texture* g_aoMap[(int)PBRTexutreType::TypeNum];
Texture* g_roughnessMap[(int)PBRTexutreType::TypeNum];
Texture* g_metallicMap[(int)PBRTexutreType::TypeNum];
OBJMesh* g_objMeshes[(int)ObjType::TypeNum];
Texture* g_HDRTexture = nullptr;
Texture* g_skyboxTexture = nullptr;
Texture* g_BRDFTexture = nullptr;


extern DevConsole* g_theConsole;
extern EventSystem* g_theEventSystem;


App::App()
{
	rng = new RandomNumberGenerator();
}

App::~App()
{
}



void App::Startup()
{
	EventSystemConfig eventSystemConfig;
	g_theEventSystem = new EventSystem( eventSystemConfig );
	g_theEventSystem->StartUp();

	InputConfig inputConfig;
	g_input = new InputSystem( inputConfig );
	g_input->StartUp();

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_input;
	windowConfig.m_windowTitle = "Protogame3D";
	windowConfig.m_clientAspect = 2.f;
	g_theWindow = new Window( windowConfig );
	g_theWindow->Startup();

	AudioConfig audioConfig;
	g_audio = new AudioSystem( audioConfig );
	g_audio->Startup();

	RenderConfig renderConfig;
	renderConfig.m_window = g_theWindow;
	renderConfig.m_isEnableMSAA = true;
	renderConfig.m_MSAASampleCount = 4;
	g_theRenderer = new Renderer( renderConfig );
	g_theRenderer->StartUp();

	g_theRenderer->SetSamplerMode( SamplerMode::BILINEAR_WRAP );
	LoadMusic();
	LoadTexutre();
	m_gameClock = new Clock();
	Clock::TickSystemClock();

	m_devCamera.SetOrthographicView( Vec2( 0.f, 0.f ), Vec2( 16.f, 8.f ) );
	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_font = g_bitMapFont;
	devConsoleConfig.m_renderer = g_theRenderer;
	devConsoleConfig.m_cameraAABB.m_mins = m_devCamera.GetOrthographicBottomLeft();
	devConsoleConfig.m_cameraAABB.m_maxs = m_devCamera.GetOrthographicTopRight();
	g_theConsole = new DevConsole( devConsoleConfig );
	g_theConsole->StartUp();


	
	g_theConsole->AddLine( DevConsole::INFO_MAJOR, "App::Starting" );
	g_theConsole->AddLine( Rgba8( 10, 200, 10, 255 ), "Use DebugToggle command to toggle debug render" );
	g_theConsole->AddLine( Rgba8( 10, 200, 10, 255 ), "Use DebugClear command to clear all debug render" );

	m_attractScreenCamera.SetOrthographicView( Vec2( 0.f, 0.f ), Vec2( 16.f, 8.f ) );

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_theRenderer;
	debugRenderConfig.m_bitMapFont = g_bitMapFont;
	DebugRenderSystemStartup( debugRenderConfig );

	std::string shaderPath = "Data/Shaders/Diffuse.hlsl";
	m_diffuseShader = g_theRenderer->CreateShader( "Diffuse", shaderPath, VertexType::PCUTBN );

	LoadOBJFiles();

	g_theGame = new Game();
	g_theGame->StartUp();
}

void App::Shutdown()
{
	g_audio->Shutdown();
	delete g_audio;
	g_audio = nullptr;
	g_theRenderer->Shutdown();
	delete g_theRenderer;
	g_theRenderer = nullptr;
	if (g_theGame != nullptr)
	{
		g_theGame->ShutDown();
		delete g_theGame;
		g_theGame = nullptr;
	}

}

void App::RunFrame()
{
	Clock::TickSystemClock();
	float deltaSeconds = m_gameClock->GetDeltaSeconds();
	BeginFrame();
	Update( deltaSeconds );
	Render();
 	EndFrame();
}

void App::BeginFrame()
{
	g_theWindow->BeginFrame();
	g_audio->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theConsole->BeginFrame();
}

void App::Update( float deltaSeconds )
{
	UpdateAppInput();
	UpdateAppState( deltaSeconds );
	if (g_theGame != nullptr)
	{
		if (m_isFinishAttractMode == true)
		{
			g_theGame->Update( deltaSeconds );
		}
	}
	UpdateGameState();
	UpdateAttractMode( deltaSeconds );
	g_input->BeginFrame();
}

void App::Render() const
{
	g_theRenderer->BeginCamera( m_attractScreenCamera );
	g_theRenderer->ClearScreen( Rgba8( 10, 10, 10, 255 ) );

	if (m_isFinishAttractMode == false)
	{
		std::vector<Vertex_PCU> textVerts;
		g_bitMapFont->AddVertsForText2D( textVerts, Vec2( 5.9f, 6.f ), 0.5f, "PRR Renderer", Rgba8::PINK );
		g_bitMapFont->AddVertsForText2D( textVerts, Vec2( 7.f, 3.f ), 0.2f, "Space to Start", Rgba8::PINK );
		g_theRenderer->BindTexture( &g_bitMapFont->GetTexture() );
		g_theRenderer->SetModelConstants();
		g_theRenderer->DrawVertexArray( (int)textVerts.size(), textVerts.data() );

		Vec2 screenCenter = (m_attractScreenCamera.GetOrthographicBottomLeft() + m_attractScreenCamera.GetOrthographicTopRight()) / 2.f + Vec2(0.f, -1.f);
		DrawRing( screenCenter, 2.f, m_attractScreenThinkness, Rgba8( 255, 100, 0, 255 ) );
	}
	Mat44 mat;
	g_theRenderer->SetModelConstants( mat, Rgba8::WHITE );
	g_theRenderer->EndCamera( m_attractScreenCamera );

	if (g_theGame != nullptr)
	{
		if (m_isFinishAttractMode == true)
		{
			g_theGame->Render();
		}
	}

	g_theRenderer->BeginCamera( m_devCamera );
	g_theRenderer->SetModelConstants();
	g_theConsole->Render();
	g_theRenderer->EndCamera( m_devCamera );
}

void App::EndFrame()
{
	g_theWindow->EndFrame();
	g_input->EndFrame();
	g_audio->EndFrame();
	g_theRenderer->EndFrame();
}


bool App::HandleKeypressed( unsigned char keyCode )
{
	m_isKeyDown[keyCode] = true;
	return false;
}

bool App::HandlekeyReleased( unsigned char keyCode )
{
	m_isKeyDown[keyCode] = false;

	return false;
}

bool App::HandleQuitRequested()
{
	return false;
}

bool App::IsKeyDown( unsigned char keycode )const
{
	return m_isKeyDown[keycode];
}

bool App::IsKeyPressed( unsigned char keycode )const
{
	if (m_isKeyDown[keycode] && !m_isKeyPressed[keycode])
	{
		return true;
	}
	else
	{
		return false;
	}
}


void App::LoadMusic()
{

}

void App::LoadTexutre()
{
	g_testTex = g_theRenderer->CreateTextureFromFile( "Data/Images/PixelTest.png" );
	g_bitMapFont = g_theRenderer->CreateOrGetBitmapFont( "Data/Fonts/SquirrelFixedFont.png" );
	g_cubeTestTex = g_theRenderer->CreateTextureFromFile( "Data/Images/Test_StbiFlippedAndOpenGL.png" );

	g_normalMap[(int)PBRTexutreType::BrikWall] = g_theRenderer->CreateTextureFromFile( "Data/Images/normalmap.png", true );
	g_albedoMap[(int)PBRTexutreType::BrikWall] = g_theRenderer->CreateTextureFromFile( "Data/Images/albedo.png", true );
	g_aoMap[(int)PBRTexutreType::BrikWall] = g_theRenderer->CreateTextureFromFile( "Data/Images/ao.png", true );
	g_roughnessMap[(int)PBRTexutreType::BrikWall] = g_theRenderer->CreateTextureFromFile( "Data/Images/roughness.png", true );
	g_metallicMap[(int)PBRTexutreType::BrikWall] = g_theRenderer->CreateTextureFromFile( "Data/Images/metallic.png", true );

	g_normalMap[(int)PBRTexutreType::Iron] = g_theRenderer->CreateTextureFromFile( "Data/Images/normalmap2.png", true );
	g_albedoMap[(int)PBRTexutreType::Iron] = g_theRenderer->CreateTextureFromFile( "Data/Images/albedo2.png", true );
	g_aoMap[(int)PBRTexutreType::Iron] = g_theRenderer->CreateTextureFromFile( "Data/Images/ao2.png", true );
	g_roughnessMap[(int)PBRTexutreType::Iron] = g_theRenderer->CreateTextureFromFile( "Data/Images/roughness2.png", true );
	g_metallicMap[(int)PBRTexutreType::Iron] = g_theRenderer->CreateTextureFromFile( "Data/Images/metallic2.png", true );

	g_normalMap[(int)PBRTexutreType::FutureWall] = g_theRenderer->CreateTextureFromFile( "Data/Images/normalmap3.png", true );
	g_albedoMap[(int)PBRTexutreType::FutureWall] = g_theRenderer->CreateTextureFromFile( "Data/Images/albedo3.png", true );
	g_aoMap[(int)PBRTexutreType::FutureWall] = g_theRenderer->CreateTextureFromFile( "Data/Images/ao3.png", true );
	g_roughnessMap[(int)PBRTexutreType::FutureWall] = g_theRenderer->CreateTextureFromFile( "Data/Images/roughness3.png", true );
	g_metallicMap[(int)PBRTexutreType::FutureWall] = g_theRenderer->CreateTextureFromFile( "Data/Images/metallic3.png", true );

	g_normalMap[(int)PBRTexutreType::Cloth] = g_theRenderer->CreateTextureFromFile( "Data/Images/normalmap4.png", true );
	g_albedoMap[(int)PBRTexutreType::Cloth] = g_theRenderer->CreateTextureFromFile( "Data/Images/albedo4.png", true );
	g_aoMap[(int)PBRTexutreType::Cloth] = g_theRenderer->CreateTextureFromFile( "Data/Images/ao4.png", true );
	g_roughnessMap[(int)PBRTexutreType::Cloth] = g_theRenderer->CreateTextureFromFile( "Data/Images/roughness4.png", true );
	g_metallicMap[(int)PBRTexutreType::Cloth] = g_theRenderer->CreateTextureFromFile( "Data/Images/metallic4.png", true );

	g_normalMap[(int)PBRTexutreType::Sofa] = g_theRenderer->CreateTextureFromFile( "Data/Images/normalmap5.png", true );
	g_albedoMap[(int)PBRTexutreType::Sofa] = g_theRenderer->CreateTextureFromFile( "Data/Images/albedo5.png", true );
	g_aoMap[(int)PBRTexutreType::Sofa] = g_theRenderer->CreateTextureFromFile( "Data/Images/ao5.png", true );
	g_roughnessMap[(int)PBRTexutreType::Sofa] = g_theRenderer->CreateTextureFromFile( "Data/Images/roughness5.png", true );
	g_metallicMap[(int)PBRTexutreType::Sofa] = g_theRenderer->CreateTextureFromFile( "Data/Images/metallic5.png", true );

	g_normalMap[(int)PBRTexutreType::RustIron] = g_theRenderer->CreateTextureFromFile( "Data/Images/normalmap6.png", true );
	g_albedoMap[(int)PBRTexutreType::RustIron] = g_theRenderer->CreateTextureFromFile( "Data/Images/albedo6.png", true );
	g_aoMap[(int)PBRTexutreType::RustIron] = g_theRenderer->CreateTextureFromFile( "Data/Images/ao6.png", true );
	g_roughnessMap[(int)PBRTexutreType::RustIron] = g_theRenderer->CreateTextureFromFile( "Data/Images/roughness6.png", true );
	g_metallicMap[(int)PBRTexutreType::RustIron] = g_theRenderer->CreateTextureFromFile( "Data/Images/metallic6.png", true );

	g_normalMap[(int)PBRTexutreType::Gold] = g_theRenderer->CreateTextureFromFile( "Data/Images/normalmap7.png", true );
	g_albedoMap[(int)PBRTexutreType::Gold] = g_theRenderer->CreateTextureFromFile( "Data/Images/albedo7.png", true );
	g_aoMap[(int)PBRTexutreType::Gold] = nullptr;
	g_roughnessMap[(int)PBRTexutreType::Gold] = g_theRenderer->CreateTextureFromFile( "Data/Images/roughness7.png", true );
	g_metallicMap[(int)PBRTexutreType::Gold] = g_theRenderer->CreateTextureFromFile( "Data/Images/metallic7.png", true );

 	g_HDRTexture = g_theRenderer->CreateTextureFromFile( "Data/HDR/environment.hdr" );
	g_skyboxTexture = g_theRenderer->CreateTextureFromFile( "Data/Images/StandardCubeMap3.png" );
	g_BRDFTexture = g_theRenderer->CreateTextureFromFile( "Data/Images/bdrf.png" );

}

void App::LoadOBJFiles()
{
	g_objMeshes[(int)ObjType::AnimCharacter1] = CreateObjMesh( "Data/ObjModels/Amber.obj" );
	g_objMeshes[(int)ObjType::AnimCharacter2] = CreateObjMesh( "Data/ObjModels/Lumine.obj" );
}

OBJMesh* App::CreateObjMesh( std::string finePath )
{
	OBJMesh* obj = new OBJMesh();
	obj->ReadFile( finePath );
	return obj;
}

void App::UpdateAppInput()
{
	g_input->m_controllers[0].Update();
	if (g_input->WasKeyJustPressed( KEYCODE_ESC ) || g_input->GetController( 0 ).WasButtonJustPressed( XboxButtonID::XboxKeyBack ))
	{
		if (m_isFinishAttractMode == true)//In Game
		{
			m_isFinishAttractMode = false;
		}
		else
		{
			m_isQuitting = true;
		}
	}

	if (g_input->WasKeyJustPressed( KEYCODE_TILDE ))
	{
		g_theConsole->ToggleOpen();
	}

	if (g_input->WasKeyJustPressed( KEYCODE_SPACE ) || g_input->GetController( 0 ).WasButtonJustPressed( XboxButtonID::XboxKeyStart ))
	{
		if (m_isFinishAttractMode == false)
		{
			m_isPaused = false;
			g_input->m_keyStates[32].m_isPressed = false;
			m_isFinishAttractMode = true;
		}
	}

	if (g_input->WasKeyJustPressed( 'M' ))
	{
		m_isShowCursor = !m_isShowCursor;
	}

	if (g_theConsole->GetIsGameClose())
	{
		m_isQuitting = true;
	}

}

void App::UpdateAppState( float& deltaSeconds )
{

	if (m_TickPerFrame == true && m_isPaused != true)
	{
		m_isPaused = true;
	}
	//else if (m_TickPerFrame == true && m_isPaused == true)
	//{
	//	if (m_game != nullptr)
	//	{
	//		m_game->Update( deltaSeconds );
	//		m_TickPerFrame = false;
	//	}
	//}
	if (!m_isFinishAttractMode || g_theConsole->IsOpen() || m_isShowCursor)
	{
		g_input->SetCursorMode( false, false );
	}
	else
	{
		//g_input->SetCursorMode( true, true );
	}

	if (m_isPaused == true)
	{
		deltaSeconds = 0.f;
	}

	else if (m_isSlowMo == true)
	{
		deltaSeconds *= 0.1f;
	}

}


void App::UpdateGameState()
{

}

void App::UpdateAttractMode( float deltaSeconds )
{
	m_timer += deltaSeconds;
	m_attractScreenThinkness = sinf( 3.f * m_timer ) * 0.4f;
}



void App::Run()
{
	while (!m_isQuitting)
	{
		RunFrame();
	}
}




