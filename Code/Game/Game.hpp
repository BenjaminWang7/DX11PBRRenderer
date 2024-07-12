#pragma once
#include "Entity.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/PointLight.hpp"

class Camera;
class Player;
class Prop;
class SpotLight;
class HDRCubeMap;
class SkyBox;
class SphereProp;



class Game
{
public:
	void StartUp();
	void Update( float deltaSeconds );
	void Render() const;
	void ShutDown();

	void PropsInitilization();
	void LightInitilization();

	void KeyBoradInput();
	void ImguiUpdate( float deltaSeconds );
	void LightUpdate( float deltaSeconds );
	void FPSUpdate( float deltaSeconds );
	void RayCastUpdate( float deltaSeconds );

	void RenderDefaultProps() const;
	void RenderDiffuseProps() const;
	void RenderDebugMessage() const;
	void RenderHDRTexture()const;
	void RenderWorldArrow()const;
	void RenderScreenText()const;
	void RenderImgui() const;
	Mat44 GetModelMatrix( Vec3 position, EulerAngles orientation, float scale = 1.f ) const;
	void SetTextureConstants( Vec4 texDimension );
	Vec3 GetRayDirFromCursorPosToWorldPos() const;
public:
	bool								m_isGameComplete = false;
	Camera								m_screenCamera;

	Player*								m_player = nullptr;
	Prop*								m_planeProp = nullptr;
	std::vector<std::vector<Prop*>>		m_characterModels;
	std::vector<SphereProp*>			m_sphereProps;
	std::vector<PointLight>				m_pointLights;
	SpotLight*							m_spotLight;
	Vec3								m_lightDirection;
	float								m_sunIntensity;
	float								m_ambientIntensity;
	HDRCubeMap*							m_hdrCubemap = nullptr;
	Shader*								m_outlineShader = nullptr;
protected:
	Timer*								m_FPSTimer;
	int									m_currentFPS = 0;
	int									m_lastFrameFPS = 0;
	float								m_directionalLightDegrees = 0.f;
	float								m_adjustableRoughness = 0.2f;
	float								m_adjustableMetallic = 0.6f;
	Shader*								m_HDRShader = nullptr;

	ConstantBuffer*						m_skyboxTextureCBO = nullptr;
	SkyBox*								m_skyBox = nullptr;
	Shader*								m_skyShader = nullptr;
	Shader*								m_postProcessShader = nullptr;
	Prop*								m_raycastSphere = nullptr;
	int									m_previoudHitSphereIndex = -1;
	std::vector<Prop*>					m_allProps;
};