#pragma once
#include "Transform.h"

class Component
{
protected:
	std::string Name;
	Transform ComponentTransform;
	std::vector <Component*> Children;

public:
	Component(std::string name = "undefined", Transform t = Transform());
	Component(std::string name = "undefined", glm::vec3 pos = glm::vec3(0.0f), glm::vec3 rot = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 scale = glm::vec3(1.0f));
	std::string GetName();
	Transform* GetTransform();
	std::vector<Component*> GetChildren();
	void SetName(std::string name);
	void SetTransform(Transform transform);
	void AddComponent(Component* component);
	void AddComponents(std::vector<Component*> components);
	virtual void HandleInputs(GLFWwindow*, float) {}
	void HandleInputsAll(GLFWwindow* window, float deltaTime);
	virtual void Update(float) {}
	void UpdateAll(float dt);
	Component* SearchForComponent(std::string name);
	template<class CompClass> void GetAllComponents(std::vector <CompClass*>* comps)	//this function returns every element further in the hierarchy tree (kids, kids' kids, ...) that is of CompClass type
	{
		if (!comps)
			return;
		for (unsigned int i = 0; i < Children.size(); i++)
		{
			CompClass* c = dynamic_cast<CompClass*>(Children[i]);
			if (c)					//if dynamic cast was succesful - this children is of CompCLass type
				comps->push_back(c);
		}
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->GetAllComponents<CompClass>(comps);	//do it reccurently in every child
	}
	template<class CastToClass, class CheckClass> void GetAllComponents(std::vector <CastToClass*>* comps) //this function works like the previous method, but every element must also be of CheckClass type (for ex. CastToClass = CollisionComponent, CheckClass = BSphere)
	{																												  //it's useful when you want to get a vector of CastToClass type pointers to objects, that are also of CheckClass type
		if (!comps)
			return;
		for (unsigned int i = 0; i < Children.size(); i++)
		{
			CastToClass* c = dynamic_cast<CastToClass*>(dynamic_cast<CheckClass*>(Children[i]));	//we're casting to CheckClass to check if our child is of the required type; we then cast to CastToClass to get a pointer of this type
			if (c)
				comps->push_back(c);
		}
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->GetAllComponents<CastToClass, CheckClass>(comps);	//robimy to samo we wszystkich "dzieciach"
	}
	~Component();
};