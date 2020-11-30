#pragma once
#include "Component.h"
#include "GameSettings.h"
#include <array>

class CameraComponent: public Component
{
	glm::vec3 RotationEuler;

	
	glm::mat4 Projection;	

public:
	CameraComponent(GameScene*, std::string name, const glm::mat4& projectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f));
	glm::mat4 GetProjectionMat();		//returns the camera projection matrix - i've decided that each camera should have their own projection mat, because just as in the real world each cam has its own FoV and aspect ratio
	RenderInfo GetRenderInfo(RenderToolboxCollection& renderCollection);
	void RotateWithMouse(glm::vec2);	//rotates camera - you should pass the mouse offset from the center
	virtual void HandleInputs(GLFWwindow*);	//checks for keyboard input
	virtual void Update(float);		//controls the component
};