#pragma once
#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>
#include <string>

constexpr int KEYBOARD_INPUT_NUM = 256;
constexpr int SOUND_MAX_NUM = 100;

class Clock;
class Shader;
class OBJMesh;

enum class PBRTexutreType
{
	BrikWall,
	Iron,
	FutureWall,
	Cloth,
	Sofa,
	RustIron,
	Gold,
	TypeNum,
};

enum class ObjType
{
	AnimCharacter1,
	AnimCharacter2,
	TypeNum,
};

class App
{
public:

	App();
	~App();
	void Startup();
	void Shutdown();
	void RunFrame();

	bool IsOuitting() const { return m_isQuitting; }
	bool HandleKeypressed( unsigned char keyCode );
	bool HandlekeyReleased( unsigned char keyCode );
	bool HandleQuitRequested();

	void BeginFrame();
	void Update( float deltaSeconds );
	void Render() const;
	void EndFrame();
	bool IsKeyDown( unsigned char keycode )const;
	bool IsKeyPressed( unsigned char keycode )const;
	void LoadMusic();
	void LoadTexutre();
	void LoadOBJFiles();
	void UpdateAppInput();
	void UpdateAppState( float& deltaSeconds );
	void UpdateGameState();
	void UpdateAttractMode( float deltaSeconds );
	void Run();
	OBJMesh* CreateObjMesh( std::string finePath );
public:
	bool m_isQuitting = false;
	bool m_isPaused = false;
	bool m_isSlowMo = false;
	bool m_TickPerFrame = false;
	float m_lastFrame;
	bool m_isWantQuit = false;
	float m_timer = 0.f;
	Clock* m_gameClock = nullptr;
	Shader* m_diffuseShader = nullptr;
private:
	bool m_isKeyDown[KEYBOARD_INPUT_NUM] = {};
	bool m_isKeyPressed[KEYBOARD_INPUT_NUM] = {};
	Camera m_attractScreenCamera;
	float m_attractScreenThinkness = 0.f;
	Camera m_devCamera;
	bool m_isFinishAttractMode = false;
	bool m_isShowCursor = false;
};
