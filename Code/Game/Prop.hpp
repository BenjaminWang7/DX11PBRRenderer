#pragma once
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Entity.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/OBJMesh.hpp"
#include <vector>

class Game;
class Texture;
struct Mat44;

class Prop : public Entity
{
public:
	Prop();
	Prop(Game* owner);
	virtual ~Prop();

	void Update( float deltaSeconds );
	void Render() const;
	void StartUp();
	void AddVertsTogetherForObjMesh( std::vector<CPUMesh> cpuMeshes );
public:
	std::vector<Vertex_PCUTBN>		m_vertexes;
	Rgba8							m_color = Rgba8::WHITE;
	Texture*						m_texture = nullptr;
	Texture*						m_normalMap = nullptr;
	Texture*						m_albedoMap = nullptr;
	Texture*						m_aoMap = nullptr;
	Texture*						m_metallicMap = nullptr;
	Texture*						m_roughnessMap = nullptr;

	Material*						m_material = nullptr;
	bool							m_isOutline = false;
	VertexBuffer*					m_vertexBuffer = nullptr;
	bool							m_isMaterialAdjustable = false;
};


class SphereProp : public Prop
{
public:
	SphereProp( Game* owner );
	virtual ~SphereProp();
public:
	float m_radius = 0.f;
};