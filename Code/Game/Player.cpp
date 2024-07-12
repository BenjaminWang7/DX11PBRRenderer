#include "Game/Player.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/Game.hpp"

extern InputSystem* g_input;

Player::Player( Game* owner, Vec3 const& startPos )
{
	m_game = owner;
	m_position = startPos;
}

Player::~Player()
{
}

void Player::StartUp()
{
	m_playerCamera.SetPerspectiveView( 2.f, 60.f, 0.1f, 100.f );
	m_playerCamera.SetRenderBasis( Vec3( 0.f, 0.f, 1.f ), Vec3( -1.f, 0.f, 0.f ), Vec3( 0.f, 1.f, 0.f ) );
	m_playerCamera.SetTransform( m_position, m_orientation );
	//m_fowardVector = GetModelMatrix( m_fowardVector, m_orientation ).GetIBasis3D();
}

void Player::Update( float deltaSeconds )
{
	m_position +=  m_velocity * deltaSeconds;
	UpdateInput( deltaSeconds );
	m_playerCamera.SetTransform( m_position, m_orientation );
}

void Player::UpdateInput( float deltaSeconds )
{
	(void)deltaSeconds;
	m_velocity = Vec3( 0.f, 0.f, 0.f );
	float speed = 1.f;
	if (g_input->IsKeyDown( KEYCODE_SHIFT ) || g_input->GetController( 0 ).IsButtonDown( XboxButtonID::XboxKeyA ))
	{
		speed = 10.f;
	}

	//Xbox Controller
	m_velocity += m_fowardVector * speed * g_input->GetController( 0 ).GetLeftStick().GetPosition().y;
	m_velocity -= m_rightVector * speed * g_input->GetController( 0 ).GetLeftStick().GetPosition().x;
	m_orientation.m_yaw -= g_input->GetController( 0 ).GetRightStick().GetPosition().x ;
	m_orientation.m_pitch -= g_input->GetController( 0 ).GetRightStick().GetPosition().y;

	Vec3 xyForward = Vec3( m_fowardVector.x, m_fowardVector.y, 0.f ).GetNormalized();
	Vec3 xyRight = Vec3( m_rightVector.x, m_rightVector.y, 0.f ).GetNormalized();

	if (g_input->IsKeyDown( 'W' ))
	{
		m_velocity = xyForward * speed;
	}

	if (g_input->IsKeyDown( 'S' ))
	{
		m_velocity = xyForward * -speed;
	}
	if (g_input->IsKeyDown( 'A' ))
	{
		m_velocity = xyRight * speed;
	}
	if (g_input->IsKeyDown( 'D' ))
	{
		m_velocity = xyRight * -speed;
	}
	if (g_input->IsKeyDown( 'E' ) || g_input->GetController( 0 ).IsButtonDown( XboxButtonID::XboxKeyLS ))
	{
		m_velocity = Vec3( 0.f, 0.f, 1.f ) * speed;
	}
	if (g_input->IsKeyDown( 'Q' ) || g_input->GetController( 0 ).IsButtonDown( XboxButtonID::XboxKeyRS ))
	{
		m_velocity = Vec3( 0.f, 0.f, 1.f ) * -speed;
	}

	//if (g_input->IsKeyDown( KEYCODE_RIGHT_MOUSE ))
	{
		m_orientation.m_yaw -= g_input->GetCursorClientDelta().x * 180.f;
		m_orientation.m_pitch -= g_input->GetCursorClientDelta().y * 180.f;
		m_orientation.m_pitch = GetClamped( m_orientation.m_pitch, -85.f, 85.f );
	}


	m_fowardVector = GetModelMatrix( m_position, m_orientation ).GetIBasis3D().GetNormalized();
	m_rightVector = GetModelMatrix( m_position, m_orientation ).GetJBasis3D().GetNormalized();
	m_upVector = GetModelMatrix( m_position, m_orientation ).GetKBasis3D().GetNormalized();

}

Vec3 Player::GetPosition()
{
	return m_position;
}

EulerAngles Player::GetOrientation()
{
	return m_orientation;
}

void Player::Render() const
{
}

