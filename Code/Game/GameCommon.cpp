#include"GameCommon.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"

extern Renderer* g_theRenderer;

void DrawRing( Vec2 const& centerPos, float radius, float thickness, Rgba8 color )
{
	float halfThickness = 0.5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;
	constexpr int NUM_SIDES = 60;
	constexpr int NUM_TRIS = 2 * NUM_SIDES;
	constexpr int NUM_VERTS = 3 * NUM_TRIS;
	constexpr float DEGREEES_PER_SIDE = 360.f / static_cast<float> (NUM_SIDES);
	Vertex_PCU verts[NUM_VERTS];
	for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
	{
		float startDegrees = DEGREEES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees = DEGREEES_PER_SIDE * static_cast<float>(sideNum + 1);
		float cosStart = CosDegrees( startDegrees );
		float sinStart = SinDegrees( startDegrees );
		float cosEnd = CosDegrees( endDegrees );
		float sinEnd = SinDegrees( endDegrees );
		Vec3 innerStartPos = Vec3( centerPos.x + innerRadius * cosStart, centerPos.y + innerRadius * sinStart, 0.f );
		Vec3 outerStartPos = Vec3( centerPos.x + outerRadius * cosStart, centerPos.y + outerRadius * sinStart, 0.f );
		Vec3 outerEndPos = Vec3( centerPos.x + outerRadius * cosEnd, centerPos.y + outerRadius * sinEnd, 0.f );
		Vec3 innerEndPos = Vec3( centerPos.x + innerRadius * cosEnd, centerPos.y + innerRadius * sinEnd, 0.f );

		int vertIndexA = (6 * sideNum) + 0;
		int vertIndexB = (6 * sideNum) + 1;
		int vertIndexC = (6 * sideNum) + 2;
		int vertIndexD = (6 * sideNum) + 3;
		int vertIndexE = (6 * sideNum) + 4;
		int vertIndexF = (6 * sideNum) + 5;
		//Tri 1
		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = innerStartPos;
		verts[vertIndexC].m_position = outerStartPos;
		verts[vertIndexA].m_color = color;
		verts[vertIndexB].m_color = color;
		verts[vertIndexC].m_color = color;
		//Tri 2
		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;
		verts[vertIndexD].m_color = color;
		verts[vertIndexE].m_color = color;
		verts[vertIndexF].m_color = color;

	}	
	g_theRenderer->BindTexture( nullptr );
	g_theRenderer->BindShader( nullptr );
	g_theRenderer->SetStatesIfChanged();
	g_theRenderer->SetModelConstants();
	g_theRenderer->DrawVertexArray( NUM_VERTS, verts );

}






