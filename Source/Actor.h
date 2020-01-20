#pragma once
#include "Component.h"
#include <iostream>

class Actor
{
	Component* RootComponent;
	std::vector <Actor*> Children;
	std::string Name;

public:
	Actor(std::string);
	Component* GetRoot();
	std::string GetName();
	Transform* GetTransform();
	Actor* GetChild(unsigned int);
	void ReplaceRoot(Component*);
	void SetTransform(Transform);
	void AddComponent(Component*);
	void AddChild(Actor*);
	void HandleInputs(GLFWwindow*, float);
	void HandleInputsAll(GLFWwindow*, float);
	void Update(float);
	void UpdateAll(float);
};