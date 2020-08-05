#pragma once
#include "Animation.h"
#include <glfw/glfw3.h>
#include <glm/gtx/euler_angles.hpp>
#include <string>

struct PhysicsInput
{
	glm::vec3 Position;
	glm::quat Rotation;
};

class Transform
{
	friend class CameraComponent;
	friend class LightComponent;
	friend class RenderEngine;
	bool KUPA;
	Transform* ParentTransform;
	mutable std::unique_ptr<Transform> WorldTransformCache;
	mutable glm::mat4 WorldTransformMatrixCache;
	mutable glm::mat4 MatrixCache;

	std::vector <Transform*> Children;

	glm::vec3 Position;
	glm::quat Rotation;
	glm::vec3 Scale;
	glm::vec3 Front;

	mutable std::vector <bool> DirtyFlags;
	mutable bool Empty;	//true if the Transform object has never been changed. Allows for a simple optimization - we skip it during world transform calculation

	std::vector <std::shared_ptr<InterpolatorBase>> Interpolators;

	static float tSum;

	void FlagMyDirtiness();
	void FlagWorldDirtiness();

public:
	const glm::vec3& PositionRef;		//read-only for optimization purposes
	const glm::quat& RotationRef;
	const glm::vec3& ScaleRef;
	const glm::vec3& FrontRef;

	bool bConstrain;
	bool bRotationLock;

	Transform(glm::vec3 pos = glm::vec3(0.0f), glm::quat rot = glm::quat(glm::vec3(0.0f)), glm::vec3 scale = glm::vec3(1.0f), glm::vec3 front = glm::vec3(0.0f), bool constrain = false, bool rotlock = false);
	Transform(const Transform&);
	Transform(Transform&&) noexcept;
	static float test()
	{
		return tSum;
	}

	glm::mat3 GetRotationMatrix(float = 1.0f) const; //this argument is used as a multiplier for the rotation vector - you can pass -1.0f to get the inverse matrix for a low cost
	glm::mat4 GetMatrix() const; //returns model matrix
	glm::mat4 GetViewMatrix() const; //returns view/eye/camera matrix
	const Transform& GetWorldTransform() const; //calculates the world transform (transform data is stored in local space)
	const glm::mat4& GetWorldTransformMatrix() const; //calls this->GetWorldTransform().GetMatrix() and then stores the matrix in cache - you should call this method instead of 2 seperate ones (for sweet sweet FPS)
	Transform* GetParentTransform() const;

	void SetPosition(glm::vec3);
	void SetPositionWorld(glm::vec3);
	void Move(glm::vec3);
	void SetRotation(glm::vec3 euler);
	void SetRotation(glm::quat quat);
	void SetRotationWorld(glm::quat quat);
	void Rotate(glm::vec3 euler);
	void Rotate(glm::quat quat);
	void SetScale(glm::vec3);
	void SetFront(glm::vec3);
	void Set(int, glm::vec3);

	void SetParentTransform(Transform* transform, bool relocate = false); //if the bool is true, the transform is recalculated to the parent's space
	void AddChild(Transform*);
	void RemoveChild(Transform*);

	bool GetDirtyFlag(unsigned int index = 0);
	void SetDirtyFlag(unsigned int index = 0, bool val = true);
	void SetDirtyFlags(bool val = true);
	unsigned int AddDirtyFlag();

	void AddInterpolator(std::string fieldName, std::shared_ptr<InterpolatorBase>, bool animateFromCurrent = true);	//if animateFromCurrent is true, the method automatically changes the minimum value of the interpolator to be the current value of the interpolated variable.
	template <class T> void AddInterpolator(std::string fieldName, float begin, float end, T min, T max, InterpolationType interpType, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP);
	template <class T> void AddInterpolator(std::string fieldName, float begin, float end, T max, InterpolationType interpType, bool fadeAway = false, AnimBehaviour before = AnimBehaviour::STOP, AnimBehaviour after = AnimBehaviour::STOP);
	void Update(float deltaTime);
	
	~Transform() = default;

	Transform& operator*(const Transform&);
	Transform& operator*(Transform&&);

	Transform& operator=(const Transform&);
	Transform& operator=(Transform&&) noexcept;

	Transform& operator*=(const Transform&);
	Transform& operator*=(Transform&&);
};