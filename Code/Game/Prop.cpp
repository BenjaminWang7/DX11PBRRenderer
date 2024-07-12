#include "Game/Prop.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/Game.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Renderer/HDRCubeMap.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Game/App.hpp"
extern Renderer* g_theRenderer;
extern Texture* g_BRDFTexture;
extern App* g_theApp;
Prop::Prop()
{
}

Prop::Prop( Game* owner )
{
	m_game = owner;
}

Prop::~Prop()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;
}

void Prop::Update( float deltaSeconds )
{ 
	(void) deltaSeconds;
}

void Prop::Render() const
{
	if (g_theRenderer != nullptr)
	{
		g_theRenderer->BindShader( g_theApp->m_diffuseShader );
		g_theRenderer->SetRasterizerMode( RasterizerMode::SOLID_CULL_BACK );

		if (m_material != nullptr)
		{
			g_theRenderer->SetMaterialConstants( m_material->m_materialType, m_material->m_materialAmbient, m_material->m_materialDiffuse, m_material->m_materialSpecular, m_material->m_materialShininess, m_material->m_materialRoughness, m_material->m_materialMetallic );
		}
		g_theRenderer->SetModelConstants( GetModelMatrix( m_position, m_orientation, m_scale ), m_color );
		g_theRenderer->BindTexture( m_texture );
		g_theRenderer->BindTexture( m_normalMap, 1 );
		g_theRenderer->BindTexture( m_albedoMap, 2 );
		g_theRenderer->BindTexture( m_aoMap, 3 );
		g_theRenderer->BindTexture( m_metallicMap, 4 );
		g_theRenderer->BindTexture( m_roughnessMap, 5 );
		g_theRenderer->BindTexture( g_BRDFTexture, 6 );
		m_game->m_hdrCubemap->BindHDRCubeMap();
		g_theRenderer->DrawVertexBuffer( m_vertexBuffer, (int)m_vertexes.size(), 0 , VertexType::PCUTBN );
		
		//Draw outline
		if (m_isOutline)
		{
			g_theRenderer->SetRasterizerMode( RasterizerMode::SOLID_CULL_FRONT );
			g_theRenderer->BindShader( m_game->m_outlineShader );
			g_theRenderer->BindTexture( nullptr );
			g_theRenderer->SetModelConstants( GetModelMatrix( m_position, m_orientation, m_scale * 1.02f ), Rgba8( 255, 138, 48, 255 ) );
			g_theRenderer->DrawVertexBuffer( m_vertexBuffer, (int)m_vertexes.size(), 0, VertexType::PCUTBN );		
		}
	}

}

void Prop::StartUp()
{
	m_vertexBuffer = g_theRenderer->CreateVertexBuffer( 1 );
	g_theRenderer->CopyCPUToGPU( m_vertexes.data(), (int)m_vertexes.size() * sizeof( Vertex_PCUTBN ), m_vertexBuffer );
}

void Prop::AddVertsTogetherForObjMesh( std::vector<CPUMesh> cpuMeshes )
{
	for (int i = 0; i < cpuMeshes.size(); i++)
	{
		for (int j = 0; j < cpuMeshes[i].m_vertexes.size(); j++)
		{
			m_vertexes.push_back( cpuMeshes[i].m_vertexes[j] );
		}
	}
}

SphereProp::SphereProp( Game* owner )
{
	m_game = owner;
}

SphereProp::~SphereProp()
{
}