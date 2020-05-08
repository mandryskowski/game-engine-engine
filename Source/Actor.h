#pragma once
#include "SearchEngine.h"
#include <iostream>

class Actor
{
protected:
	Component* RootComponent;
	std::vector <Actor*> Children;
	std::string Name;
	std::stringstream* LoadStream;

public:
	Actor(std::string, bool createRoot = true);
	Component* GetRoot();
	std::string GetName();
	Transform* GetTransform();
	Actor* GetChild(unsigned int);

	void ReplaceRoot(Component*);
	void SetTransform(Transform);
	void AddComponent(Component*);
	void AddChild(Actor*);
	void SetLoadStream(std::stringstream* stream);

	virtual void LoadDataFromLevel(SearchEngine* searcher);

	virtual void HandleInputs(GLFWwindow*, float);
	void HandleInputsAll(GLFWwindow*, float);
	virtual void Update(float);
	void UpdateAll(float);

	Actor* SearchForActor(std::string name);
};