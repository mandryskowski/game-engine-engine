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

		for (auto& flag : LocalDirtyFlags)
			flag = true;

		FlagWorldDirtiness();
	}

	void Transform::FlagWorldDirtiness() const
	{
		for (auto& flag : WorldDirtyFlags)
			flag = true;

		for (auto child : Children)
			child->FlagWorldDirtiness();
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
		LocalDirtyFlags(CORE_LOCAL_FLAGS, true),
		WorldDirtyFlags(CORE_WORLD_FLAGS, true),
		Empty(false)
	{
		if (pos == Vec3f(0.0f) && rot == Quatf(Vec3f(0.0f)) && scale == Vec3f(1.0f))
			Empty = true;
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

	Mat3f Transform::GetRotationMatrix(bool inverse) const
	{
		if (inverse)
			return glm::mat4_cast(glm::inverse(Rotation));

		return glm::mat4_cast(Rotation);
	}

	Mat4f Transform::GetMatrix() const
	{
		if (!LocalDirtyFlags[0])
			return MatrixCache;
		if (Empty)
			return Mat4f(1.0f);

		MatrixCache = glm::translate(Mat4f(1.0f), Position);
		MatrixCache *= glm::mat4_cast(Rotation);
		MatrixCache = glm::scale(MatrixCache, Scale);

		LocalDirtyFlags[0] = false;

		return MatrixCache;
	}

	Mat4f Transform::GetViewMatrix() const
	{
		return glm::lookAt(Position, Position + GetFrontVec(), Vec3f(0.0f, 1.0f, 0.0f));
	}

	const Transform& Transform::GetWorldTransform() const
	{
		if (!WorldDirtyFlags[0])
			return *WorldTransformCache;

		if (!ParentTransform)
		{
			WorldTransformCache.reset(new Transform(Position, Rotation, Scale));
			WorldDirtyFlags[0] = false;
			return *WorldTransformCache;
		}
		else if (Empty)
			return ParentTransform->GetWorldTransform();

		WorldTransformCache.reset(new Transform(ParentTransform->GetWorldTransform() * (*this)));
		WorldDirtyFlags[0] = false;

		return *WorldTransformCache;
	}

	const Mat4f& Transform::GetWorldTransformMatrix() const
	{
		if (!WorldDirtyFlags[1])
			return WorldTransformMatrixCache;

		WorldTransformMatrixCache = GetWorldTransform().GetMatrix();
		WorldDirtyFlags[1] = false;

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

	void Transform::SetRotation(float roll)
	{
		SetVecAxis<TVec::RotationEuler, VecAxis::Z>(roll);
		FlagMyDirtiness();
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
	{
		Scale = scale;
		FlagMyDirtiness();
	}

	void Transform::SetScaleWorld(const Vec3f& worldScale)
	{
		Vec3f localPos = (ParentTransform) ? ((1.0f / ParentTransform->GetWorldTransform().GetScale()) * worldScale) : (worldScale);
		SetScale(localPos);
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
			Position = Vec3f(parentWorld.GetRotationMatrix(true) * GetPos());
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

	void Transform::AddInterpolator(const String& fieldName, SharedPtr<InterpolatorBase> interpolator, bool animateFromCurrent)
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
			String type = (vecCast) ? ("Vec3f") : ((quatCast) ? ("Quatf") : ("unknown"));
			std::cerr << "ERROR! Unrecognized interpolator " << fieldName << " of type " + type << ".\n";
			return;
		}


		if (animateFromCurrent)
			obj->ResetMinVal();
	}

	template <class T>
	void Transform::AddInterpolator(const String& fieldName, Time begin, Time end, T min, T max, InterpolationType interpType, bool fadeAway, AnimBehaviour before, AnimBehaviour after)
	{
		AddInterpolator(fieldName, MakeUnique<Interpolator<T>>(Interpolator<T>(begin, end, min, max, interpType, fadeAway, before, after, false)), false);
	}

	template <class T>
	void Transform::AddInterpolator(const String& fieldName, Time begin, Time end, T max, InterpolationType interpType, bool fadeAway, AnimBehaviour before, AnimBehaviour after)
	{
		AddInterpolator(fieldName, MakeUnique<Interpolator<T>>(Interpolator<T>(begin, end, T(), max, interpType, fadeAway, before, after, true)), true);
	}

	void Transform::Update(Time deltaTime)
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
		auto posVecBoxes = descBuilder.AddField("Position").GetTemplates().VecInput<3, float>([this](int axis, float val) {Vec3f pos = GetPos(); pos[axis] = val; SetPosition(pos); }, [this](int axis) { return GetPos()[axis]; }).VecInputBoxes;
		for (auto it : posVecBoxes)
			it->SetRetrieveContentEachFrame(true);

		auto rotQuatVecBoxes = descBuilder.AddField("Rotation Quat").GetTemplates().VecInput<4, float>(nullptr, nullptr).VecInputBoxes;
		auto rotEulerVecBoxes = descBuilder.AddField("Rotation Euler").GetTemplates().VecInput<3, float>([this, rotQuatVecBoxes](int axis, float val) {Vec3f rot = toEuler(GetRot()); rot[axis] = val; SetRotation(rot); for (auto it : rotQuatVecBoxes) it->UpdateValue(); }, [this](int axis) { return toEuler(GetRot())[axis]; }).VecInputBoxes;

		// Defer setting input funcs of quaternion vec boxes because we need euler vec boxes to be created in order to update their value upon editting the quaternion value. If we did it right away, we wouldn't have had any reference to euler boxes, so we do it here.
		for (int i = 0; i < static_cast<int>(rotQuatVecBoxes.size()); i++)
			rotQuatVecBoxes[i]->SetOnInputFunc([this, i, rotEulerVecBoxes](float val) {Quatf rot = GetRot(); rot[i] = val; SetRotation(glm::normalize(rot)); for (auto it : rotEulerVecBoxes) it->UpdateValue(); }, [this, i]() { return GetRot()[i]; });

		descBuilder.AddField("Scale").GetTemplates().VecInput<3, float>([this](int axis, float val) {Vec3f scale = GetScale(); scale[axis] = val; SetScale(scale); }, [this](int axis) { return GetScale()[axis]; });
	}

	void Transform::Print(String name) const
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

	Mat4f Math::SafeInverseMatrix(const Mat4f& mat)
	{
		bool hasZero = false;
		for (int i = 0; i < 3; i++)
		{
			if (mat[i][i] == 0.0f)
			{
				hasZero = true;
				break;
			}
		}

		if (hasZero)
		{
			Mat4f matCopy = mat;
			for (int i = 0; i < 3; i++)
				matCopy[i][i] = glm::max(mat[i][i], 0.001f);
			return glm::inverse(matCopy);
		}
		
		return glm::inverse(mat);
	}

	Mat4f Math::SafeInverseMatrix(const Transform& t)
	{
		if (const Vec3f& scale = t.GetScale(); scale.x == 0.0f || scale.y == 0.0f || scale.z == 0.0f)
		{
			Transform tCopy = t;
			tCopy.SetScale(glm::max(scale, 0.001f));
			return glm::inverse(tCopy.GetMatrix());
		}

		return glm::inverse(t.GetMatrix());
	}

	Vec3f Math::CutYAxis(const Vec3f& vec)
	{
		return Vec3f(vec.x, 0.0f, vec.z);
	}

	template void Transform::AddInterpolator<Vec3f>(const String&, Time, Time, Vec3f, Vec3f, InterpolationType, bool, AnimBehaviour, AnimBehaviour);
	template void Transform::AddInterpolator<Vec3f>(const String&, Time, Time, Vec3f, InterpolationType, bool, AnimBehaviour, AnimBehaviour);

	template void Transform::AddInterpolator<Quatf>(const String&, Time, Time, Quatf, Quatf, InterpolationType, bool, AnimBehaviour, AnimBehaviour);
	template void Transform::AddInterpolator<Quatf>(const String&, Time, Time, Quatf, InterpolationType, bool, AnimBehaviour, AnimBehaviour);

	Quatf quatFromDirectionVec(const Vec3f& dirVec, Vec3f up)
	{
		Vec3f right = glm::normalize(glm::cross(up, -dirVec));
		up = glm::normalize(glm::cross(-dirVec, right));

		return Quatf(Mat3f(right, up, -dirVec));
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