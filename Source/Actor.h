#pragma once
#include "GameManager.h"
#include <iostream>

struct GLFWwindow;


class Actor
{
protected:
	Component* RootComponent;
	std::vector <std::shared_ptr<Actor>> Children;
	std::string Name;
	std::stringstream* SetupStream;
	GameScene* Scene;
	GameManager* GameHandle;

public:
	Actor(GameScene*, const std::string& name);

	virtual void OnStart();
	virtual void OnStartAll();

	Component* GetRoot() const;
	std::string GetName() const;
	Transform* GetTransform() const;
	Actor* GetChild(unsigned int) const;

	void DebugHierarchy(int nrTabs = 0);

	void ReplaceRoot(Component*);
	void SetTransform(Transform);
	void AddComponent(Component*);
	void AddChild(std::shared_ptr<Actor>);
	void SetSetupStream(std::stringstream* stream);

	virtual void Setup();

	virtual void HandleInputs(GLFWwindow*);
	void HandleInputsAll(GLFWwindow*);
	virtual void Update(float);
	void UpdateAll(float);
	virtual void DebugRender(RenderInfo info, Shader* shader) const; //this method should only be called to render the components as something (usually a textured billboard) to debug the project.
	void DebugRenderAll(RenderInfo info, Shader* shader) const;

	Actor* FindActor(std::string name);
	const Actor* FindActor(std::string name) const;
};