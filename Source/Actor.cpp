#include "Actor.h"

Actor::Actor(std::string name)
{
	RootComponent = new Component("root", Transform());
	Name = name;
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
	return RootComponent->GetTransform();
}

Actor* Actor::GetChild(unsigned int index)
{
	return Children[index];
}

void Actor::ReplaceRoot(Component* component)
{
	component->AddComponents(RootComponent->GetChildren());
	component->GetTransform()->SetParentTransform(RootComponent->GetTransform()->GetParentTransform());
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
void Actor::AddChild(Actor* child)
{
	Children.push_back(child);
	child->GetTransform()->SetParentTransform(RootComponent->GetTransform());
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
}

void Actor::UpdateAll(float deltaTime)
{
	Update(deltaTime);
	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->UpdateAll(deltaTime);
}