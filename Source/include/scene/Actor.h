#pragma once
#include <game/GameManager.h>
#include <input/Event.h>
#include <iostream>
#include <map>

struct GLFWwindow;

template <typename SavedClass> class ClassSaveInfo
{
	template <typename SavedVar> void RegisterVar(SavedVar& var)
	{
		
	}
};

class Actor
{
public:
	Actor(GameScene&, const std::string& name);
	Actor(const Actor&) = delete;
	Actor(Actor&&);

	virtual void OnStart();
	virtual void OnStartAll();

	Component* GetRoot() const;
	std::string GetName() const;
	Transform* GetTransform() const;
	Actor* GetChild(unsigned int) const;
	std::vector<Actor*> GetChildren();
	GameScene& GetScene();
	bool IsBeingKilled();

	void SetName(const std::string&);

	void DebugHierarchy(int nrTabs = 0);
	void DebugActorHierarchy(int nrTabs = 0);

	void ReplaceRoot(std::unique_ptr<Component>);
	void MoveCompToRoot(Component&);
	void SetTransform(Transform);
	void AddComponent(std::unique_ptr<Component>);
	virtual Actor& AddChild(std::unique_ptr<Actor>);
	template <class T> T& CreateChild(T&&);
	template <class T> T& CreateComponent(T&&);	//Adds a new component that is a child of RootComponent.
	template <class T> T& CreateComponent(T&&, Component* parent);	//Adds a new component that is a child of parent. Pass nullptr as the second argument to make this created component the new RootComponent.
	void SetSetupStream(std::stringstream* stream);

	virtual void Setup();

	virtual void HandleEvent(const Event& ev) {}
	void HandleEventAll(const Event& ev);

	virtual void Update(float);
	void UpdateAll(float);

	void MarkAsKilled();

	void Delete();

	virtual void DebugRender(RenderInfo info, Shader* shader) const; //this method should only be called to render the components as something (usually a textured billboard) to debug the project.
	void DebugRenderAll(RenderInfo info, Shader* shader) const;

	Actor* FindActor(const std::string& name);
	const Actor* FindActor(const std::string& name) const;

	virtual void GetEditorDescription(UIActor& canvas, GameScene& editorScene);

	template<class ActorClass> void GetAllChildren(std::vector <ActorClass*>* comps)	//this function returns every element further in the hierarchy tree (kids, kids' kids, ...) that is of ActorClass type
	{
		if (!comps)
			return;
		for (unsigned int i = 0; i < Children.size(); i++)
		{
			ActorClass* c = dynamic_cast<ActorClass*>(Children[i].get());
			if (c)					//if dynamic cast was succesful - this children is of ActorClass type
				comps->push_back(c);
		}
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->GetAllChildren<ActorClass>(comps);	//do it reccurently in every child
	}

protected:
	std::unique_ptr<Component> RootComponent;
	std::vector<std::unique_ptr<Actor>> Children;
	Actor* ParentActor;
	std::string Name;
	std::stringstream* SetupStream;
	GameScene& Scene;
	GameManager* GameHandle;

	bool bKillingProcessStarted;

	friend std::ostream& operator<<(std::ostream& os, const Actor& actor);
};

/*std::ostream& operator<<(std::ostream& os, const Actor& actor)
{
	os << "newactor Actor " << actor.Name;
	//os << *RootComponent;
	for (auto& it : actor.Children)
		os << *it;

	return os;
}*/

class ActorFactory
{
	typedef std::unique_ptr<Actor>(*ActorCreateFunc)(void);
	typedef std::map<std::string, ActorCreateFunc> FactoryMap;
	FactoryMap FuncMap;
public:
	ActorFactory()
	{
		//Register("Actor", Actor::Get);
	}
	static ActorFactory& Get()
	{
		static ActorFactory factory;
		return factory;
	}
	std::unique_ptr<Actor> Create(const std::string& tName)
	{
		FactoryMap::iterator it = FuncMap.find(tName);
		if (it != FuncMap.end())
			return (*it->second)();
		return nullptr;
	}
	void Register(std::string name, ActorCreateFunc func)
	{
		FuncMap[name] = func;
	}
private:
};

template<class T>
inline T& Actor::CreateChild(T&& actorCRef)
{
	std::unique_ptr<T> createdChild = std::make_unique<T>(std::move(actorCRef));
	T& childRef = *createdChild.get();
	AddChild(std::move(createdChild));

	if (childRef.GameHandle->HasStarted())
		childRef.OnStartAll();

	return childRef;
}

template<class T>
inline T& Actor::CreateComponent(T&& compCRef)
{
	T& comp = RootComponent->CreateComponent(std::move(compCRef));
	if (comp.GameHandle->HasStarted())
		comp.OnStartAll();
	return comp;
}
