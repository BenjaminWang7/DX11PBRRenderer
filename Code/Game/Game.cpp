#include "Game.hpp"
#include "App.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include <math.h>
#include "GameCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/SimpleTriangleFont.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Player.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Clock.hpp"
#include "Game/Prop.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/SpotLight.hpp"
#include "Engine/Renderer/PointLight.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/Material.hpp"
#include "Engine/Core/OBJMesh.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Renderer/HDRCubeMap.hpp"
#include "Engine/Math/SkyBox.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Imgui/imgui_internal.h"
#include <string>
#include <DirectXMath.h>
extern App* g_theApp;
extern RandomNumberGenerator* rng;
extern float g_randomRotationRate;
extern Window* g_theWindow;
extern InputSystem* g_input;
extern AudioSystem* g_audio;
extern Renderer* g_theRenderer;
extern Texture* g_cubeTestTex;
extern BitmapFont* g_bitMapFont;
extern bool g_isDebugRenderVisible;
extern bool g_isDebugRenderOpen;
extern BillboardType g_billBoardType;
extern Texture* g_normalMap[(int)PBRTexutreType::TypeNum];
extern Texture* g_albedoMap[(int)PBRTexutreType::TypeNum];
extern Texture* g_aoMap[(int)PBRTexutreType::TypeNum];
extern Texture* g_roughnessMap[(int)PBRTexutreType::TypeNum];
extern Texture* g_metallicMap[(int)PBRTexutreType::TypeNum];
extern OBJMesh* g_objMeshes[(int)ObjType::TypeNum];
extern Texture* g_HDRTexture;
extern Texture* g_skyboxTexture;
extern Texture* g_cubeTestTex;
static const int k_modelConstantSlot = 9;


void Game::StartUp()
{
	g_theRenderer->ClearScreen( Rgba8( 50, 50, 50, 255 ) );
	std::string outlineShaderPath = "Data/Shaders/Outline.hlsl";
	m_outlineShader = g_theRenderer->CreateShader( "Outline", outlineShaderPath );

	std::string shaderPath = "Data/Shaders/HDR.hlsl";
	m_HDRShader = g_theRenderer->CreateShader( "HDR", shaderPath, VertexType::PCUTBN );

	int skyboxTexCboSize = sizeof( Vec4 );
	m_skyboxTextureCBO = g_theRenderer->CreateConstantBuffer( skyboxTexCboSize );
	SetTextureConstants( Vec4( (float)g_cubeTestTex->GetDimensions().x, (float)g_cubeTestTex->GetDimensions().y, 0.f, 0.f ) );

	m_hdrCubemap = new HDRCubeMap( g_theRenderer, g_theWindow );
	std::string hdrPath = "Data/HDR/rostock_laage_airport_4k.hdr";
	std::string cubeMapShaderPath = "Data/Shaders/EquirectangulattoCubemap.hlsl";
	std::string irradianceShaderPath = "Data/Shaders/Irradiance.hlsl";
	std::string prefilterShaderPath = "Data/Shaders/PreFilter.hlsl";
	m_hdrCubemap->LoadAndDrawHDRCubeMapFromFile( hdrPath, cubeMapShaderPath, irradianceShaderPath, prefilterShaderPath );
	
	m_skyBox = new SkyBox( g_theRenderer );
	std::string skyShaderPath = "Data/Shaders/Sky.hlsl";
	m_skyShader = g_theRenderer->CreateShader( "Sky", skyShaderPath );

	m_screenCamera.SetOrthographicView( Vec2( 0.f, 0.f ), Vec2( 16.f, 8.f ) );

	m_FPSTimer = new Timer( 0.5f, g_theApp->m_gameClock );
	m_FPSTimer->Start();

	m_player = new Player( this, Vec3( 0.f, 0.f, 1.f ) );
	m_player->StartUp();

	PropsInitilization();
	LightInitilization();
	g_billBoardType = BillboardType::FULL_CAMERA_OPPOSING;

	g_isDebugRenderVisible = true;
	g_isDebugRenderOpen = true;

	DebugAddWorldArrow( Vec3( 0.f, 0.f, 0.f ), Vec3( 1.f, 0.f, 0.f ), 0.05f, -1.f, Rgba8::RED, Rgba8::RED );
	DebugAddWorldArrow( Vec3( 0.f, 0.f, 0.f ), Vec3( 0.f, 1.f, 0.f ), 0.05f, -1.f, Rgba8::GREEN, Rgba8::GREEN );
	DebugAddWorldArrow( Vec3( 0.f, 0.f, 0.f ), Vec3( 0.f, 0.f, 1.f ), 0.05f, -1.f, Rgba8::BLUE, Rgba8::BLUE );
	DebugAddWorldBillboardText( "Lighting Models", Vec3( 10.f, 0.f, 5.f ), 0.2f, Vec2( 0.5f, 0.5f ), -1.f, Rgba8( 127, 221, 251 ) );
	DebugAddWorldBillboardText( "Spot Light", Vec3( -3.f, -10.f, 6.f ), 0.2f, Vec2( 0.5f, 0.5f ), -1.f, Rgba8( 127, 221, 251 ) );
	DebugAddWorldBillboardText( "Point Lights", Vec3( -10.f, 0.f, 7.f ), 0.2f, Vec2( 0.5f, 0.5f ), -1.f, Rgba8( 127, 221, 251 ) );
	g_input->SetCursorMode( true, true );

	//std::string postProccessShaderPath = "Data/Shaders/PostProccess.hlsl";
	//m_postProcessShader = g_theRenderer->CreateShader( "PostProccess", postProccessShaderPath );
}

void Game::ShutDown()
{
	DebugRenderSystemShutdown();
	delete m_skyboxTextureCBO;
	m_skyboxTextureCBO = nullptr;

	delete m_skyBox;
	m_skyBox = nullptr;

	delete m_hdrCubemap;
	m_hdrCubemap = nullptr;

	for (int i = 0; i < m_allProps.size(); i++)
	{
		if (m_allProps[i] != nullptr)
		{
			delete m_allProps[i];
			m_allProps[i] = nullptr;
		}
	}
}

void Game::PropsInitilization()
{
	int sphereSlices = 32;

	SphereProp* sphereProp1 = new SphereProp( this );
	sphereProp1->m_radius = 0.5f;
	sphereProp1->m_material = new Material( MaterialType::BlinnPhong, 1.f, 1.f, 1.f, 128.f );
	sphereProp1->m_position = Vec3( 10.f, 3.f, 2.f );
	sphereProp1->m_vertexes.reserve( 200 );
	AddVertsForSphere3D( sphereProp1->m_vertexes, Vec3( 0.f, 0.f, 0.f ), sphereProp1->m_radius, Rgba8::WHITE, AABB2::ZERO_TO_ONE, sphereSlices );
	sphereProp1->StartUp();
	m_sphereProps.push_back( sphereProp1 );
	m_allProps.push_back( sphereProp1 );

	SphereProp* sphereProp2 = new SphereProp( this );
	sphereProp2->m_radius = 0.5f;
	sphereProp2->m_material = new Material( MaterialType::Lambert, 1.f, 1.f, 1.f, 1.f );
	sphereProp2->m_position = Vec3( 10.f, 1.f, 2.f );
	sphereProp2->m_vertexes.reserve( 200 );
	AddVertsForSphere3D( sphereProp2->m_vertexes, Vec3( 0.f, 0.f, 0.f ), sphereProp2->m_radius, Rgba8( 125, 228, 163, 255 ), AABB2::ZERO_TO_ONE, sphereSlices );
	sphereProp2->StartUp();
	m_sphereProps.push_back( sphereProp2 );
	m_allProps.push_back( sphereProp2 );

	SphereProp* sphereProp3 = new SphereProp( this );
	sphereProp3->m_radius = 0.5f;
	sphereProp3->m_material = new Material( MaterialType::WrapLight, 1.f, 1.f, 1.f, 1.f );
	sphereProp3->m_position = Vec3( 10.f, -1.f, 2.f );
	sphereProp3->m_vertexes.reserve( 200 );
	AddVertsForSphere3D( sphereProp3->m_vertexes, Vec3( 0.f, 0.f, 0.f ), sphereProp3->m_radius, Rgba8::PINK, AABB2::ZERO_TO_ONE, sphereSlices );
	sphereProp3->StartUp();
	m_sphereProps.push_back( sphereProp3 );
	m_allProps.push_back( sphereProp3 );

	SphereProp* sphereProp4 = new SphereProp( this );
	sphereProp4->m_radius = 0.5f;
	sphereProp4->m_material = new Material( MaterialType::Minnaert, 1.f, 1.f, 1.f, 1.f );
	sphereProp4->m_position = Vec3( 10.f, -3.f, 2.f );
	sphereProp4->m_vertexes.reserve( 200 );
	AddVertsForSphere3D( sphereProp4->m_vertexes, Vec3( 0.f, 0.f, 0.f ), sphereProp4->m_radius, Rgba8::WHITE, AABB2::ZERO_TO_ONE, sphereSlices );
	sphereProp4->StartUp();
	m_sphereProps.push_back( sphereProp4 );
	m_allProps.push_back( sphereProp4 );

	SphereProp* sphereProp5 = new SphereProp( this );
	sphereProp5->m_radius = 0.5f;
	sphereProp5->m_material = new Material( MaterialType::Banded, 1.f, 1.f, 1.f, 1.f );
	sphereProp5->m_position = Vec3( 10.f, 3.f, 4.f );
	sphereProp5->m_vertexes.reserve( 200 );
	AddVertsForSphere3D( sphereProp5->m_vertexes, Vec3( 0.f, 0.f, 0.f ), sphereProp5->m_radius, Rgba8( 69, 177, 246, 255 ), AABB2::ZERO_TO_ONE, sphereSlices );
	sphereProp5->StartUp();
	m_sphereProps.push_back( sphereProp5 );
	m_allProps.push_back( sphereProp5 );

	SphereProp* sphereProp6 = new SphereProp( this );
	sphereProp6->m_radius = 0.5f;
	sphereProp6->m_material = new Material( MaterialType::FresnelBanded, 1.f, 1.f, 1.f, 16.f );
	sphereProp6->m_position = Vec3( 10.f, 1.f, 4.f );
	sphereProp6->m_vertexes.reserve( 200 );
	AddVertsForSphere3D( sphereProp6->m_vertexes, Vec3( 0.f, 0.f, 0.f ), sphereProp6->m_radius, Rgba8( 225, 156, 234, 255 ), AABB2::ZERO_TO_ONE, sphereSlices );
	sphereProp6->StartUp();
	m_sphereProps.push_back( sphereProp6 );
	m_allProps.push_back( sphereProp6 );

	SphereProp* sphereProp7 = new SphereProp( this );
	sphereProp7->m_radius = 0.5f;
	sphereProp7->m_material = new Material( MaterialType::BackLight, 1.f, 1.f, 1.f, 64.f );
	sphereProp7->m_position = Vec3( 10.f, -1.f, 4.f );
	sphereProp7->m_vertexes.reserve( 200 );
	AddVertsForSphere3D( sphereProp7->m_vertexes, Vec3( 0.f, 0.f, 0.f ), sphereProp7->m_radius, Rgba8( 16, 216, 18, 255 ), AABB2::ZERO_TO_ONE, sphereSlices );
	sphereProp7->StartUp();
	m_sphereProps.push_back( sphereProp7 );
	m_allProps.push_back( sphereProp7 );

	SphereProp* sphereProp8 = new SphereProp( this );
	sphereProp8->m_radius = 0.5f;
	sphereProp8->m_material = new Material( MaterialType::OrenNayar, 1.f, 1.f, 1.f, 64.f );
	sphereProp8->m_position = Vec3( 10.f, -3.f, 4.f );
	sphereProp8->m_vertexes.reserve( 200 );
	AddVertsForSphere3D( sphereProp8->m_vertexes, Vec3( 0.f, 0.f, 0.f ), sphereProp8->m_radius, Rgba8( 255, 215, 87, 255 ), AABB2::ZERO_TO_ONE, sphereSlices );
	sphereProp8->StartUp();
	m_sphereProps.push_back( sphereProp8 );
	m_allProps.push_back( sphereProp8 );

	SphereProp* spherePropForSpotLight = new SphereProp( this );
	spherePropForSpotLight->m_radius = 0.5f;
	spherePropForSpotLight->m_material = new Material( MaterialType::BlinnPhong, 1.f, 1.f, 1.f, 64.f );
	spherePropForSpotLight->m_position = Vec3( -3.f, -10.f, 2.f );
	spherePropForSpotLight->m_vertexes.reserve( 2000 );
	spherePropForSpotLight->m_texture = g_cubeTestTex;
	AddVertsForSphere3D( spherePropForSpotLight->m_vertexes, Vec3( 0.f, 0.f, 0.f ), spherePropForSpotLight->m_radius, Rgba8::WHITE, AABB2::ZERO_TO_ONE, sphereSlices );
	spherePropForSpotLight->StartUp();
	m_sphereProps.push_back( spherePropForSpotLight );
	m_allProps.push_back( spherePropForSpotLight );

	//PBR
	int rowNum = 7;
	int columnNum = 7;
	float cuurentMetallic = 0.f;
	float currentRoughness = 0.f;
	for (int i = 0; i < rowNum; i++)
	{
		cuurentMetallic = GetClamped((float)i / (float)rowNum, 0.05f, 1.f );
		for (int j = 0; j < columnNum; j++)
		{
			currentRoughness = (float)j / (float)columnNum;
			SphereProp* spherePropPBR = new SphereProp( this );
			spherePropPBR->m_isMaterialAdjustable = true;
			spherePropPBR->m_radius = 0.5f;
			spherePropPBR->m_material = new Material( MaterialType::PBR, 1.f, 1.f, 1.f, 64.f, cuurentMetallic, currentRoughness );
			spherePropPBR->m_position = Vec3( -15.f, -3.f, 2.f ) + Vec3( 0.f, (float)i * 1.2f, (float)j * 1.2f );
			spherePropPBR->m_vertexes.reserve( 2000 );
			AddVertsForSphere3D( spherePropPBR->m_vertexes, Vec3( 0.f, 0.f, 0.f ), spherePropPBR->m_radius, Rgba8::RED, AABB2::ZERO_TO_ONE, sphereSlices );
			spherePropPBR->StartUp();
			m_sphereProps.push_back( spherePropPBR );
			m_allProps.push_back( spherePropPBR );
		}
	}

	int texturedPBRSphereNum = 7;
	//PBR IBL
	for (int i = 0; i < texturedPBRSphereNum; i++)
	{
		SphereProp* PBRSphere = new SphereProp( this );
		PBRSphere->m_radius = 0.5f;
		PBRSphere->m_material = new Material( MaterialType::IBLPBR, 1.f, 1.f, 1.f, 64.f, m_adjustableMetallic, m_adjustableRoughness );
		PBRSphere->m_position = Vec3( -2.f, 4.f - i * 1.2f, 5.f );
		PBRSphere->m_normalMap = g_normalMap[i];
		PBRSphere->m_albedoMap = g_albedoMap[i];
		PBRSphere->m_aoMap = g_aoMap[i];
		PBRSphere->m_metallicMap = g_metallicMap[i];
		PBRSphere->m_roughnessMap = g_roughnessMap[i];
		PBRSphere->m_vertexes.reserve( 2000 );
		AddVertsForSphere3D( PBRSphere->m_vertexes, Vec3( 0.f, 0.f, 0.f ), PBRSphere->m_radius, Rgba8::WHITE, AABB2::ZERO_TO_ONE, sphereSlices );
		g_theRenderer->CalculateModelVectors( (int)PBRSphere->m_vertexes.size(), PBRSphere->m_vertexes.data() );
		PBRSphere->StartUp();
		m_sphereProps.push_back( PBRSphere );
		m_allProps.push_back( PBRSphere );
	}

	//Base
	int texIndex = 6;
	m_planeProp = new Prop( this );
	m_planeProp->m_material = new Material( MaterialType::IBLPBR, 1.f, 1.f, 1.f, 64.f, m_adjustableMetallic, m_adjustableRoughness );
	m_planeProp->m_position = Vec3( 0.f, 0.f, 0.f );
	m_planeProp->m_normalMap = g_normalMap[texIndex];
	m_planeProp->m_albedoMap = g_albedoMap[texIndex];
	m_planeProp->m_aoMap = g_aoMap[texIndex];
	m_planeProp->m_metallicMap = g_metallicMap[texIndex];
	m_planeProp->m_roughnessMap = g_roughnessMap[texIndex];
	AddVertsForQuad3D( m_planeProp->m_vertexes, Vec3( -16.f, -16.f, 0.f ), Vec3( 16.f, -16.f, 0.f ), Vec3( 16.f, 16.f, 0.f ), Vec3( -16.f, 16.f, 0.f ), Rgba8( 212, 191, 222, 255 ) );
	m_planeProp->StartUp();
	m_allProps.push_back( m_planeProp );

	//OBJ Model
	for (int i = 0; i < (int)ObjType::TypeNum; i++)
	{
		std::vector<Prop*> characterObj;
		for (int j = 0; j < g_objMeshes[i]->m_cpuMeshes.size(); j++)
		{
			Prop* objPart = new Prop( this );
			objPart->m_material = new Material( MaterialType::Banded, 1.f, 1.f, 0.f, 0.f );
			objPart->m_vertexes = g_objMeshes[i]->m_cpuMeshes[j].m_vertexes;
			objPart->m_texture = g_objMeshes[i]->m_cpuMeshes[j].m_texture;
			objPart->m_position = Vec3( 4.f + i * 2.f, 0.f, 5.f );
			objPart->m_orientation = EulerAngles( 0.f, 0.f, 90.f );
			objPart->m_scale = 0.1f;
			objPart->m_color = Rgba8::WHITE;
			objPart->StartUp();
			characterObj.push_back( objPart );
			m_allProps.push_back( objPart );
		}
		m_characterModels.push_back( characterObj );
	}
}

void Game::LightInitilization()
{
	//Directional Light
	m_lightDirection = Vec3( 1.f, 0.f, -1.f );
	m_sunIntensity = 0.6f;
	m_ambientIntensity = 0.1f;
	//Spot Light
	Vec3 spotLightPosition( -3.f, -10.f, 5.f );
	Vec3 spotLightDirection( 0.f, 0.f, -1.f );
	m_spotLight = new SpotLight( spotLightPosition, spotLightDirection, 30.f, 50.f, 0.2f, 0.6f, 1.f, 1.f, 0.07f, 0.0017f );
	//Point Lights
	PointLight pointLight1( Vec3( -10.f, 2.f, 8.f ), 0.05f, 0.6f, 1.f, 1.f, 0.09f, 0.032f,	Vec3( 20.f ) );
	PointLight pointLight2( Vec3( -10.f, -2.f, 8.f ), 0.05f, 0.6f, 1.f, 1.f, 0.09f, 0.032f, Vec3( 20.f ) );
	PointLight pointLight3( Vec3( -10.f, -2.f, 4.f ), 0.05f, 0.6f, 1.f, 1.f, 0.09f, 0.032f, Vec3( 20.f ) );
	PointLight pointLight4( Vec3( -10.f, 2.f, 4.f ), 0.05f, 0.6f, 1.f, 1.f, 0.09f, 0.032f,	Vec3( 20.f ) );
	m_pointLights.push_back( pointLight1 );
	m_pointLights.push_back( pointLight2 );
	m_pointLights.push_back( pointLight3 );
	m_pointLights.push_back( pointLight4 );
}


void Game::Update( float deltaSeconds )
{
	m_player->Update( deltaSeconds );
	LightUpdate( deltaSeconds );
	DebugRenderBeginFrame();
	FPSUpdate( deltaSeconds );
	//MaterialUpdate( deltaSeconds );
	KeyBoradInput();
	RayCastUpdate( deltaSeconds );
	ImguiUpdate( deltaSeconds );
	DebugRenderEndFrame();
}

void Game::Render() const
{
	g_theRenderer->ClearScreen( Rgba8( 50, 50, 50, 255 ) );
	g_theRenderer->BeginCamera( m_player->m_playerCamera );
	RenderHDRTexture();
	RenderDiffuseProps();
	RenderDefaultProps();
	RenderWorldArrow();
	g_theRenderer->EndCamera( m_player->m_playerCamera );

	g_theRenderer->BeginCamera( m_screenCamera );
	RenderDebugMessage();
	RenderScreenText();	

	g_theRenderer->EndCamera( m_screenCamera );

	DebugRenderWorld( m_player->m_playerCamera );
	DebugRenderScreen( m_screenCamera );
	RenderImgui();
}

void Game::KeyBoradInput()
{
	if (g_input->WasKeyJustPressed( 119 )) //F8
	{
		ShutDown();
		StartUp();
	}

	if (g_input->WasKeyJustPressed( 'P' ))
	{
		g_theApp->m_gameClock->TogglePause();
	}
	if (g_input->IsKeyDown( 'T' ))
	{
		g_theApp->m_gameClock->SetTimeScale( 0.1f );
	}
	if (g_input->WasKeyJustReleased( 'T' ))
	{
		g_theApp->m_gameClock->SetTimeScale( 1.f );
	}
	if (g_input->WasKeyJustPressed( 'O' ))
	{
		g_theApp->m_gameClock->StepSingleFrame();
	}
	if (g_input->IsKeyDown( KEYCODE_RIGHT_MOUSE ))
	{
		g_input->SetCursorMode( true, true );
	}
	if (g_input->WasKeyJustReleased( KEYCODE_RIGHT_MOUSE ))
	{
		g_input->SetCursorMode( false, false );
	}
}

void Game::ImguiUpdate( float deltaSeconds )
{
	(void)deltaSeconds;

	ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
	ImGuiIO& IO = ImGui::GetIO();
	IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGuiWindowFlags windowsFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos( viewport->WorkPos );
	ImGui::SetNextWindowSize( viewport->WorkSize );
	ImGui::SetNextWindowViewport( viewport->ID );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.f );
	windowsFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	windowsFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	windowsFlags |= ImGuiWindowFlags_NoBackground;
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.f, 0.f ) );

	//render imgui
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin( "DockSpace", nullptr, windowsFlags );
	ImGui::PopStyleVar();
	ImGui::PopStyleVar(2);

	if (IO.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockSpaceID = ImGui::GetID( "MyDockSpace" );
		ImGui::DockSpace( dockSpaceID, ImVec2( 0.f, 0.f ), dockSpaceFlags );
	}
	ImGui::End();

	//Create ImGui Window
	ImGui::SetNextWindowBgAlpha( 1.f );
	ImGui::PushStyleColor( ImGuiCol_WindowBg, IM_COL32( 20, 20, 20, 255 ) );
	ImGui::PushStyleColor( ImGuiCol_Text, IM_COL32( 255, 255, 255, 255 ) );
	if (ImGui::Begin( "Information" ))
	{
		if (m_raycastSphere)
		{
			if (m_raycastSphere->m_isMaterialAdjustable)
			{
				ImGui::DragFloat( "Roughness", &m_raycastSphere->m_material->m_materialRoughness, 0.05f, 0.05f, 1.f );
				ImGui::DragFloat( "Metallic", &m_raycastSphere->m_material->m_materialMetallic, 0.f, 0.f, 1.f );
			}
			float spherePosition[3] = { m_raycastSphere->m_position.x, m_raycastSphere->m_position.y, m_raycastSphere->m_position.z };
			ImGui::DragFloat3( "Location", spherePosition, 0.1f );
			m_raycastSphere->m_position = Vec3( spherePosition );

			float rotation[3] = { m_raycastSphere->m_orientation.m_yaw, m_raycastSphere->m_orientation.m_pitch, m_raycastSphere->m_orientation.m_roll };
			ImGui::DragFloat3( "Rotation", rotation, 1.f );
			m_raycastSphere->m_orientation = EulerAngles( rotation[0], rotation[1], rotation[2] );
		}
	}
	ImGui::End();

	//ImGui::PopStyleColor(2);

	ImGui::SetNextWindowBgAlpha( 1.f );
	if (ImGui::Begin( "Controls" ))
	{
		ImGui::Text( "WASD to move" );
		ImGui::Text( "Press LeftMouse to select Sphere, Hold RightMouse to rotate" );
		ImGui::Text( "M to select Show Cursor" );
		ImGui::End();
	}
	ImGui::SetNextWindowBgAlpha( 1.f );

	if (ImGui::Begin( "DebugMessage" ))
	{
		//ImGui::PushStyleColor( ImGuiCol_TitleBg, IM_COL32( 255, 171, 80, 255 ) );

		ImGui::Text( Stringf( "Time: %.2f", g_theApp->m_gameClock->GetTotalSeconds() ).c_str() );
		ImGui::Text( Stringf( "FPS: %.2i", m_currentFPS ).c_str() );
		ImGui::Text( Stringf( "TimeScale: %.2f", g_theApp->m_gameClock->GetTimeScale() ).c_str() );
		ImGui::Text( Stringf( "Camera Position %.2f, %.2f, %.2f ", m_player->GetPosition().x, m_player->GetPosition().y, m_player->GetPosition().z ).c_str() );
		ImGui::Text( Stringf( "Cursor Position %.2f, %.2f ", g_theWindow->GetNormalizedCursorPos().x, g_theWindow->GetNormalizedCursorPos().y ).c_str() );
		ImGui::End();
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
}

void Game::LightUpdate( float deltaSeconds )
{
	m_directionalLightDegrees += 20.f * deltaSeconds;
	m_lightDirection.x = CosDegrees( m_directionalLightDegrees );
	m_lightDirection.y = SinDegrees( m_directionalLightDegrees );
}

void Game::FPSUpdate( float deltaSeconds )
{
	(void)deltaSeconds;
	if (m_FPSTimer->DecrementPeriodIfElapsed())
	{
		m_currentFPS = ((int)g_theApp->m_gameClock->GetFrameCount() - m_lastFrameFPS) * 2;
		m_lastFrameFPS = (int)g_theApp->m_gameClock->GetFrameCount();
	}
}

void Game::RayCastUpdate( float deltaSeconds )
{
	(void)deltaSeconds;
	if (g_input->WasKeyJustPressed( KEYCODE_LEFT_MOUSE ))
	{
		std::vector<RaycastResult3D> allRayCastResult;
		RaycastResult3D nearestRayCast;
		nearestRayCast.m_rayStartPosition = m_player->GetPosition();
		nearestRayCast.m_rayDirection = GetRayDirFromCursorPosToWorldPos();
		nearestRayCast.m_rayLength = 10.f;
		float nearestImpactDistance = 999999999.f;
		int nearestIndex = -1;
		for (int i = 0; i < m_sphereProps.size(); i++)
		{
			RaycastResult3D sphereResult;
			sphereResult.m_rayStartPosition = nearestRayCast.m_rayStartPosition;
			sphereResult.m_rayDirection = nearestRayCast.m_rayDirection;
			sphereResult.m_rayLength = nearestRayCast.m_rayLength;
			isRaycastVSSphere3D( sphereResult, m_sphereProps[i]->m_position, m_sphereProps[i]->m_radius );
			allRayCastResult.push_back( sphereResult );
		}
		for (int i = 0; i < allRayCastResult.size(); i++)
		{
			if (allRayCastResult[i].m_didImpact)
			{
				if (nearestImpactDistance > allRayCastResult[i].m_impactDistance)
				{
					nearestImpactDistance = allRayCastResult[i].m_impactDistance;
					nearestIndex = i;
				}
			}
		}
		if (nearestIndex != -1)
		{
			nearestRayCast = allRayCastResult[nearestIndex];

			if (m_previoudHitSphereIndex != -1)
			{
				m_sphereProps[m_previoudHitSphereIndex]->m_isOutline = false;
			}
			m_sphereProps[nearestIndex]->m_isOutline = true;
			m_raycastSphere = m_sphereProps[nearestIndex];
			m_previoudHitSphereIndex = nearestIndex;

		}
	}
}

void Game::RenderDefaultProps() const
{
	g_theRenderer->BindShader( g_theRenderer->m_defaultShader );
	std::vector<Vertex_PCU> verts;
	for (int i = 0; i < m_pointLights.size(); i++)
	{
		AABB3 pointLightAABB( m_pointLights[i].m_lightPosition - Vec3(0.1f, 0.1f, 0.1f), m_pointLights[i].m_lightPosition + Vec3(0.1f, 0.1f, 0.1f));
		AddVertsForAABB3D( verts, pointLightAABB, Rgba8::WHITE );
	}
	AddVertsForCylinderZ3D( verts, Vec2( m_spotLight->m_position.x, m_spotLight->m_position.y ), FloatRange( m_spotLight->m_position.z - 0.1f, m_spotLight->m_position.z + 0.1f ),0.1f );
	g_theRenderer->BindTexture( nullptr );
	g_theRenderer->SetModelConstants();
	g_theRenderer->DrawVertexArray( (int)verts.size(), verts.data() );
}

void Game::RenderDiffuseProps() const
{
	g_theRenderer->BindShader( g_theApp->m_diffuseShader );
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetLightConstants( m_lightDirection, m_sunIntensity, m_ambientIntensity );
	g_theRenderer->SetPointLightConstants( (int)m_pointLights.size(), m_pointLights.data() );
	g_theRenderer->SetSpotLightConstants( m_spotLight->m_position, m_spotLight->m_direction, m_spotLight->m_cutOff, m_spotLight->m_outerCutOff, m_spotLight->m_ambientIntensity, m_spotLight->m_diffuseIntensity,
		m_spotLight->m_specularIntensity, m_spotLight->m_constantValue, m_spotLight->m_linearValue, m_spotLight->m_quadraticValue );
	for (int i = 0; i < (int)m_sphereProps.size(); i++)
	{
		m_sphereProps[i]->Render();
	}
	for (int i = 0; i < (int)m_characterModels.size(); i++)
	{
		for (int j = 0; j < (int)m_characterModels[i].size(); j++)
		{
			m_characterModels[i][j]->Render();
		}
	}
	m_planeProp->Render();
}

void Game::RenderHDRTexture()const
{	
	std::vector<Vertex_PCU> verts;
	verts.reserve( 36 );
	AddVertsForSkyBox( verts, AABB3( Vec3( -50.f ), Vec3( 50.f ) ), Rgba8::WHITE );
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture( g_skyboxTexture, 0 );
	g_theRenderer->DrawVertexArray( (int)verts.size(), verts.data() );
}

void Game::RenderDebugMessage() const
{
	//DebugAddScreenText( "Time:" + Stringf( "%.2f", g_theApp->m_gameClock->GetTotalSeconds() ), Vec2( 12.5f, 7.7f ), 0.1f, Vec2( 0.5f, 0.5f ), 0.f, Rgba8::WHITE );
	//if (!g_theApp->m_gameClock->IsPaused())
	//{
	//	DebugAddScreenText( "FPS:" + std::to_string( m_currentFPS ), Vec2( 13.6f, 7.7f ), 0.1f, Vec2( 0.5f, 0.5f ), 0.f, Rgba8::WHITE );
	//}
	//else
	//{
	//	DebugAddScreenText( "FPS: inf", Vec2( 13.6f, 7.7f ), 0.1f, Vec2( 0.5f, 0.5f ), 0.f, Rgba8::WHITE );
	//}
	//DebugAddScreenText( "TimeScale:" + Stringf( "%.2f", g_theApp->m_gameClock->GetTimeScale() ), Vec2( 14.5f, 7.7f ), 0.1f, Vec2( 0.5f, 0.5f ), 0.f, Rgba8::WHITE );
	//
	//Vec3 worldPos = GetRayDirFromCursorPosToWorldPos();

	//DebugAddMessage( "Camera Position:" + Stringf( "%.2f", m_player->GetPosition().x ) + "," +
	//	Stringf( "%.2f", m_player->GetPosition().y ) + "," + Stringf( "%.2f", m_player->GetPosition().z ), 0.f );
	//DebugAddMessage( "Cursor Position:" + Stringf( "%.2f", g_theWindow->GetNormalizedCursorPos().x ) + "," +
	//	Stringf( "%.2f", g_theWindow->GetNormalizedCursorPos().y ), 0.f );
	//DebugAddMessage( "Cursor Position:" + Stringf( "%.2f", worldPos.x ) + "," +
	//	Stringf( "%.2f", worldPos.y ) + Stringf( ",%.2f", worldPos.z ), 0.f );
}

Mat44 Game::GetModelMatrix( Vec3 position, EulerAngles orientation, float scale ) const
{
	Mat44 mat;
	mat = orientation.GetAsMatrix_IFwd_JLeft_KUp();
	mat.SetTranslation3D( position );
	mat.AppendScaleUniform3D( scale );
	return mat;
}

void Game::RenderImgui() const
{
	//Assemble Together Draw Data
	ImGui::Render();
	//Render Draw Data
	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
}

void Game::RenderWorldArrow() const
{
	std::vector<Vertex_PCU> arrowVerts;
	Mat44 arrowTransformMat;
	arrowTransformMat.SetTranslation3D( m_player->GetPosition() );
	arrowVerts.reserve( 300 );
	Vec3 fowardVector = GetModelMatrix( m_player->GetPosition(), m_player->GetOrientation() ).GetIBasis3D().GetNormalized();
	AddVertsForArrow3D( arrowVerts, fowardVector * 0.2f, fowardVector * 0.2f + Vec3( 0.01f, 0.f, 0.f ), 0.0005f, Rgba8::RED );
	AddVertsForArrow3D( arrowVerts, fowardVector * 0.2f, fowardVector * 0.2f + Vec3( 0.f, 0.01f, 0.f ), 0.0005f, Rgba8::GREEN );
	AddVertsForArrow3D( arrowVerts, fowardVector * 0.2f, fowardVector * 0.2f + Vec3( 0.f, 0.f, 0.01f ), 0.0005f, Rgba8::BLUE );
	g_theRenderer->BindTexture( nullptr );
	g_theRenderer->SetModelConstants( arrowTransformMat );
	g_theRenderer->DrawVertexArray( (int)arrowVerts.size(), arrowVerts.data() );
}

void Game::RenderScreenText() const
{
	//g_theRenderer->BeginCamera( m_screenCamera );
	std::vector<Vertex_PCU> textVerts;
	g_bitMapFont->AddVertsForText2D( textVerts, Vec2( 4.f, 7.5f ), 0.2f, "Press M to show/hide cursor", Rgba8( 255, 255, 0, 255 ) );
	g_theRenderer->BindTexture( &g_bitMapFont->GetTexture() );
	g_theRenderer->SetModelConstants();
	g_theRenderer->DrawVertexArray( (int)textVerts.size(), textVerts.data() );
	//g_theRenderer->EndCamera( m_screenCamera );
}

void Game::SetTextureConstants( Vec4 texDimension )
{
	g_theRenderer->CopyCPUToGPU( &texDimension, m_skyboxTextureCBO->m_size, m_skyboxTextureCBO );
	g_theRenderer->BindConstantBuffer( k_modelConstantSlot, m_skyboxTextureCBO );
}

Vec3 Game::GetRayDirFromCursorPosToWorldPos() const
{
	POINT cursorPos;
	GetCursorPos( &cursorPos );
	ScreenToClient( HWND( g_theWindow->GetHwnd() ), &cursorPos );
	float screenX = (2.0f * cursorPos.x) / g_theWindow->GetClientDimensions().x - 1.0f;
	float screenY = 1.0f - (2.0f * cursorPos.y) / g_theWindow->GetClientDimensions().y;
	float screenZ = 1.0f;
	DirectX::XMVECTOR rayNDC = DirectX::XMVectorSet( screenX, screenY, screenZ, 1.0f );

	//Vec2 cursorPos = g_theWindow->GetNormalizedCursorPos();
	//DirectX::XMVECTOR rayNDC = DirectX::XMVectorSet( cursorPos.x, cursorPos.y, 1.f, 1.f );
	Mat44 viewMat = m_player->m_playerCamera.GetViewMatrix();
	Mat44 projectionMat = m_player->m_playerCamera.GetProjectionMatrix();
	DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixSet(
		viewMat.GetIBasis4D().x, viewMat.GetIBasis4D().y, viewMat.GetIBasis4D().z, viewMat.GetIBasis4D().w,
		viewMat.GetJBasis4D().x, viewMat.GetJBasis4D().y, viewMat.GetJBasis4D().z, viewMat.GetJBasis4D().w,
		viewMat.GetKBasis4D().x, viewMat.GetKBasis4D().y, viewMat.GetKBasis4D().z, viewMat.GetKBasis4D().w,
		viewMat.GetTranslation4D().x, viewMat.GetTranslation4D().y, viewMat.GetTranslation4D().z, viewMat.GetTranslation4D().w
	);
	DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixSet(
		projectionMat.GetIBasis4D().x, projectionMat.GetIBasis4D().y, projectionMat.GetIBasis4D().z, projectionMat.GetIBasis4D().w,
		projectionMat.GetJBasis4D().x, projectionMat.GetJBasis4D().y, projectionMat.GetJBasis4D().z, projectionMat.GetJBasis4D().w,
		projectionMat.GetKBasis4D().x, projectionMat.GetKBasis4D().y, projectionMat.GetKBasis4D().z, projectionMat.GetKBasis4D().w,
		projectionMat.GetTranslation4D().x, projectionMat.GetTranslation4D().y, projectionMat.GetTranslation4D().z, projectionMat.GetTranslation4D().w
	);

	DirectX::XMMATRIX viewProjMatrix = DirectX::XMMatrixMultiply( viewMatrix, projectionMatrix );
	DirectX::XMMATRIX invViewProjMatrix = DirectX::XMMatrixInverse( nullptr, viewProjMatrix );
	DirectX::XMVECTOR rayWorld = DirectX::XMVector3TransformCoord( rayNDC, invViewProjMatrix );
	Vec3 worldPos;
	worldPos.x = DirectX::XMVectorGetX( rayWorld );
	worldPos.y = DirectX::XMVectorGetY( rayWorld );
	worldPos.z = DirectX::XMVectorGetZ( rayWorld );
	
	return (worldPos - m_player->m_playerCamera.GetPosition()).GetNormalized();
}