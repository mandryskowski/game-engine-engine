#include "Transform.h"
Transform::Transform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, glm::vec3 front, bool constrain)
{
	ParentTransform = nullptr;

	Position = pos;
	Rotation = rot;
	Scale = scale;
	Front = front;
	bConstrain = constrain;
}

glm::mat3 Transform::GetRotationMatrix(float scalar) const
{
	return GetRotationMatrixWithOffset(glm::vec3(0.0f), scalar);
}

glm::mat3 Transform::GetRotationMatrixWithOffset(glm::vec3 offset, float scalar) const
{
	glm::vec3 rot = (Rotation + offset) * scalar;

	if (!bConstrain)
		return (glm::mat3)glm::eulerAngleXYZ(glm::radians(rot.x), glm::radians(rot.y), glm::radians(rot.z));

	return (glm::mat3)glm::eulerAngleYXZ(glm::radians(rot.y), glm::radians(rot.x), glm::radians(rot.z));
}

glm::mat4 Transform::GetMatrix() const
{
	glm::mat4 mat = glm::translate(glm::mat4(1.0f), Position);

	if (bConstrain)
		mat *= glm::eulerAngleYXZ(glm::radians(Rotation.y), glm::radians(Rotation.x), glm::radians(Rotation.z));
	else
		mat *= glm::eulerAngleXYZ(glm::radians(Rotation.x), glm::radians(Rotation.y), glm::radians(Rotation.z));

	mat = glm::scale(mat, Scale);

	return mat;
}

glm::mat4 Transform::GetViewMatrix() const
{
	return glm::lookAt(Position, Position + Front, glm::vec3(0.0f, 1.0f, 0.0f));
}

Transform Transform::GetWorldTransform() const
{
	if (ParentTransform == nullptr)
		return Transform(Position, Rotation, Scale, Front, bConstrain);

	Transform ParentWorldTransform = ParentTransform->GetWorldTransform();
	glm::mat3 parentRotMat = ParentWorldTransform.GetRotationMatrix();
	glm::mat3 rotMat = parentRotMat * GetRotationMatrix();

	glm::vec3 pos = ParentWorldTransform.Position + glm::vec3(parentRotMat * Position);
	glm::vec3 rot = ParentWorldTransform.Rotation + Rotation;
	glm::vec3 scale = ParentWorldTransform.Scale * Scale;

	glm::vec3 front = rotMat * Front;


	return Transform(pos, rot, scale, front, bConstrain);
}

Transform* Transform::GetParentTransform() const
{
	return ParentTransform;
}

void Transform::SetParentTransform(Transform* parent, bool relocate)
{
	if (relocate && parent)
	{
		Transform worldTransform = GetWorldTransform();
		Transform parentWorld = parent->GetWorldTransform();
		Position = worldTransform.Position - parentWorld.Position;
		Position = glm::vec3(parentWorld.GetRotationMatrix(-1.0f) * Position);
		Rotation = worldTransform.Rotation = parentWorld.Rotation;
	}
	ParentTransform = parent;
}

glm::mat3 ModelToNormal(glm::mat4 model)
{
	return glm::mat3(glm::transpose(glm::inverse(model)));
}

glm::vec3& Transform::operator [](unsigned int i)
{
	switch (i)
	{
	case 0:
		return Position;
	case 1:
		return Rotation;
	case 2:
		return Scale;
	case 3:
		return Front;
	}
	return Position;
}