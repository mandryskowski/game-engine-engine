#include "Actor.h"
#include "Component.h"
#include "Mesh.h"
#include "RenderInfo.h"
#include <vector>

Actor::Actor(GameScene* scene, const std::string& name):
	Scene(scene),
	GameHandle(scene->GetGameHandle())
{
	RootComponent = new Component(scene, name + "'s root", Transform());
	Name = name;
	SetupStream = nullptr;
}

void Actor::OnStart()
{
	RootComponent->OnStartAll();
}

void Actor::OnStartAll()
{
	OnStart();
	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->OnStartAll();
}

Component* Actor::GetRoot() const
{
	return RootComponent;
}

std::string Actor::GetName() const
{
	return Name;
}

Transform* Actor::GetTransform() const
{
	return &RootComponent->GetTransform();
}

Actor* Actor::GetChild(unsigned int index) const
{
	return Children[index].get();
}

void Actor::DebugHierarchy(int nrTabs)
{
	RootComponent->DebugHierarchy(nrTabs);
	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->DebugHierarchy(nrTabs + 1);
}

void Actor::ReplaceRoot(Component* component)
{
	component->AddComponents(RootComponent->GetChildren());
	component->GetTransform().SetParentTransform(RootComponent->GetTransform().GetParentTransform());
	delete RootComponent;
	RootComponent = component;
}

void Actor::SetTransform(Transform transform)
{
	RootComponent->SetTransform(transform);
}

void Actor::AddComponent(Component* component)
{
	RootComponent->AddComponent(component);
}

void Actor::AddChild(std::shared_ptr<Actor> child)
{
	Children.push_back(child);
	child->GetTransform()->SetParentTransform(&RootComponent->GetTransform());
}

void Actor::SetSetupStream(std::stringstream* stream)
{
	SetupStream = stream;
	std::cout << "Adding " << SetupStream << " to actor " + Name + ".\n";
}

void Actor::Setup()
{
	if (SetupStream)
	{
		delete SetupStream;
		SetupStream = nullptr;
	}

	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->Setup();
}

void Actor::HandleInputs(GLFWwindow* window)
{
	RootComponent->HandleInputsAll(window);
}

void Actor::HandleInputsAll(GLFWwindow* window)
{
	HandleInputs(window);
	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->HandleInputsAll(window);
}

void Actor::Update(float deltaTime)
{
	RootComponent->UpdateAll(deltaTime);
	if (Name == "CubeActor")
		;//RootComponent->GetTransform().SetScale(glm::vec3(1.0f + glfwGetTime() * 0.05f));
}

void Actor::UpdateAll(float deltaTime)
{
	Update(deltaTime);
	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->UpdateAll(deltaTime);
}

void Actor::DebugRender(RenderInfo info, Shader* shader) const
{
	RootComponent->DebugRenderAll(info, shader);
}

void Actor::DebugRenderAll(RenderInfo info, Shader* shader) const
{
	DebugRender(info, shader);
	for (int i = 0; i < static_cast<int>(Children.size()); i++)
		Children[i]->DebugRenderAll(info, shader);
}

Actor* Actor::FindActor(std::string name)
{
	if (Name == name)
		return this;

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
	{
		Actor* found = Children[i]->FindActor(name);
		if (found)
			return found;
	}

	return nullptr;
}

const Actor* Actor::FindActor(std::string name) const
{
	if (Name == name)
		return this;

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
	{
		Actor* found = Children[i]->FindActor(name);
		if (found)
			return found;
	}

	return nullptr;
}