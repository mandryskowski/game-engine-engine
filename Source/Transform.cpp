#include "Transform.h"

float Transform::tSum = 0.0f;
float beginning;

void Transform::FlagMyDirtiness()
{
	Empty = false;
	DirtyFlags[0] = true;
	FlagWorldDirtiness();
}

void Transform::FlagWorldDirtiness()
{
	for (int i = 1; i < static_cast<int>(DirtyFlags.size()); i++)
		DirtyFlags[i] = true;

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->FlagWorldDirtiness();
}

Transform::Transform(glm::vec3 pos, glm::quat rot, glm::vec3 scale, glm::vec3 front, bool constrain, bool rotLock) :
	ParentTransform(nullptr),
	WorldTransformCache(nullptr),
	WorldTransformMatrixCache(glm::mat4(1.0f)),
	MatrixCache(glm::mat4(1.0f)),
	Position(pos),
	Rotation(rot),
	Scale(scale),
	Front(front),
	DirtyFlags(3, true),
	Empty(false),
	bConstrain(constrain),
	bRotationLock(rotLock),
	PositionRef(Position),
	RotationRef(Rotation),
	ScaleRef(Scale),
	FrontRef(Front)
{
	if (pos == glm::vec3(0.0f) && rot == glm::quat(glm::vec3(0.0f)) && scale == glm::vec3(1.0f) && front == glm::vec3(0.0f))
		Empty = true;
	KUPA = false;
}

Transform::Transform(const Transform& t):
	Transform(t.Position, t.Rotation, t.Scale, t.Front, t.bConstrain, t.bRotationLock)
{
	/* if (t.ParentTransform)
		t.ParentTransform->AddChild(this); */
}

Transform::Transform(Transform&& t) noexcept :
	Transform(t.Position, t.Rotation, t.Scale, t.Front, t.bConstrain, t.bRotationLock)
{
	/*
	if (t.ParentTransform)
	{
		t.ParentTransform->AddChild(this);
		t.ParentTransform->RemoveChild(t.ParentTransform);
	}  */
}

glm::mat3 Transform::GetRotationMatrix(float scalar) const
{
	glm::quat rot = Rotation;
	if (scalar == -1.0f)
		rot = glm::inverse(rot);

	return glm::mat4_cast(rot);

	/*if (!bConstrain)
		return (glm::mat3)glm::eulerAngleXYZ(glm::radians(rot.x), glm::radians(rot.y), glm::radians(rot.z));

	return (glm::mat3)glm::eulerAngleYXZ(glm::radians(rot.y), glm::radians(rot.x), glm::radians(rot.z));
	*/
}

glm::mat4 Transform::GetMatrix() const
{
	float time1, time2;
	if (tSum == 0.0f)
		beginning = (float)glfwGetTime();
	time1 = (float)glfwGetTime() - beginning;

	if (!DirtyFlags[0])
		return MatrixCache;
	if (Empty)
		return glm::mat4(1.0f);

	MatrixCache = glm::translate(glm::mat4(1.0f), Position);

	
	/*if (bConstrain)
		mat *= glm::eulerAngleYXZ(glm::radians(Rotation.y), glm::radians(Rotation.x), glm::radians(Rotation.z));
	else
		mat *= glm::eulerAngleXYZ(glm::radians(Rotation.x), glm::radians(Rotation.y), glm::radians(Rotation.z));*/
	MatrixCache *= glm::mat4_cast(Rotation);

	MatrixCache = glm::scale(MatrixCache, Scale);

	time2 = (float)glfwGetTime() - beginning;
	tSum += time2 - time1;

	DirtyFlags[0] = false;

	return MatrixCache;
}

glm::mat4 Transform::GetViewMatrix() const
{
	return glm::lookAt(Position, Position + Front, glm::vec3(0.0f, 1.0f, 0.0f));
}

const Transform& Transform::GetWorldTransform() const
{
	if (!DirtyFlags[1])
		return *WorldTransformCache;

	if (!ParentTransform)
	{
		WorldTransformCache.reset(new Transform(Position, Rotation, Scale, Front));
		DirtyFlags[1] = false;
		return *WorldTransformCache;
	}
	else if (Empty)
		return ParentTransform->GetWorldTransform();

	Transform ParentWorldTransform = ParentTransform->GetWorldTransform();
	glm::mat3 parentMat = static_cast<glm::mat3>(ParentWorldTransform.GetMatrix());

	glm::vec3 pos = ParentWorldTransform.Position + glm::vec3(parentMat * Position);
	glm::quat rot = ParentWorldTransform.Rotation * Rotation;
	glm::vec3 scale = ParentWorldTransform.Scale * Scale;
	glm::vec3 front = (Front == glm::vec3(0.0f)) ? (glm::vec3(0.0f)) : (glm::normalize(rot * Front));

	WorldTransformCache.reset(new Transform(pos, rot, scale, front));
	DirtyFlags[1] = false;

	return *WorldTransformCache;
}

const glm::mat4& Transform::GetWorldTransformMatrix() const
{
	if (!DirtyFlags[2])
		return WorldTransformMatrixCache;

	Transform worldTransform = GetWorldTransform();
	WorldTransformMatrixCache = worldTransform.GetMatrix();
	DirtyFlags[2] = false;

	return WorldTransformMatrixCache;
}

Transform* Transform::GetParentTransform() const
{
	return ParentTransform;
}

void Transform::SetPosition(glm::vec3 pos)
{
	Position = pos;
	FlagMyDirtiness();
}

void Transform::SetPositionWorld(glm::vec3 worldPos)
{
	glm::vec3 localPos = worldPos;
	if (ParentTransform)
		localPos = glm::vec3(glm::inverse(ParentTransform->GetWorldTransformMatrix()) * glm::vec4(worldPos, 1.0f));

	SetPosition(localPos);
}

void Transform::Move(glm::vec3 offset)
{
	SetPosition(Position + offset);
}

void Transform::SetRotation(glm::vec3 euler)
{
	Rotation = toQuat(euler);
	FlagMyDirtiness();
}

void Transform::SetRotation(glm::quat q)
{
	Rotation = q;
	FlagMyDirtiness();
}

void Transform::SetRotationWorld(glm::quat q)
{
	glm::quat localRot = q;
	if (ParentTransform)
		localRot = glm::inverse(ParentTransform->GetWorldTransform().RotationRef) * localRot;

	SetRotation(localRot);
}

void Transform::Rotate(glm::vec3 euler)
{
	SetRotation(Rotation * toQuat(euler));
}

void Transform::Rotate(glm::quat q)
{
	SetRotation(Rotation * q);
}

void Transform::SetScale(glm::vec3 scale)
{
	Scale = scale;
	FlagMyDirtiness();
}

void Transform::SetFront(glm::vec3 front)
{
	Front = front;
	FlagMyDirtiness();
}

void Transform::Set(int index, glm::vec3 vec)
{
	switch (index)
	{
	case 0:	SetPosition(vec); return;
	case 1:	SetRotation(vec); return;
	case 2:	SetScale(vec); return;
	case 3: SetFront(vec); return;
	default: std::cerr << "ERROR! Tried to access Transform field nr " << index << ".\n";
	}
}

void Transform::SetParentTransform(Transform* parent, bool relocate)
{
	if (ParentTransform)
		ParentTransform->RemoveChild(this);
	if (!parent)
	{
		ParentTransform = nullptr;
		return;
	}

	if (relocate)
	{
		//TODO: zmien to kurwa bo siara jak ktos zobaczy
		Transform parentWorld = parent->GetWorldTransform();
		Transform worldTransform = GetWorldTransform();
		Position = worldTransform.Position - parentWorld.PositionRef;
		Position = glm::vec3(parentWorld.GetRotationMatrix(-1.0f) * PositionRef);
		Rotation = worldTransform.Rotation = parentWorld.RotationRef;

	
		/*
		glm::mat4 parentWorldMat = parent->GetWorldTransformMatrix();
		Position = glm::vec3(glm::inverse(parentWorldMat) * glm::vec4(Position, 1.0f));
		Rotation = glm::inverse(parent->GetWorldTransform().RotationRef) * Rotation;
		Rotation = parent->GetWorldTransform().RotationRef;*/
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

void Transform::AddInterpolator(std::string fieldName, std::shared_ptr<InterpolatorBase> interpolator, bool animateFromCurrent)
{
	Interpolators.push_back(interpolator);
	std::shared_ptr<InterpolatorBase> obj = Interpolators.back();
	Interpolator<glm::vec3>* vecCast = dynamic_cast<Interpolator<glm::vec3>*>(obj.get());
	Interpolator<glm::quat>* quatCast = dynamic_cast<Interpolator<glm::quat>*>(obj.get());

	if (fieldName == "position" && vecCast)
		vecCast->SetValPtr(&Position);
	else if (fieldName == "rotation" && quatCast)
		quatCast->SetValPtr(&Rotation);
	else if (fieldName == "scale" && vecCast)
		vecCast->SetValPtr(&Scale);
	else if (fieldName == "front")
		vecCast->SetValPtr(&Front);
	else
	{
		std::string type = (vecCast) ? ("glm::vec3") : ((quatCast) ? ("glm::quat") : ("unknown"));
		std::cerr << "ERROR! Unrecognized interpolator " << fieldName << " of type " + type << ".\n";
		return;
	}


	if (animateFromCurrent)
		obj->ResetMinVal();
}

template <class T>
void Transform::AddInterpolator(std::string fieldName, float begin, float end, T min, T max, InterpolationType interpType, bool fadeAway, AnimBehaviour before, AnimBehaviour after)
{
	AddInterpolator(fieldName, std::make_unique<Interpolator<T>>(Interpolator<T>(begin, end, min, max, interpType, fadeAway, before, after, false)), false);
}

template <class T>
void Transform::AddInterpolator(std::string fieldName, float begin, float end, T max, InterpolationType interpType, bool fadeAway, AnimBehaviour before, AnimBehaviour after)
{
	AddInterpolator(fieldName, std::make_unique<Interpolator<T>>(Interpolator<T>(begin, end, T(), max, interpType, fadeAway, before, after)), true);
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
		else if (Interpolators[i]->GetHasEnded())
		{
			Interpolators.erase(Interpolators.begin() + i);
			i--;
		}
	}

	if (dirty)
		FlagMyDirtiness();
}

Transform& Transform::operator=(const Transform& t)
{
	Position = t.PositionRef;
	Rotation = t.RotationRef;
	Scale = t.ScaleRef;
	Front = t.FrontRef;

	FlagMyDirtiness();

	return *this;
}

Transform& Transform::operator=(Transform&& t) noexcept
{
	Position = t.PositionRef;
	Rotation = t.RotationRef;
	Scale = t.ScaleRef;
	Front = t.FrontRef;

	FlagMyDirtiness();

	return *this;
}

Transform& Transform::operator*=(const Transform& t)
{
	return *(this) * t;
}

Transform& Transform::operator*=(Transform&& t)
{
	return *(this) * t;
}

Transform& Transform::operator*(const Transform& t)
{
	Position += t.Position;
	Rotation *= t.Rotation;
	Scale *= t.Scale;

	return *this;
}

Transform& Transform::operator*(Transform&& t)
{
	Position += t.Position;
	Rotation *= t.Rotation;
	Scale *= t.Scale;

	return *this;
}

glm::mat3 ModelToNormal(glm::mat4 model)
{
	return glm::mat3(glm::transpose(glm::inverse(model)));
}

template void Transform::AddInterpolator<glm::vec3>(std::string, float, float, glm::vec3, glm::vec3, InterpolationType, bool, AnimBehaviour, AnimBehaviour);
template void Transform::AddInterpolator<glm::vec3>(std::string, float, float, glm::vec3, InterpolationType, bool, AnimBehaviour, AnimBehaviour);

template void Transform::AddInterpolator<glm::quat>(std::string, float, float, glm::quat, glm::quat, InterpolationType, bool, AnimBehaviour, AnimBehaviour);
template void Transform::AddInterpolator<glm::quat>(std::string, float, float, glm::quat, InterpolationType, bool, AnimBehaviour, AnimBehaviour);