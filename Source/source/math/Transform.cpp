#include <math/Transform.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <UI/UICanvasActor.h> // for EditorDescriptionBuilder
#include <UI/UICanvasField.h> // for EditorDescriptionBuilder

namespace GEE
{
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

	Transform::Transform() :
		Transform(Vec3f(0.0f))
	{
	}

	Transform::Transform(const Vec2f& pos, const Vec2f& scale, const Quatf& rot) :
		Transform(Vec3f(pos, 0.0f), rot, Vec3f(scale, 1.0f))
	{
	}

	Transform::Transform(const Vec3f& pos, const Quatf& rot, const Vec3f& scale) :
		ParentTransform(nullptr),
		WorldTransformCache(nullptr),
		WorldTransformMatrixCache(Mat4f(1.0f)),
		MatrixCache(Mat4f(1.0f)),
		Position(pos),
		Rotation(rot),
		Scale(scale),
		DirtyFlags(3, true),
		Empty(false)
	{
		if (pos == Vec3f(0.0f) && rot == Quatf(Vec3f(0.0f)) && scale == Vec3f(1.0f))
			Empty = true;
		KUPA = false;
	}

	Transform::Transform(const Transform& t) :
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

	Vec3f Transform::GetFrontVec() const
	{
		return Rotation * Vec3f(0.0f, 0.0f, -1.0f);
	}

	Transform Transform::GetInverse() const
	{
		return Transform((Mat3f)glm::inverse(GetMatrix()) * -GetPos(), glm::inverse(GetRot()), 1.0f / glm::max(GetScale(), Vec3f(0.001f)));
	}

	Mat3f Transform::GetRotationMatrix(float scalar) const
	{
		Quatf rot = Rotation;
		if (scalar == -1.0f)
			rot = glm::inverse(rot);

		return glm::mat4_cast(rot);

	}

	Mat4f Transform::GetMatrix() const
	{
		if (!DirtyFlags[0])
			return MatrixCache;
		if (Empty)
			return Mat4f(1.0f);

		MatrixCache = glm::translate(Mat4f(1.0f), Position);
		MatrixCache *= glm::mat4_cast(Rotation);
		MatrixCache = glm::scale(MatrixCache, Scale);

		DirtyFlags[0] = false;

		return MatrixCache;
	}

	Mat4f Transform::GetViewMatrix() const
	{
		return glm::lookAt(Position, Position + GetFrontVec(), Vec3f(0.0f, 1.0f, 0.0f));
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

	const Mat4f& Transform::GetWorldTransformMatrix() const
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

	void Transform::SetPosition(const Vec2f& pos)
	{
		SetPosition(Vec3f(pos, 0.0f));
	}

	void Transform::SetPosition(const Vec3f& pos)
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

	void Transform::SetPositionWorld(const Vec3f& worldPos)
	{
		Vec3f localPos = (ParentTransform) ? (Vec3f(glm::inverse(ParentTransform->GetWorldTransformMatrix()) * Vec4f(worldPos, 1.0f))) : (worldPos);
		SetPosition(localPos);
	}

	void Transform::Move(const Vec2f& offset)
	{
		Move(Vec3f(offset, 0.0f));
	}

	void Transform::Move(const Vec3f& offset)
	{
		SetPosition(Position + offset);
	}

	void Transform::SetRotation(const Vec3f& euler)
	{
		Rotation = toQuat(euler);
		FlagMyDirtiness();
	}

	void Transform::SetRotation(const Quatf& q)
	{
		Rotation = q;
		FlagMyDirtiness();
	}

	void Transform::SetRotationWorld(const Quatf& q)
	{
		Quatf localRot = q;
		if (ParentTransform)
			localRot = glm::inverse(ParentTransform->GetWorldTransform().GetRot()) * localRot;

		SetRotation(localRot);
	}

	void Transform::Rotate(const Vec3f& euler)
	{
		SetRotation(Rotation * toQuat(euler));
	}

	void Transform::Rotate(const Quatf& q)
	{
		SetRotation(Rotation * q);
	}

	void Transform::SetScale(const Vec2f& scale)
	{
		SetScale(Vec3f(scale, 1.0f));
	}

	void Transform::SetScale(const Vec3f& scale)
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
		SetScale(GetScale() * scale);
	}

	void Transform::ApplyScale(const Vec2f& scale)
	{
		SetScale(static_cast<Vec2f>(GetScale()) * scale);
	}

	void Transform::ApplyScale(const Vec3f& scale)
	{
		SetScale(GetScale() * scale);
	}

	void Transform::Set(int index, const Vec3f& vec)
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
			Position = worldTransform.Position - parentWorld.GetPos();
			Position = Vec3f(parentWorld.GetRotationMatrix(-1.0f) * GetPos());
			Rotation = worldTransform.Rotation = parentWorld.GetRot();


			/*
			Mat4f parentWorldMat = parent->GetWorldTransformMatrix();
			Position = Vec3f(glm::inverse(parentWorldMat) * Vec4f(Position, 1.0f));
			Rotation = glm::inverse(parent->GetWorldTransform().RotationRef) * Rotation;
			Rotation = parent->GetWorldTransform().RotationRef;*/
		}

		ParentTransform = parent;
		ParentTransform->AddChild(this);

		FlagWorldDirtiness();
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
		return DirtyFlags.size() - 1;
	}

	void Transform::AddInterpolator(std::string fieldName, SharedPtr<InterpolatorBase> interpolator, bool animateFromCurrent)
	{
		Interpolators.push_back(interpolator);
		SharedPtr<InterpolatorBase> obj = Interpolators.back();
		Interpolator<Vec3f>* vecCast = dynamic_cast<Interpolator<Vec3f>*>(obj.get());
		Interpolator<Quatf>* quatCast = dynamic_cast<Interpolator<Quatf>*>(obj.get());

		if (fieldName == "position" && vecCast)
			vecCast->SetValPtr(&Position);
		else if (fieldName == "rotation" && quatCast)
			quatCast->SetValPtr(&Rotation);
		else if (fieldName == "scale" && vecCast)
			vecCast->SetValPtr(&Scale);
		else
		{
			std::string type = (vecCast) ? ("Vec3f") : ((quatCast) ? ("Quatf") : ("unknown"));
			std::cerr << "ERROR! Unrecognized interpolator " << fieldName << " of type " + type << ".\n";
			return;
		}


		if (animateFromCurrent)
			obj->ResetMinVal();
	}

	template <class T>
	void Transform::AddInterpolator(std::string fieldName, float begin, float end, T min, T max, InterpolationType interpType, bool fadeAway, AnimBehaviour before, AnimBehaviour after)
	{
		AddInterpolator(fieldName, MakeUnique<Interpolator<T>>(Interpolator<T>(begin, end, min, max, interpType, fadeAway, before, after, false)), false);
	}

	template <class T>
	void Transform::AddInterpolator(std::string fieldName, float begin, float end, T max, InterpolationType interpType, bool fadeAway, AnimBehaviour before, AnimBehaviour after)
	{
		AddInterpolator(fieldName, MakeUnique<Interpolator<T>>(Interpolator<T>(begin, end, T(), max, interpType, fadeAway, before, after, true)), true);
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


	void Transform::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		descBuilder.AddField("Position").GetTemplates().VecInput<Vec3f>([this](float x, float val) {Vec3f pos = GetPos(); pos[x] = val; SetPosition(pos); }, [this](float x) { return GetPos()[x]; });
		descBuilder.AddField("Rotation").GetTemplates().VecInput<Quatf>([this](float x, float val) {Quatf rot = GetRot(); rot[x] = val; SetRotation(glm::normalize(rot)); }, [this](float x) { return GetRot()[x]; });
		descBuilder.AddField("Scale").GetTemplates().VecInput<Vec3f>([this](float x, float val) {Vec3f scale = GetScale(); scale[x] = val; SetScale(scale); }, [this](float x) { return GetScale()[x]; });
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
		Position = t.GetPos();
		Rotation = t.GetRot();
		Scale = t.GetScale();

		FlagMyDirtiness();

		return *this;
	}

	Transform& Transform::operator=(Transform&& t) noexcept
	{
		Position = t.GetPos();
		Rotation = t.GetRot();
		Scale = t.GetScale();

		FlagMyDirtiness();

		return *this;
	}

	Transform& Transform::operator*=(const Transform& t)
	{
		FlagMyDirtiness();
		Position += static_cast<Mat3f>(GetMatrix()) * t.Position;
		Rotation *= t.Rotation;
		Scale *= t.Scale;
		return *(this);
	}

	Transform& Transform::operator*=(Transform&& t)
	{
		FlagMyDirtiness();
		return *this *= t;
	}

	Transform Transform::operator*(const Transform& t) const
	{
		Transform copy = *this;
		return copy *= t;
	}

	template void Transform::AddInterpolator<Vec3f>(std::string, float, float, Vec3f, Vec3f, InterpolationType, bool, AnimBehaviour, AnimBehaviour);
	template void Transform::AddInterpolator<Vec3f>(std::string, float, float, Vec3f, InterpolationType, bool, AnimBehaviour, AnimBehaviour);

	template void Transform::AddInterpolator<Quatf>(std::string, float, float, Quatf, Quatf, InterpolationType, bool, AnimBehaviour, AnimBehaviour);
	template void Transform::AddInterpolator<Quatf>(std::string, float, float, Quatf, InterpolationType, bool, AnimBehaviour, AnimBehaviour);

	Quatf quatFromDirectionVec(const Vec3f& dirVec, Vec3f up)
	{
		Vec3f right = glm::normalize(glm::cross(up, dirVec));
		up = glm::normalize(glm::cross(dirVec, right));

		return Quatf(Mat3f(right, up, dirVec));
	}

	Transform decompose(const Mat4f& mat)
	{
		Vec3f position, scale, skew;
		Vec4f perspective;
		Quatf rotation;

		glm::decompose(mat, scale, rotation, position, skew, perspective);

		return Transform(position, rotation/*glm::conjugate(rotation)*/, scale);
	}

}