#include <math/Transform.h>
#include <glm/gtx/matrix_decompose.hpp>

float beginning;

void Transform::FlagMyDirtiness() const
{
	Empty = false;
	DirtyFlags[0] = true;
	FlagWorldDirtiness();
}

void Transform::FlagWorldDirtiness() const
{
	for (int i = 1; i < static_cast<int>(DirtyFlags.size()); i++)
		DirtyFlags[i] = true;

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->FlagWorldDirtiness();
}

Transform::Transform():
	Transform(glm::vec3(0.0f))
{
}

Transform::Transform(const glm::vec2& pos, const glm::vec2& scale, const glm::quat& rot):
	Transform(glm::vec3(pos, 0.0f), rot, glm::vec3(scale, 1.0f))
{
}

Transform::Transform(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale) :
	ParentTransform(nullptr),
	WorldTransformCache(nullptr),
	WorldTransformMatrixCache(glm::mat4(1.0f)),
	MatrixCache(glm::mat4(1.0f)),
	Position(pos),
	Rotation(rot),
	Scale(scale),
	DirtyFlags(3, true),
	Empty(false),
	PositionRef(Position),
	RotationRef(Rotation),
	ScaleRef(Scale)
{
	if (pos == glm::vec3(0.0f) && rot == glm::quat(glm::vec3(0.0f)) && scale == glm::vec3(1.0f))
		Empty = true;
	KUPA = false;
}

Transform::Transform(const Transform& t):
	Transform(t.Position, t.Rotation, t.Scale)
{
	/* if (t.ParentTransform)
		t.ParentTransform->AddChild(this); */
}

Transform::Transform(Transform&& t) noexcept :
	Transform(t.Position, t.Rotation, t.Scale)
{
	/*
	if (t.ParentTransform)
	{
		t.ParentTransform->AddChild(this);
		t.ParentTransform->RemoveChild(t.ParentTransform);
	}  */
}

glm::vec3 Transform::GetFrontVec() const
{
	return Rotation * glm::vec3(0.0f, 0.0f, -1.0f);
}

Transform Transform::GetInverse() const
{
	return Transform((glm::mat3)glm::inverse(GetMatrix()) * -PositionRef, glm::inverse(RotationRef), 1.0f / glm::max(ScaleRef, glm::vec3(0.001f)));
}

glm::mat3 Transform::GetRotationMatrix(float scalar) const
{
	glm::quat rot = Rotation;
	if (scalar == -1.0f)
		rot = glm::inverse(rot);

	return glm::mat4_cast(rot);

}

glm::mat4 Transform::GetMatrix() const
{
	if (!DirtyFlags[0])
		return MatrixCache;
	if (Empty)
		return glm::mat4(1.0f);

	MatrixCache = glm::translate(glm::mat4(1.0f), Position);
	MatrixCache *= glm::mat4_cast(Rotation);
	MatrixCache = glm::scale(MatrixCache, Scale);

	DirtyFlags[0] = false;

	return MatrixCache;
}

glm::mat4 Transform::GetViewMatrix() const
{
	return glm::lookAt(Position, Position + GetFrontVec(), glm::vec3(0.0f, 1.0f, 0.0f));
}

const Transform& Transform::GetWorldTransform() const
{
	if (!DirtyFlags[1])
		return *WorldTransformCache;

	if (!ParentTransform)
	{
		WorldTransformCache.reset(new Transform(Position, Rotation, Scale));
		DirtyFlags[1] = false;
		return *WorldTransformCache;
	}
	else if (Empty)
		return ParentTransform->GetWorldTransform();

	WorldTransformCache.reset(new Transform(ParentTransform->GetWorldTransform() * (*this)));
	DirtyFlags[1] = false;

	return *WorldTransformCache;
}

const glm::mat4& Transform::GetWorldTransformMatrix() const
{
	if (!DirtyFlags[2])
		return WorldTransformMatrixCache;

	WorldTransformMatrixCache = GetWorldTransform().GetMatrix();
	DirtyFlags[2] = false;

	return WorldTransformMatrixCache; 
}

Transform* Transform::GetParentTransform() const
{
	return ParentTransform;
}

void Transform::SetPosition(const glm::vec2& pos)
{
	SetPosition(glm::vec3(pos, 0.0f));
}

void Transform::SetPosition(const glm::vec3& pos)
{
	/*CacheVariableInterface<float>(PositionRef.x, *this) = 0.0f;
	CacheVariableInterface<float> sX(PositionRef.x, *this);
	sX = pos.x;
	CacheVariableInterface<float> sY(PositionRef.y, *this);
	sY = pos.y;
	CacheVariableInterface<float> sZ(PositionRef.z, *this);
	sZ = pos.z;*/
	Position = pos;
	FlagMyDirtiness();
}

void Transform::SetPositionWorld(const glm::vec3& worldPos)
{
	glm::vec3 localPos = (ParentTransform) ? (glm::vec3(glm::inverse(ParentTransform->GetWorldTransformMatrix()) * glm::vec4(worldPos, 1.0f))) : (worldPos);
	SetPosition(localPos);
}

void Transform::Move(const glm::vec2& offset)
{
	Move(glm::vec3(offset, 0.0f));
}

void Transform::Move(const glm::vec3& offset)
{
	SetPosition(Position + offset);
}

void Transform::SetRotation(const glm::vec3& euler)
{
	Rotation = toQuat(euler);
	FlagMyDirtiness();
}

void Transform::SetRotation(const glm::quat& q)
{
	Rotation = q;
	FlagMyDirtiness();
}

void Transform::SetRotationWorld(const glm::quat& q)
{
	glm::quat localRot = q;
	if (ParentTransform)
		localRot = glm::inverse(ParentTransform->GetWorldTransform().RotationRef) * localRot;

	SetRotation(localRot);
}

void Transform::Rotate(const glm::vec3& euler)
{
	SetRotation(Rotation * toQuat(euler));
}

void Transform::Rotate(const glm::quat& q)
{
	SetRotation(Rotation * q);
}

void Transform::SetScale(const glm::vec2& scale)
{
	SetScale(glm::vec3(scale, 1.0f));
}

void Transform::SetScale(const glm::vec3& scale)
{/*
	
	CacheVariable<float, Transform> sX(ScaleRef.x, *this);
	sX = scale.x;
	CacheVariable<float, Transform> sY(ScaleRef.y, *this);
	sY = scale.y;
	CacheVariable<float, Transform> sZ(ScaleRef.z, *this);
	sZ = scale.z;*/
	Scale = scale;
	FlagMyDirtiness();
}

void Transform::ApplyScale(float scale)
{
	SetScale(ScaleRef * scale);
}

void Transform::ApplyScale(const glm::vec2& scale)
{
	SetScale(static_cast<glm::vec2>(ScaleRef) * scale);
}

void Transform::ApplyScale(const glm::vec3& scale)
{
	SetScale(ScaleRef * scale);
}

void Transform::Set(int index, const glm::vec3& vec)
{
	switch (index)
	{
	case 0:	SetPosition(vec); return;
	case 1:	SetRotation(vec); return;
	case 2:	SetScale(vec); return;
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

bool Transform::GetDirtyFlag(unsigned int index, bool reset)
{
	if (index == std::numeric_limits<unsigned int>::max())
		return true;
	
	bool flag = DirtyFlags[index];
	if (reset)
		SetDirtyFlag(index, false);

	return flag;
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
	AddInterpolator(fieldName, std::make_unique<Interpolator<T>>(Interpolator<T>(begin, end, T(), max, interpType, fadeAway, before, after, true)), true);
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

void Transform::Print(std::string name) const
{
	std::cout << "===Transform " + name + ", child of " << ParentTransform << "===\n";
	printVector(Position, "Position");
	printVector(Rotation, "Rotation");
	printVector(Scale, "Scale");
	std::cout << "VVVVVV\n";
}

Transform& Transform::operator=(const Transform& t)
{
	Position = t.PositionRef;
	Rotation = t.RotationRef;
	Scale = t.ScaleRef;

	FlagMyDirtiness();

	return *this;
}

Transform& Transform::operator=(Transform&& t) noexcept
{
	Position = t.PositionRef;
	Rotation = t.RotationRef;
	Scale = t.ScaleRef;

	FlagMyDirtiness();

	return *this;
}

Transform& Transform::operator*=(const Transform& t)
{
	FlagMyDirtiness();
	Position += static_cast<glm::mat3>(GetMatrix()) * t.Position;
	Rotation *= t.Rotation;
	Scale *= t.Scale;
	return *(this);
}

Transform& Transform::operator*=(Transform&& t)
{
	FlagMyDirtiness();
	return const_cast<Transform&>(*(this)) *= t;
}

Transform Transform::operator*(const Transform& t) const
{
	Transform copy = *this;
	return copy *= t;
}

template void Transform::AddInterpolator<glm::vec3>(std::string, float, float, glm::vec3, glm::vec3, InterpolationType, bool, AnimBehaviour, AnimBehaviour);
template void Transform::AddInterpolator<glm::vec3>(std::string, float, float, glm::vec3, InterpolationType, bool, AnimBehaviour, AnimBehaviour);

template void Transform::AddInterpolator<glm::quat>(std::string, float, float, glm::quat, glm::quat, InterpolationType, bool, AnimBehaviour, AnimBehaviour);
template void Transform::AddInterpolator<glm::quat>(std::string, float, float, glm::quat, InterpolationType, bool, AnimBehaviour, AnimBehaviour);

glm::quat quatFromDirectionVec(const glm::vec3& dirVec, glm::vec3 up)
{
	glm::vec3 right = glm::normalize(glm::cross(up, dirVec));
	up = glm::normalize(glm::cross(dirVec, right));

	return glm::quat(glm::mat3(right, up, dirVec));
}

Transform decompose(const glm::mat4& mat)
{
	glm::vec3 position, scale, skew;
	glm::vec4 perspective;
	glm::quat rotation;

	glm::decompose(mat, scale, rotation, position, skew, perspective);

	return Transform(position, rotation/*glm::conjugate(rotation)*/, scale);
}
