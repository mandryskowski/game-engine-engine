#pragma once
#include "Transform.h"

class Component
{
protected:
	std::string Name;
	Transform ComponentTransform;
	std::vector <Component*> Children;

public:
	Component(std::string name = "error", Transform t = Transform()) : ComponentTransform(t) { Name = name; }
	Component(std::string name = "error", glm::vec3 pos = glm::vec3(0.0f), glm::vec3 rot = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 scale = glm::vec3(1.0f)) : ComponentTransform(pos, rot, scale) { Name = name; }
	std::string GetName() { return Name; }
	Transform* GetTransform() { return &ComponentTransform; }
	std::vector<Component*> GetChildren() { return Children; }
	void SetName(std::string name) { Name = name; }
	void SetTransform(Transform transform) { transform.SetParentTransform(ComponentTransform.GetParentTransform()); ComponentTransform = transform; }
	void AddComponent(Component* component) { component->GetTransform()->SetParentTransform(&this->ComponentTransform); Children.push_back(component); }
	void AddComponents(std::vector<Component*> components)
	{
		for (unsigned int i = 0; i < components.size(); i++)
		{
			components[i]->GetTransform()->SetParentTransform(&this->ComponentTransform);
			Children.push_back(components[i]);
		}
	}
	virtual void HandleInputs(GLFWwindow*, float) {}
	void HandleInputsAll(GLFWwindow* window, float deltaTime)
	{
		HandleInputs(window, deltaTime);
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->HandleInputsAll(window, deltaTime);
	}
	virtual void Update(float) {}
	void UpdateAll(float dt)
	{
		Update(dt);
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->Update(dt);
	}
	Component* SearchForComponent(std::string name)
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
	template<class CompClass> void GetAllComponents(std::vector <CompClass*>* comps)	//ta funkcja zwraca wszystkie elementy pochodne (dzieci, dzieci dzieci, dzieci dzieci dzieci...) przekazanego typu
	{
		for (unsigned int i = 0; i < Children.size(); i++)
		{
			CompClass* c = dynamic_cast<CompClass*>(Children[i]);		//rzutujemy dynamicznie na przekazany typ CompClass
			if (c)					//jesli rzutowanie sie udalo - sprawdzany obiekt Children[i] jest typu CompClass
				comps->push_back(c);
		}
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->GetAllComponents<CompClass>(comps);	//robimy to samo we wszystkich "dzieciach"
	}
	template<class VectorClass, class CastClass> void GetAllComponents(std::vector <VectorClass*>* comps) //ta funkcja robi to samo co poprzednia, jednak sprawdzane dziecko musi byc ROWNIEZ typu CastClass (CastClass to pochodna VectorClass, np VectorClass = CollisionComponent, CastClass = BSphere
	{																									 //przydaje sie ona kiedy np. chcemy uzyskac wszystkie CollisionComponent ktore sa jednoczesnie np. kulami
		for (unsigned int i = 0; i < Children.size(); i++)
		{
			VectorClass* c = dynamic_cast<VectorClass*>(dynamic_cast<CastClass*>(Children[i]));	//rzutujemy na CastClass, zeby sprawdzic czy nasze dziecko jest tego typu; pozniej rzutujemy na VectorClass aby miec calkowita pewnosc ze CastClass jest tez typu VectorClass
			if (c)																				//tak naprawde nie musimy rzutowac na VectorClass, bo semantyka tej funkcji mowi ze CastClass jest pochodna VectorClass
				comps->push_back(c);
		}
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->GetAllComponents<VectorClass, CastClass>(comps);	//robimy to samo we wszystkich "dzieciach"
	}
	~Component()
	{
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->GetTransform()->SetParentTransform(nullptr);
	}
};
