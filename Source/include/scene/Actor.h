#pragma once
#include <game/GameManager.h>
#include <math/Transform.h>
#include <input/Event.h>
#include <iostream>
#include <map>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>

struct GLFWwindow;
namespace GEE
{

	template <typename SavedClass> class ClassSaveInfo
	{
		template <typename SavedVar>
		static void RegisterVar(SavedVar& var)
		{

		}
	};

	class EditorDescriptionBuilder;

	class Actor
	{
	public:
		Actor(GameScene&, Actor* parentActor, const std::string& name, const Transform& t = Transform());
		Actor(const Actor&) = delete;
		Actor(Actor&&);

		void PassToDifferentParent(Actor& newParent)
		{
			if (ParentActor)
			{
				for (auto& it = ParentActor->Children.begin(); it != ParentActor->Children.end(); it++)
					if (it->get() == this)
					{
						it->release();
						ParentActor->Children.erase(it);
						break;
					}
			}
			newParent.AddChild(UniquePtr<Actor>(this));
		}

		virtual void OnStart();
		virtual void OnStartAll();

		Component* GetRoot() const;
		std::string GetName() const;
		Transform* GetTransform() const;
		Actor* GetChild(unsigned int) const;
		std::vector<Actor*> GetChildren();
		GameScene& GetScene();
		GameManager* GetGameHandle();
		bool IsBeingKilled() const;

		void SetName(const std::string&);

		void DebugHierarchy(int nrTabs = 0);
		void DebugActorHierarchy(int nrTabs = 0);

		void ReplaceRoot(UniquePtr<Component>);
		void MoveCompToRoot(Component&);
		void SetTransform(Transform);
		void AddComponent(UniquePtr<Component>);
		virtual Actor& AddChild(UniquePtr<Actor>);
		template <typename ChildClass, typename... Args> ChildClass& CreateChild(Args&&...);
		template <typename CompClass, typename... Args> CompClass& CreateComponent(Args&&...);	//Adds a new component that is a child of RootComponent.
		template <typename CompClass, typename... Args> CompClass& CreateComponent(CompClass&&, Component* parent);	//Adds a new component that is a child of parent. Pass nullptr as the second argument to make this created component the new RootComponent.
		void SetSetupStream(std::stringstream* stream);

		virtual void Setup();

		virtual void HandleEvent(const Event& ev) {}
		virtual void HandleEventAll(const Event& ev);

		virtual void Update(float);
		void UpdateAll(float);

		void MarkAsKilled();

		void Delete();

		virtual void DebugRender(RenderInfo info, Shader* shader) const; //this method should only be called to render the components as something (usually a textured billboard) to debug the project.
		void DebugRenderAll(RenderInfo info, Shader* shader) const;

		Actor* FindActor(const std::string& name);
		const Actor* FindActor(const std::string& name) const;

		/**
		 * @brief Creates UI elements to edit the Actor in the editor
		 * @param descBuilder: an EditorDescriptionBuilder used to easily create UI elements
		*/
		virtual void GetEditorDescription(EditorDescriptionBuilder descBuilder);

		template<class ActorClass> void GetAllActors(std::vector <ActorClass*>* comps)	//this function returns every element further in the hierarchy tree (kids, kids' kids, ...) that is of ActorClass type
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
				Children[i]->GetAllActors<ActorClass>(comps);	//do it reccurently in every child
		}

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(CEREAL_NVP(Name), CEREAL_NVP(RootComponent), CEREAL_NVP(Children));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			cereal::LoadAndConstruct<Component>::ActorRef = this;	//For constructing the root component and its children
			cereal::LoadAndConstruct<Component>::ParentComp = nullptr;	//The root component doesn't have a parent
			RootComponent = nullptr;
			archive(CEREAL_NVP(Name), CEREAL_NVP(RootComponent));
			std::cout << "Serializing actor " << Name << '\n';

			if (GameHandle->HasStarted())
				OnStartAll();

			//LoadAndConstruct<Actor>::ParentActor = this;
			Children.clear();
			archive(CEREAL_NVP(Children));
			//LoadAndConstruct<Actor>::ParentActor = ParentActor;
			for (auto& it : Children)
			{
				it->ParentActor = this;
				it->GetTransform()->SetParentTransform(GetTransform());
			}
		}

		virtual ~Actor() {}

	protected:
		UniquePtr<Component> RootComponent;
		std::vector<UniquePtr<Actor>> Children;
		Actor* ParentActor;
		std::string Name;
		std::stringstream* SetupStream;
		GameScene& Scene;
		GameManager* GameHandle;

		bool bKillingProcessStarted;
	};


	template <typename ChildClass, typename... Args>
	inline ChildClass& Actor::CreateChild(Args&&... args)
	{
		UniquePtr<ChildClass> createdChild = MakeUnique<ChildClass>(Scene, this, std::forward<Args>(args)...);
		ChildClass& childRef = *createdChild.get();
		AddChild(std::move(createdChild));

		if (GameHandle->HasStarted())
			childRef.OnStartAll();

		return childRef;
	}

	template <typename CompClass, typename... Args>
	inline CompClass& Actor::CreateComponent(Args&&... args)
	{
		CompClass& comp = RootComponent->CreateComponent<CompClass>(std::forward<Args>(args)...);
		return comp;
	}
}

namespace cereal
{
	template <> struct LoadAndConstruct<GEE::Actor>
	{
		static GEE::GameScene* ScenePtr;
		//	static Actor* ParentActor;
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::Actor>& construct)
		{
			std::cout << "trying to construct actor " << ScenePtr << '\n';
			if (!ScenePtr)
				return;

			construct(*ScenePtr, nullptr, "undefined actor");
			construct->Load(ar);
		}
	};
	template <> struct LoadAndConstruct<GEE::GunActor>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::GunActor>& construct)
		{
			if (!LoadAndConstruct<GEE::Actor>::ScenePtr)
				return;

			construct(*LoadAndConstruct<GEE::Actor>::ScenePtr, nullptr, "undefined gunactor");
			construct->Load(ar);
		}
	};
	template <> struct LoadAndConstruct<GEE::PawnActor>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::PawnActor>& construct)
		{
			if (!LoadAndConstruct<GEE::Actor>::ScenePtr)
				return;

			construct(*LoadAndConstruct<GEE::Actor>::ScenePtr, nullptr, "undefined pawnactor");
			construct->Load(ar);
		}
	};
	template <> struct LoadAndConstruct<GEE::Controller>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::Controller>& construct)
		{
			if (!LoadAndConstruct<GEE::Actor>::ScenePtr)
				return;

			construct(*LoadAndConstruct<GEE::Actor>::ScenePtr, nullptr, "undefined controller");
			construct->Load(ar);
		}
	};
	template <> struct LoadAndConstruct<GEE::ShootingController>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::ShootingController>& construct)
		{
			if (!LoadAndConstruct<GEE::Actor>::ScenePtr)
				return;

			construct(*LoadAndConstruct<GEE::Actor>::ScenePtr, nullptr, "undefined controller");
			construct->Load(ar);
		}
	};
}