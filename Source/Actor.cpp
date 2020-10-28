#include "Actor.h"
#include "Component.h"
#include <vector>

Actor::Actor(GameManager* gameHandle, std::string name)
{
	GameHandle = gameHandle;
	RootComponent = new Component(GameHandle, "root", Transform());
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

Component* Actor::GetRoot()
{
	return RootComponent;
}

std::string Actor::GetName()
{
	return Name;
}

Transform* Actor::GetTransform()
{
	return &RootComponent->GetTransform();
}

Actor* Actor::GetChild(unsigned int index)
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

void Actor::Setup(SearchEngine* searcher)
{
	if (SetupStream)
	{
		delete SetupStream;
		SetupStream = nullptr;
	}

	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->Setup(searcher);
}

void Actor::HandleInputs(GLFWwindow* window, float deltaTime)
{
	RootComponent->HandleInputsAll(window, deltaTime);
}

void Actor::HandleInputsAll(GLFWwindow* window, float deltaTime)
{
	HandleInputs(window, deltaTime);
	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->HandleInputsAll(window, deltaTime);
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

Actor* Actor::SearchForActor(std::string name)
{
	if (Name == name)
		return this;

	for (int i = 0; i < static_cast<int>(Children.size()); i++)
	{
		Actor* found = Children[i]->SearchForActor(name);
		if (found)
			return found;
	}

	return nullptr;
}