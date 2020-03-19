#pragma once
#include "CollisionEngine.h"

class CameraComponent: public Component
{
	CollisionEngine* CollisionEng;

	glm::vec3 VelocityPerSec;
	float SpeedPerSec;

	CollisionComponent* GroundCheckComponent;
	float HoverHeight;
	bool bHoverHeightUnlocked;
	
	glm::mat4 Projection;	

public:
	CameraComponent(std::string name, glm::uvec2, CollisionEngine*, float = 3.0f, Transform = Transform());
	bool PerformCollisionCheck(std::vector<CollisionComponent*>, glm::vec3, std::vector<glm::vec3>*, std::vector<CollisionComponent*>* = nullptr);
	glm::vec3 ApplyCollisionResponse(glm::vec3);
	glm::mat4 GetProjectionMat();		//returns the camera projection matrix - i've decided that each camera should have their own projection mat, because just as in the real world each cam has its own FoV and aspect ratio
	glm::mat4 GetVP(Transform* = nullptr);	//you can pass a WorldTransform of this component if it was calculated before (you can save on calculations
	void RotateWithMouse(glm::vec2);	//rotates camera - you should pass the mouse offset from the center
	virtual void HandleInputs(GLFWwindow*, float);	//checks for keyboard input
	virtual void Update(float);		//controls the component
};