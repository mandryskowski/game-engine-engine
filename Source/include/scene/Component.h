#pragma once
#include <game/GameManager.h>
#include <math/Transform.h>
#include <game/GameScene.h>
#include <editor/EditorManager.h>

enum class EditorElementRepresentation
{
	INPUT_BOX,
	TICK_BOX,
	SLIDER,
	PALETTE
};

struct EditorElementDesc
{

};

class Component
{
public:
	Component(GameScene&, const std::string& name = "undefined", const Transform& t = Transform());
	Component(GameScene&, const std::string& name = "undefined", glm::vec3 pos = glm::vec3(0.0f), glm::vec3 rot = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 scale = glm::vec3(1.0f));

	Component(const Component&) = delete;
	Component(Component&&);

protected:
	friend class HierarchyTemplate::ComponentTemplate<Component>;
	Component& operator=(const Component&);
public:
	Component& operator=(Component&&);

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

	const std::string& GetName() const;
	Transform& GetTransform();
	const Transform& GetTransform() const;
	std::vector<Component*> GetChildren();
	GameScene& GetScene() const;

	virtual void GenerateFromNode(const MeshSystem::TemplateNode&, Material* overrideMaterial = nullptr);

	void SetName(std::string name);
	void SetTransform(Transform transform);
	CollisionObject* SetCollisionObject(std::unique_ptr<CollisionObject>&);
	void AddComponent(std::unique_ptr<Component> component);
	void AddComponents(std::vector<std::unique_ptr<Component>> components);
	template <typename ChildClass> ChildClass& CreateComponent(ChildClass&& objectConstructor);

	virtual void Update(float);
	void UpdateAll(float dt);

	virtual void QueueAnimation(Animation*);
	void QueueAnimationAll(Animation*);

	virtual void QueueKeyFrame(AnimationChannel&);
	void QueueKeyFrameAll(AnimationChannel&);

	virtual void DebugRender(RenderInfo info, Shader* shader) const; //this method should only be called to render the component as something (usually a textured billboard) to debug the project.
	void DebugRenderAll(RenderInfo info, Shader* shader) const;

	virtual AtlasMaterial& LoadDebugRenderMaterial(const std::string& materialName, const std::string& path);
	virtual MaterialInstance GetDebugMatInst(EditorIconState);

	Component* SearchForComponent(std::string name);
	template <class CompClass = Component> CompClass* GetComponent(const std::string& name)
	{
		if (Name == name)
			return dynamic_cast<CompClass*>(this);

		for (const auto& it : Children)
			if (auto found = it->GetComponent<CompClass>(name))
				return found;

		return nullptr;
	}
	template<class CompClass> void GetAllComponents(std::vector <CompClass*>* comps)	//this function returns every element further in the hierarchy tree (kids, kids' kids, ...) that is of CompClass type
	{
		if (!comps)
			return;
		for (unsigned int i = 0; i < Children.size(); i++)
		{
			CompClass* c = dynamic_cast<CompClass*>(Children[i].get());
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
			CastToClass* c = dynamic_cast<CastToClass*>(dynamic_cast<CheckClass*>(Children[i].get()));	//we're casting to CheckClass to check if our child is of the required type; we then cast to CastToClass to get a pointer of this type
			if (c)
				comps->push_back(c);
		}
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->GetAllComponents<CastToClass, CheckClass>(comps);	//robimy to samo we wszystkich "dzieciach"
	}

	virtual void GetEditorDescription(UIActor& editorParent, GameScene& editorScene);

	virtual ~Component();

public:
	friend class Actor;
	std::unique_ptr<Component> DetachChild(Component& soughtChild);	//Find child in hierarchy and detach it from its parent
	void MoveChildren(Component& moveTo);

	std::string Name;

	Transform ComponentTransform;
	std::vector <std::unique_ptr<Component>> Children;
	std::unique_ptr<CollisionObject> CollisionObj;

	GameScene& Scene; //The scene that this Component is present in
	GameManager* GameHandle;

	AtlasMaterial* DebugRenderMat;
	std::shared_ptr<MaterialInstance> DebugRenderMatInst;
	mutable glm::mat4 DebugRenderLastFrameMVP;
};

template<typename ChildClass>
inline ChildClass& Component::CreateComponent(ChildClass&& objectCRef)
{
	std::unique_ptr<ChildClass> createdChild = std::make_unique<ChildClass>(std::move(objectCRef));
	ChildClass& childRef = *createdChild;
	AddComponent(std::move(createdChild));

	return (ChildClass&)childRef;
}