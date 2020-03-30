#pragma once
#include "Utility.h"
#include <glfw/glfw3.h>
#include <glm/gtx/euler_angles.hpp>
#include <string>


class Transform
{
	Transform* ParentTransform;
	Transform* WorldTransformCache;
	glm::mat4 WorldTransformMatrixCache;

	std::vector <Transform*> Children;

	glm::vec3 Position;
	glm::vec3 Rotation;
	glm::vec3 Scale;
	glm::vec3 Front;

	std::vector <bool> DirtyFlags;

	static float tSum;
	
	void FlagChildrenAsDirty();

public:
	const glm::vec3& PositionRef;
	const glm::vec3& RotationRef;		//read-only for optimization purposes
	const glm::vec3& ScaleRef;
	const glm::vec3& FrontRef;

	bool bConstrain;

	Transform(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 rot = glm::vec3(0.0f), glm::vec3 scale = glm::vec3(1.0f), glm::vec3 front = glm::vec3(0.0f), bool constrain = false);
	Transform(const Transform&);
	Transform(Transform&&) noexcept = default;
	static float test()
	{
		return tSum;
	}

	glm::mat3 GetRotationMatrix(float = 1.0f) const; //this argument is used as a multiplier for the rotation vector - you can pass -1.0f to get the inverse matrix for a low cost
	glm::mat3 GetRotationMatrixWithOffset(glm::vec3 = glm::vec3(0.0f), float = 1.0f) const;
	glm::mat4 GetMatrix() const; //returns model matrix
	glm::mat4 GetViewMatrix() const; //returns view/eye/camera matrix
	const Transform& GetWorldTransform(); //calculates the world transform (transform data is stored in local space)
	const glm::mat4& GetWorldTransformMatrix(); //calls this->GetWorldTransform().GetMatrix() and then stores the matrix in cache - you should call this method instead of 2 seperate ones (for sweet sweet FPS)
	Transform* GetParentTransform() const;

	void SetPosition(glm::vec3);
	void Move(glm::vec3);
	void SetRotation(glm::vec3);
	void Rotate(glm::vec3);
	void SetScale(glm::vec3);
	void SetFront(glm::vec3);

	void SetParentTransform(Transform* transform, bool = false); //if the bool is true, the transform is recalculated to the parent's space
	void AddChild(Transform*);
	void RemoveChild(Transform*);

	bool GetDirtyFlag(unsigned int index = 0);
	void SetDirtyFlag(unsigned int index = 0, bool val = true);
	void SetDirtyFlags(bool val = true);
	unsigned int AddDirtyFlag();
	
	~Transform() = default;

	Transform& operator=(const Transform&) = default;
	Transform& operator=(Transform&&) noexcept = default;
	glm::vec3& operator [](unsigned int);
};

glm::mat3 ModelToNormal(glm::mat4);