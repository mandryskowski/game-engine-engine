#pragma once
#include "SearchEngine.h"
#include <iostream>

class Actor
{
protected:
	Component* RootComponent;
	std::vector <std::shared_ptr<Actor>> Children;
	std::string Name;
	std::stringstream* SetupStream;
	GameManager* GameHandle;

public:
	Actor(GameManager*, std::string);
	Component* GetRoot();
	std::string GetName();
	Transform* GetTransform();
	Actor* GetChild(unsigned int);

	void DebugHierarchy(int nrTabs = 0);

	void ReplaceRoot(Component*);
	void SetTransform(Transform);
	void AddComponent(Component*);
	void AddChild(std::shared_ptr<Actor>);
	void SetSetupStream(std::stringstream* stream);

	virtual void Setup(SearchEngine* searcher);

	virtual void HandleInputs(GLFWwindow*, float);
	void HandleInputsAll(GLFWwindow*, float);
	virtual void Update(float);
	void UpdateAll(float);

	Actor* SearchForActor(std::string name);
};