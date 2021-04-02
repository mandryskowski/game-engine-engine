#pragma once
#include <animation/Animation.h>
#include <glfw/glfw3.h>
#include <glm/gtx/euler_angles.hpp>
#include <string>

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

class Transform
{
	bool KUPA;
	Transform* ParentTransform;
	mutable std::unique_ptr<Transform> WorldTransformCache;
	mutable glm::mat4 WorldTransformMatrixCache;
	mutable glm::mat4 MatrixCache;

	std::vector <Transform*> Children;

	glm::vec3 Position;
	glm::quat Rotation;
	glm::vec3 Scale;

	mutable std::vector <bool> DirtyFlags;
	mutable bool Empty;	//true if the Transform object has never been changed. Allows for a simple optimization - we skip it during world transform calculation
public:
	std::vector <std::shared_ptr<InterpolatorBase>> Interpolators;

	void FlagMyDirtiness() const;
	void FlagWorldDirtiness() const;

public:
	const glm::vec3& PositionRef;		//read-only for optimization purposes
	const glm::quat& RotationRef;
	const glm::vec3& ScaleRef;

	explicit Transform();
	explicit Transform(glm::vec2 pos, glm::vec2 scale = glm::vec2(1.0f), glm::quat rot = glm::quat(glm::vec3(0.0f)));
	explicit Transform(glm::vec3 pos, glm::quat rot = glm::quat(glm::vec3(0.0f)), glm::vec3 scale = glm::vec3(1.0f));
	Transform(const Transform&);
	Transform(Transform&&) noexcept;

	glm::vec3 Transform::GetFrontVec() const;
	Transform GetInverse() const;
	glm::mat3 GetRotationMatrix(float = 1.0f) const; //this argument is used as a multiplier for the rotation vector - you can pass -1.0f to get the inverse matrix for a low cost
	glm::mat4 GetMatrix() const; //returns model matrix
	glm::mat4 GetViewMatrix() const; //returns view/eye/camera matrix
	const Transform& GetWorldTransform() const; //calculates the world transform (transform data is stored in local space)
	const glm::mat4& GetWorldTransformMatrix() const; //calls this->GetWorldTransform().GetMatrix() and then stores the matrix in cache - you should call this method instead of 2 seperate ones (for sweet sweet FPS)
	Transform* GetParentTransform() const;

	void SetPosition(glm::vec2);
	void SetPosition(glm::vec3);
	void SetPositionWorld(glm::vec3);
	void Move(glm::vec2);
	void Move(glm::vec3);
	void SetRotation(glm::vec3 euler);
	void SetRotation(glm::quat quat);
	void SetRotationWorld(glm::quat quat);
	void Rotate(glm::vec3 euler);
	void Rotate(glm::quat quat);
	void SetScale(glm::vec2);
	void SetScale(glm::vec3);
	void Set(int, glm::vec3);
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

	void Print(std::string name = "unnamed") const;
	
	~Transform() = default;

	Transform operator*(const Transform&) const;

	Transform& operator=(const Transform&);
	Transform& operator=(Transform&&) noexcept;

	Transform& operator*=(const Transform&);
	Transform& operator*=(Transform&&);
};

/*const Transform& operator*(const glm::mat4& mat, const Transform& t)
{
	glm::vec3 scale(0.0f);
	glm::scale(mat, scale);
	return Transform(mat * glm::vec4(t.PositionRef, 1.0f), (mat) * t.RotationRef, scale * t.ScaleRef);
}*/

Transform decompose(const glm::mat4&);