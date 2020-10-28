#pragma once
#include "Component.h"
#include "GameSettings.h"
#include <array>

class CameraComponent: public Component
{
	enum MovementDir
	{
		FORWARD,
		LEFT,
		RIGHT,
		BACKWARD
	};

	struct MovementAxis
	{
		std::unique_ptr<Interpolator<float>> MovementInterpolator;
		bool Inversed;
		glm::vec3 Direction;
	};

	glm::vec3 VelocityPerSec;
	float SpeedPerSec;
	glm::vec3 RotationEuler;
	std::array<MovementAxis, 4> MovementAxises;

	float HoverHeight;
	bool bHoverHeightUnlocked;
	
	glm::mat4 Projection;	

public:
	CameraComponent(GameManager*, std::string name,float = 3.0f);
	glm::mat4 GetProjectionMat();		//returns the camera projection matrix - i've decided that each camera should have their own projection mat, because just as in the real world each cam has its own FoV and aspect ratio
	glm::mat4 GetVP(Transform* = nullptr);	//you can pass a WorldTransform of this component if it was calculated before (you can save on calculations
	void RotateWithMouse(glm::vec2);	//rotates camera - you should pass the mouse offset from the center
	virtual void HandleInputs(GLFWwindow*, float);	//checks for keyboard input
	void HandleMovementAxis(bool pressed, MovementAxis& axis);
	virtual void Update(float);		//controls the component
};