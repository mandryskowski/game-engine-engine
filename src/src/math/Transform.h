#pragma once
#include <animation/Animation.h>
#include <glfw/glfw3.h>
#include <glm/gtx/euler_angles.hpp>
#include <string>
#include <cereal/archives/json.hpp>
#include "Vec.h"

namespace GEE
{
	enum class VecAxis	//VectorAxis
	{
		X, Y, Z, W
	};

	enum class TVec	//TransformVector
	{
		POSITION,
		ROTATION,
		SCALE,
		ROTATION_EULER
	};


	class Transform
	{
		bool KUPA;
		Transform* ParentTransform;
		mutable UniquePtr<Transform> WorldTransformCache;
		mutable Mat4f WorldTransformMatrixCache;
		mutable Mat4f MatrixCache;

		std::vector <Transform*> Children;

		Vec3f Position;
		Quatf Rotation;
		Vec3f Scale;

		mutable std::vector <bool> DirtyFlags;
		mutable bool Empty;	//true if the Transform object has never been changed. Allows for a simple optimization - we skip it during world transform calculation
	public:
		std::vector <SharedPtr<InterpolatorBase>> Interpolators;

		void FlagMyDirtiness() const;
		void FlagWorldDirtiness() const;

	public:
		explicit Transform();
		explicit Transform(const Vec2f& pos, const Vec2f& scale = Vec2f(1.0f), const Quatf& rot = Quatf(Vec3f(0.0f)));
		explicit Transform(const Vec3f& pos, const Quatf& rot = Quatf(Vec3f(0.0f)), const Vec3f& scale = Vec3f(1.0f));
		Transform(const Transform&);
		Transform(Transform&&) noexcept;

		bool IsEmpty() const { return Empty; }

		const Vec3f& GetPos() const { return Position; }
		const Quatf& GetRot() const { return Rotation; }
		const Vec3f& GetScale() const { return Scale; }

		Vec2f GetPos2D() const { return static_cast<Vec2f>(Position); }
		Vec2f GetScale2D() const { return static_cast<Vec2f>(Scale); }

		Vec3f GetFrontVec() const;
		Transform GetInverse() const;
		Mat3f GetRotationMatrix(bool inverse = false) const;
		Mat4f GetMatrix() const; //returns model matrix
		Mat4f GetViewMatrix() const; //returns view/eye/camera matrix
		const Transform& GetWorldTransform() const; //calculates the world transform (transform data is stored in local space)
		const Mat4f& GetWorldTransformMatrix() const; //calls this->GetWorldTransform().GetMatrix() and then stores the matrix in cache - you should call this method instead of 2 seperate ones (for sweet sweet FPS)
		Transform* GetParentTransform() const;

		void SetPosition(const Vec2f&);
		void SetPosition(const Vec3f&);
		void SetPositionWorld(const Vec3f&);
		void Move(const Vec2f&);
		void Move(const Vec3f&);
		void SetRotation(float roll);
		void SetRotation(const Vec3f& euler);
		void SetRotation(const Quatf& quat);
		void SetRotationWorld(const Quatf& quat);
		void Rotate(const Vec3f& euler);
		void Rotate(const Quatf& quat);
		void SetScale(const Vec2f&);
		void SetScale(const Vec3f&);
		void ApplyScale(float);
		void ApplyScale(const Vec2f&);
		void ApplyScale(const Vec3f&);
		void Set(int, const Vec3f&);
		template <TVec vec, VecAxis axis> void SetVecAxis(float val)
		{
			unsigned int axisIndex = static_cast<unsigned int>(axis);
			GEE_CORE_ASSERT(axisIndex <= 2 || (vec == TVec::ROTATION && axisIndex <= 3));

			switch (vec)
			{
			case TVec::POSITION: Position[axisIndex] = val; break;
			case TVec::ROTATION: Rotation[axisIndex] = val; break;
			case TVec::ROTATION_EULER: Vec3f euler = toEuler(GetRot()); euler[axisIndex] = val; SetRotation(euler); break;
			case TVec::SCALE: Scale[axisIndex] = val; break;
			}

			FlagMyDirtiness();
		}

		void SetParentTransform(Transform* transform, bool relocate = false); //if the bool is true, the transform is recalculated to the parent's space
		void AddChild(Transform*);
		void RemoveChild(Transform*);

		bool GetDirtyFlag(unsigned int index = 0, bool reset = true);
		void SetDirtyFlag(unsigned int index = 0, bool val = true);
		void SetDirtyFlags(bool val = true);
		unsigned int AddDirtyFlag();

		void AddInterpolator(std::string fieldName, SharedPtr<InterpolatorBase>, bool animateFromCurrent = true);	//if animateFromCurrent is true, the method automatically changes the minimum value of the interpolator to be the current value of the interpolated variable.
		template <class T> void AddInterpolator(std::string fieldName, float begin, float end, T min, T max, InterpolationType interpType = InterpolationType::LINEAR, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP);
		template <class T> void AddInterpolator(std::string fieldName, float begin, float end, T max, InterpolationType interpType = InterpolationType::LINEAR, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP);
		void Update(float deltaTime);

		template <typename Archive> void Serialize(Archive& archive)
		{
			archive(CEREAL_NVP(Position), CEREAL_NVP(Rotation), CEREAL_NVP(Scale));
			FlagMyDirtiness();
		}

		void GetEditorDescription(EditorDescriptionBuilder);

		void Print(std::string name = "unnamed") const;

		~Transform() = default;

		Transform operator*(const Transform&) const;

		Transform& operator=(const Transform&);
		Transform& operator=(Transform&&) noexcept;

		Transform& operator*=(const Transform&);
		Transform& operator*=(Transform&&);
	};

	namespace Math
	{
		Mat4f SafeInverseMatrix(const Mat4f& t);
		Mat4f SafeInverseMatrix(const Transform& t);

		Vec3f CutYAxis(const Vec3f&);
	}

	Quatf quatFromDirectionVec(const Vec3f& dirVec, Vec3f up = Vec3f(0.0f, 1.0f, 0.0f));
	/*const Transform& operator*(const Mat4f& mat, const Transform& t)
	{
		Vec3f scale(0.0f);
		glm::scale(mat, scale);
		return Transform(mat * Vec4f(t.PositionRef, 1.0f), (mat) * t.RotationRef, scale * t.ScaleRef);
	}*/

	Transform decompose(const Mat4f&);
}