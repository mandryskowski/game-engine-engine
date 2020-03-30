#include "Transform.h"

float Transform::tSum = 0.0f;
float beginning;

void Transform::FlagChildrenAsDirty()
{
	for (unsigned int i = 0; i < DirtyFlags.size(); i++)
		DirtyFlags[i] = true;

	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->FlagChildrenAsDirty();
}

Transform::Transform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, glm::vec3 front, bool constrain):
	ParentTransform(nullptr),
	WorldTransformCache(nullptr),
	WorldTransformMatrixCache(glm::mat4(1.0f)),
	Position(pos),
	Rotation(rot),
	Scale(scale),
	Front(front),
	DirtyFlags(2, true),
	bConstrain(constrain),
	PositionRef(Position),
	RotationRef(Rotation),
	ScaleRef(Scale),
	FrontRef(Front)
{
}

Transform::Transform(const Transform& t):
	ParentTransform(nullptr),
	WorldTransformCache(nullptr),
	WorldTransformMatrixCache(glm::mat4(1.0f)),
	Position(t.Position),
	Rotation(t.Rotation),
	Scale(t.Scale),
	Front(t.Front),
	DirtyFlags(2, true),
	bConstrain(t.bConstrain),
	PositionRef(Position),
	RotationRef(Rotation),
	ScaleRef(Scale),
	FrontRef(Front)
{
	//ParentTransform = t.ParentTransform;
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
	float time1, time2;
	if (tSum == 0.0f)
		beginning = glfwGetTime();
	time1 = glfwGetTime() - beginning;
	glm::mat4 mat = glm::translate(glm::mat4(1.0f), Position);

	
	if (bConstrain)
		mat *= glm::eulerAngleYXZ(glm::radians(Rotation.y), glm::radians(Rotation.x), glm::radians(Rotation.z));
	else
		mat *= glm::eulerAngleXYZ(glm::radians(Rotation.x), glm::radians(Rotation.y), glm::radians(Rotation.z));

	mat = glm::scale(mat, Scale);

	time2 = glfwGetTime() - beginning;
	tSum += time2 - time1;

	return mat;
}

glm::mat4 Transform::GetViewMatrix() const
{
	return glm::lookAt(Position, Position + Front, glm::vec3(0.0f, 1.0f, 0.0f));
}

const Transform& Transform::GetWorldTransform()
{
	if (!DirtyFlags[0])
		return *WorldTransformCache;

	if (ParentTransform == nullptr)
		return Transform(Position, Rotation, Scale, Front, bConstrain);

	Transform ParentWorldTransform = ParentTransform->GetWorldTransform();
	glm::mat3 parentRotMat = ParentWorldTransform.GetRotationMatrix();
	glm::mat3 rotMat = parentRotMat * GetRotationMatrix();

	glm::vec3 pos = ParentWorldTransform.Position + glm::vec3(parentRotMat * Position);
	glm::vec3 rot = ParentWorldTransform.Rotation + Rotation;
	glm::vec3 scale = ParentWorldTransform.Scale * Scale;
	glm::vec3 front = (Front == glm::vec3(0.0f)) ? (Front) : (rotMat * Front);	//try to save on calculations.

	if (WorldTransformCache)
		delete WorldTransformCache;
	WorldTransformCache = new Transform(pos, rot, scale, front, bConstrain);
	DirtyFlags[0] = false;

	return Transform(pos, rot, scale, front, bConstrain);
}

const glm::mat4& Transform::GetWorldTransformMatrix()
{
	if (!DirtyFlags[1])
		return WorldTransformMatrixCache;

	Transform worldTransform = GetWorldTransform();
	WorldTransformMatrixCache = worldTransform.GetMatrix();
	DirtyFlags[1] = false;

	return WorldTransformMatrixCache;
}

Transform* Transform::GetParentTransform() const
{
	return ParentTransform;
}

void Transform::SetPosition(glm::vec3 pos)
{
	Position = pos;
	FlagChildrenAsDirty();
}

void Transform::Move(glm::vec3 offset)
{
	SetPosition(Position + offset);
}

void Transform::SetRotation(glm::vec3 rot)
{
	Rotation = rot;
	FlagChildrenAsDirty();
}

void Transform::Rotate(glm::vec3 offset)
{
	SetRotation(Rotation + offset);
}

void Transform::SetScale(glm::vec3 scale)
{
	Scale = scale;
	FlagChildrenAsDirty();
}

void Transform::SetFront(glm::vec3 front)
{
	Front = front;
	FlagChildrenAsDirty();
}

void Transform::SetParentTransform(Transform* parent, bool relocate)
{
	if (ParentTransform)
		ParentTransform->RemoveChild(this);
	if (!parent)
	{
		ParentTransform = parent;
		return;
	}

	if (relocate)
	{
		Transform parentWorld = parent->GetWorldTransform();
		Transform worldTransform = GetWorldTransform();
		Position = worldTransform.Position - parentWorld.PositionRef;
		Position = glm::vec3(parentWorld.GetRotationMatrix(-1.0f) * PositionRef);
		Rotation = worldTransform.Rotation = parentWorld.RotationRef;
	}

	ParentTransform = parent;
	ParentTransform->AddChild(this);
}

void Transform::AddChild(Transform* t)
{
	Children.push_back(t);
}

void Transform::RemoveChild(Transform* t)
{
	for (unsigned int i = 0; i < Children.size(); i++)
	{
		if (Children[i] == t)
		{
			Children.erase(Children.begin() + i);
			break;
		}
	}
}

bool Transform::GetDirtyFlag(unsigned int index)
{
	return DirtyFlags[index];
}

void Transform::SetDirtyFlag(unsigned int index, bool val)
{
	DirtyFlags[index] = val;
}

void Transform::SetDirtyFlags(bool val)
{
	for (int i = 0; i < DirtyFlags.size(); i++)
		DirtyFlags[i] = val;
}

unsigned int Transform::AddDirtyFlag()
{
	DirtyFlags.push_back(true);
	return DirtyFlags.size() - 1;
}

glm::mat3 ModelToNormal(glm::mat4 model)
{
	return glm::mat3(glm::transpose(glm::inverse(model)));
}

glm::vec3& Transform::operator [](unsigned int i)
{
	FlagChildrenAsDirty();
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