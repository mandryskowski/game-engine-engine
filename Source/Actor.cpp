#include "Actor.h"

Actor::Actor(std::string name, bool createRoot)
{
	RootComponent = (createRoot) ? (new Component("root", Transform())) : (nullptr);
	Name = name;
	LoadStream = nullptr;
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

void Actor::SetLoadStream(std::stringstream* stream)
{
	LoadStream = stream;
}

void Actor::LoadDataFromLevel(SearchEngine* searcher)
{
	if (LoadStream)
	{
		delete LoadStream;
		LoadStream = nullptr;
	}

	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->LoadDataFromLevel(searcher);
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

Actor* Actor::SearchForActor(std::string name)
{
	if (Name == name)
		return this;

	for (unsigned int i = 0; i < Children.size(); i++)
		return Children[i]->SearchForActor(name);

	return nullptr;
}