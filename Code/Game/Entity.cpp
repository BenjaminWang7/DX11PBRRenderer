#include"Entity.hpp"
#include "Game/GameCommon.hpp"

Entity::Entity()
{
}

Entity::Entity(Game* owner)
{
	UNUSED( owner );
}

Entity::~Entity()
{
}

void Entity::StartUp()
{
}

Mat44 Entity::GetModelMatrix( Vec3 position, EulerAngles orientation, float scale ) const
{
	Mat44 mat;
	mat = orientation.GetAsMatrix_IFwd_JLeft_KUp();
	mat.SetTranslation3D( position );
	mat.AppendScaleUniform3D( scale );
	return mat;
}


void Entity::Update(float deltaSeconds)
{
	UNUSED( deltaSeconds );
}

void Entity::Render() const
{
}







