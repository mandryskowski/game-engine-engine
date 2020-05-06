#include "Transform.h"

float Transform::tSum = 0.0f;
float beginning;

void Transform::FlagMeAndChildrenAsDirty()
{
	for (unsigned int i = 0; i < DirtyFlags.size(); i++)
		DirtyFlags[i] = true;

	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->FlagMeAndChildrenAsDirty();
}

Transform::Transform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale, glm::vec3 front, bool constrain, bool rotLock):
	ParentTransform(nullptr),
	WorldTransformCache(nullptr),
	WorldTransformMatrixCache(glm::mat4(1.0f)),
	Position(pos),
	Rotation(rot),
	Scale(scale),
	Front(front),
	DirtyFlags(2, true),
	bConstrain(constrain),
	bRotationLock(rotLock),
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
	bRotationLock(t.bRotationLock),
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
		beginning = (float)glfwGetTime();
	time1 = (float)glfwGetTime() - beginning;
	glm::mat4 mat = glm::translate(glm::mat4(1.0f), Position);

	
	if (bConstrain)
		mat *= glm::eulerAngleYXZ(glm::radians(Rotation.y), glm::radians(Rotation.x), glm::radians(Rotation.z));
	else
		mat *= glm::eulerAngleXYZ(glm::radians(Rotation.x), glm::radians(Rotation.y), glm::radians(Rotation.z));

	mat = glm::scale(mat, Scale);

	time2 = (float)glfwGetTime() - beginning;
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
	{
		if (WorldTransformCache)
			delete WorldTransformCache;
		WorldTransformCache = new Transform(Position, Rotation, Scale, Front, bConstrain);
		DirtyFlags[0] = false;
		return *WorldTransformCache;
	}

	Transform ParentWorldTransform = ParentTransform->GetWorldTransform();

	glm::vec3 pos = ParentWorldTransform.Position;
	glm::vec3 front = Front;

	if (!bRotationLock)
	{
		glm::mat3 parentMat = ParentWorldTransform.GetMatrix();
		glm::mat3 mat = parentMat * (glm::mat3)GetMatrix();

		pos += glm::vec3(parentMat * Position);

		if (Front != glm::vec3(0.0f))
			front = ModelToNormal(mat) * front;
	}
	else
	{
		pos += Position;
	}

	glm::vec3 rot = ParentWorldTransform.Rotation + Rotation;
	glm::vec3 scale = ParentWorldTransform.Scale * Scale;

	if (WorldTransformCache)
		delete WorldTransformCache;
	WorldTransformCache = new Transform(pos, rot, scale, front, bConstrain);
	DirtyFlags[0] = false;

	return *WorldTransformCache;
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
	FlagMeAndChildrenAsDirty();
}

void Transform::Move(glm::vec3 offset)
{
	SetPosition(Position + offset);
}

void Transform::SetRotation(glm::vec3 rot)
{
	Rotation = rot;
	FlagMeAndChildrenAsDirty();
}

void Transform::Rotate(glm::vec3 offset)
{
	SetRotation(Rotation + offset);
}

void Transform::SetScale(glm::vec3 scale)
{
	Scale = scale;
	FlagMeAndChildrenAsDirty();
}

void Transform::SetFront(glm::vec3 front)
{
	Front = front;
	FlagMeAndChildrenAsDirty();
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
	return (unsigned int)DirtyFlags.size() - 1;
}

void Transform::AddInterpolator(Interpolator<glm::vec3>* interpolator, std::string name)
{
	Interpolators.push_back(interpolator);

	if (name == "position")
		Interpolators.back()->SetValPtr(&Position);
	else if (name == "rotation")
		Interpolators.back()->SetValPtr(&Rotation);
	else if (name == "scale")
		Interpolators.back()->SetValPtr(&Scale);
	else if (name == "front")
		Interpolators.back()->SetValPtr(&Front);
	else
		std::cerr << "ERROR! Unrecognized interpolator type: " << name << ".\n";
}

void Transform::Update(float deltaTime)
{
	if (Interpolators.empty())
		return;

	bool dirty = false;

	for (unsigned int i = 0; i < Interpolators.size(); i++)
	{
		Interpolators[i]->Update(deltaTime);
		if (Interpolators[i]->GetInterp()->IsChanging())
			dirty = true;
	}

	if (dirty)
		FlagMeAndChildrenAsDirty();
}

glm::mat3 ModelToNormal(glm::mat4 model)
{
	return glm::mat3(glm::transpose(glm::inverse(model)));
}

glm::vec3& Transform::operator [](unsigned int i)
{
	FlagMeAndChildrenAsDirty();
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