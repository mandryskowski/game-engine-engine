#pragma once
#include "Utility.h"
#include <glfw/glfw3.h>
#include <glm/gtx/euler_angles.hpp>
#include <string>

class Transform
{
	Transform* ParentTransform;
public:
	glm::vec3 Position;
	glm::vec3 Rotation;
	glm::vec3 Scale;

	glm::vec3 Front;

	bool bConstrain;

	Transform(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 rot = glm::vec3(0.0f), glm::vec3 scale = glm::vec3(1.0f), glm::vec3 front = glm::vec3(0.0f), bool constrain = false);
	glm::mat3 GetRotationMatrix(float = 1.0f) const; //this argument is used as a multiplier for the rotation vector - you can pass -1.0f to get the inverse matrix for a low cost
	glm::mat3 GetRotationMatrixWithOffset(glm::vec3 = glm::vec3(0.0f), float = 1.0f) const;
	glm::mat4 GetMatrix() const; //returns model matrix
	glm::mat4 GetViewMatrix() const; //returns view/eye/camera matrix
	Transform GetWorldTransform() const; //calculates the world transform (transform data is stored in local space)
	Transform* GetParentTransform() const;
	void SetParentTransform(Transform* transform, bool = false); //if the bool is true, the transform is recalculated to the parent's space

	glm::vec3& operator [](unsigned int);
};

glm::mat3 ModelToNormal(glm::mat4);