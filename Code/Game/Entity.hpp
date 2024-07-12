#pragma once
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/EulerAngles.hpp"
class Game; 

class Entity
{
public:
	Entity();
	Entity(Game* owner);
	virtual ~Entity();

	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;
	virtual void StartUp();
	Mat44 GetModelMatrix( Vec3 position, EulerAngles orientation, float scale = 1.f ) const;
public:
	Game*			m_game = nullptr;
	Vec3			m_position;
	Vec3			m_velocity;
	EulerAngles		m_orientation;
	EulerAngles		m_angularVelocity;
	float			m_scale = 1.f;
};