#pragma once
#include <animation/Animation.h>
#include <glfw/glfw3.h>
#include <glm/gtx/euler_angles.hpp>
#include <string>
#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>

struct PhysicsInput
{
	glm::vec3 Position;
	glm::quat Rotation;
};

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

class Vec2f : public glm::vec2
{
	using glm::vec2::vec2;
public:
	template <typename Archive> void Serialize(Archive& archive)
	{
		archive(CEREAL_NVP(x), CEREAL_NVP(y));
	}
	glm::vec2 GetGlmType() const
	{
		return static_cast<glm::vec2>(*this);
	}
};

class Vec3f : public glm::vec3
{
	using glm::vec3::vec3;
public:
	template <typename Archive> void Serialize(Archive& archive)
	{
		archive(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
	}
	glm::vec3 GetGlmType() const
	{
		return static_cast<glm::vec3>(*this);
	}
};

class Vec4f : public glm::vec4
{
	using glm::vec4::vec4;
public:
	template <typename Archive> void Serialize(Archive& archive)
	{
		archive(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z), CEREAL_NVP(w));
	}
	glm::vec4 GetGlmType() const
	{
		return static_cast<glm::vec4>(*this);
	}
};

class Quatf : public glm::quat
{
	using glm::quat::quat;
public:
	template <typename Archive> void Serialize(Archive& archive)
	{
		archive(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z), CEREAL_NVP(w));
	}
};

class Mat4f : public glm::mat4
{
	using glm::mat4::mat4;
public:
	template <typename Archive> void Save(Archive& archive) const
	{
		for (int i = 0; i < 4; i++)
			archive(cereal::make_nvp("row" + std::to_string(i), Vec4f((*this)[i])));
	}
	template <typename Archive> void Load(Archive& archive)
	{
		for (int i = 0; i < 4; i++)
		{
			Vec4f row;
			archive(cereal::make_nvp("row" + std::to_string(i), row));
			(*this)[i] = row;
		}

	}
};

namespace Math = glm;


class Transform
{
	bool KUPA;
	Transform* ParentTransform;
	mutable std::unique_ptr<Transform> WorldTransformCache;
	mutable glm::mat4 WorldTransformMatrixCache;
	mutable glm::mat4 MatrixCache;

	std::vector <Transform*> Children;

	Vec3f Position;
	Quatf Rotation;
	Vec3f Scale;

	mutable std::vector <bool> DirtyFlags;
	mutable bool Empty;	//true if the Transform object has never been changed. Allows for a simple optimization - we skip it during world transform calculation
public:
	std::vector <std::shared_ptr<InterpolatorBase>> Interpolators;

	void FlagMyDirtiness() const;
	void FlagWorldDirtiness() const;

public:
	const Vec3f& PositionRef;		//read-only for optimization purposes
	const Quatf& RotationRef;
	const Vec3f& ScaleRef;

	explicit Transform();
	explicit Transform(const glm::vec2& pos, const glm::vec2& scale = glm::vec2(1.0f), const glm::quat& rot = glm::quat(glm::vec3(0.0f)));
	explicit Transform(const glm::vec3& pos, const glm::quat& rot = glm::quat(glm::vec3(0.0f)), const glm::vec3& scale = glm::vec3(1.0f));
	Transform(const Transform&);
	Transform(Transform&&) noexcept;

	bool IsEmpty() const;

	glm::vec3 Transform::GetFrontVec() const;
	Transform GetInverse() const;
	glm::mat3 GetRotationMatrix(float = 1.0f) const; //this argument is used as a multiplier for the rotation vector - you can pass -1.0f to get the inverse matrix for a low cost
	glm::mat4 GetMatrix() const; //returns model matrix
	glm::mat4 GetViewMatrix() const; //returns view/eye/camera matrix
	const Transform& GetWorldTransform() const; //calculates the world transform (transform data is stored in local space)
	const glm::mat4& GetWorldTransformMatrix() const; //calls this->GetWorldTransform().GetMatrix() and then stores the matrix in cache - you should call this method instead of 2 seperate ones (for sweet sweet FPS)
	Transform* GetParentTransform() const;

	void SetPosition(const glm::vec2&);
	void SetPosition(const glm::vec3&);
	void SetPositionWorld(const glm::vec3&);
	void Move(const glm::vec2&);
	void Move(const glm::vec3&);
	void SetRotation(const glm::vec3& euler);
	void SetRotation(const glm::quat& quat);
	void SetRotationWorld(const glm::quat& quat);
	void Rotate(const glm::vec3& euler);
	void Rotate(const glm::quat& quat);
	void SetScale(const glm::vec2&);
	void SetScale(const glm::vec3&);
	void ApplyScale(float);
	void ApplyScale(const glm::vec2&);
	void ApplyScale(const glm::vec3&);
	void Set(int, const glm::vec3&);
	template <TVec vec, VecAxis axis> void SetVecAxis(float val)
	{
		unsigned int axisIndex = static_cast<unsigned int>(axis);
		assert(axisIndex <= 2 || (axis == TVec::ROTATION && axisIndex == 3));

		switch (vec)
		{
		case TVec::POSITION: Position[axisIndex] = val; break;
		case TVec::ROTATION: Rotation[axisIndex] = val; break;
		case TVec::ROTATION_EULER: glm::vec3 euler = toEuler(RotationRef); euler[axisIndex] = val; SetRotation(euler); break;
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

	void AddInterpolator(std::string fieldName, std::shared_ptr<InterpolatorBase>, bool animateFromCurrent = true);	//if animateFromCurrent is true, the method automatically changes the minimum value of the interpolator to be the current value of the interpolated variable.
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

glm::quat quatFromDirectionVec(const glm::vec3& dirVec, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));
/*const Transform& operator*(const glm::mat4& mat, const Transform& t)
{
	glm::vec3 scale(0.0f);
	glm::scale(mat, scale);
	return Transform(mat * glm::vec4(t.PositionRef, 1.0f), (mat) * t.RotationRef, scale * t.ScaleRef);
}*/

Transform decompose(const glm::mat4&);