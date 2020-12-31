#pragma once
#include "GameManager.h"
#include "Transform.h"
#include "GameScene.h"

class Component
{
protected:
	std::string Name;

	Transform ComponentTransform;
	std::vector <Component*> Children;
	std::unique_ptr<CollisionObject> CollisionObj;

	GameScene* Scene; //The scene that this Component is present in
	GameManager* GameHandle;

	Material* DebugRenderMaterial;
	mutable glm::mat4 DebugRenderLastFrameMVP;

public:
	Component(GameScene*, const std::string& name = "undefined", const Transform& t = Transform());
	Component(GameScene*, const std::string& name = "undefined", glm::vec3 pos = glm::vec3(0.0f), glm::vec3 rot = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 scale = glm::vec3(1.0f));

	Component(const Component&) = delete;
	Component(Component&&) = delete;

	virtual void OnStart();
	virtual void OnStartAll();

	void DebugHierarchy(int nrTabs = 0)
	{
		std::string tabs;
		for (int i = 0; i < nrTabs; i++)	tabs += "	";
		std::cout << tabs + "-" + Name + "\n";

		for (int i = 0; i < static_cast<int>(Children.size()); i++)
			Children[i]->DebugHierarchy(nrTabs + 1);
	}

	std::string GetName() const;
	Transform& GetTransform();
	const Transform& GetTransform() const;
	std::vector<Component*> GetChildren();
	GameScene* GetScene() const;

	virtual void GenerateFromNode(const MeshSystem::TemplateNode*, Material* overrideMaterial = nullptr);

	void SetName(std::string name);
	void SetTransform(Transform transform);
	CollisionObject* SetCollisionObject(std::unique_ptr<CollisionObject>&);
	void AddComponent(Component* component);
	void AddComponents(std::vector<Component*> components);

	virtual void Update(float);
	void UpdateAll(float dt);

	virtual void QueueAnimation(Animation*);
	void QueueAnimationAll(Animation*);

	virtual void DebugRender(RenderInfo info, Shader* shader) const; //this method should only be called to render the component as something (usually a textured billboard) to debug the project.
	void DebugRenderAll(RenderInfo info, Shader* shader) const;

	void LoadDebugRenderMaterial(GameManager* gameHandle, std::string materialName, std::string path);

	Component* SearchForComponent(std::string name);
	template <class CompClass = Component> CompClass* GetComponent(const std::string& name)
	{
		auto found = std::find_if(Children.begin(), Children.end(), [name](Component* child) { return child->GetName() == name; });
		if (found != Children.end())
			return dynamic_cast<CompClass*>(*found);

		return nullptr;
	}
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