#include "Component.h"

Component::Component(std::string name, Transform t):
	Name(name), ComponentTransform(t)
{
}
Component::Component(std::string name, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale):
	Name(name), ComponentTransform(pos, rot, scale)
{
}
std::string Component::GetName()
{
	return Name;
}
Transform* Component::GetTransform()
{
	return &ComponentTransform;
}
std::vector<Component*> Component::GetChildren()
{
	return Children;
}
void Component::SetName(std::string name)
{
	Name = name;
}
void Component::SetTransform(Transform transform)
{ 
	//don't change the ParentTransform pointer; it's not needed in this case
	ComponentTransform.SetPosition(transform.PositionRef);
	ComponentTransform.SetRotation(transform.RotationRef);
	ComponentTransform.SetScale(transform.ScaleRef);
	ComponentTransform.SetFront(transform.FrontRef);
	ComponentTransform.bConstrain = transform.bConstrain;
}
void Component::AddComponent(Component* component)
{
	component->GetTransform()->SetParentTransform(&this->ComponentTransform); Children.push_back(component);
}
void Component::AddComponents(std::vector<Component*> components)
{
	for (unsigned int i = 0; i < components.size(); i++)
	{
		components[i]->GetTransform()->SetParentTransform(&this->ComponentTransform);
		Children.push_back(components[i]);
	}
}
void Component::HandleInputsAll(GLFWwindow* window, float deltaTime)
{
	HandleInputs(window, deltaTime);
	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->HandleInputsAll(window, deltaTime);
}
void Component::UpdateAll(float dt)
{
	Update(dt);
	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->Update(dt);
}
Component* Component::SearchForComponent(std::string name)
{
	if (Name == name)
		return this;

	for (unsigned int i = 0; i < Children.size(); i++)
	{
		Component* childSearch = Children[i]->SearchForComponent(name);
		if (childSearch)
			return childSearch;
	}
	return nullptr;
}

Component::~Component()
{
	for (unsigned int i = 0; i < Children.size(); i++)
		Children[i]->GetTransform()->SetParentTransform(nullptr);
}