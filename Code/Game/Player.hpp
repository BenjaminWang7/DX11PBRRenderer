#pragma once
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Entity.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include <vector>

class Game;
class Texture;
class Camera;

class Player : Entity
{
public:
	Player(Game* owner, Vec3 const& startPos);
	virtual ~Player();

	void Update( float deltaSeconds );
	void Render() const;
	void StartUp();
	void UpdateInput( float deltaSeconds );
	Vec3 GetPosition();
	EulerAngles GetOrientation();
	Camera m_playerCamera;
	Vec3 m_fowardVector;
	Vec3 m_rightVector;
	Vec3 m_upVector;
};