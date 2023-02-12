#pragma once
#include <scene/Component.h>
#include <game/GameManager.h>
#include <math/Transform.h>
#include <input/Event.h>
#include <utility/CerealNames.h>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>

namespace GEE
{
	class EditorDescriptionBuilder;
	class FPSController;
	class CueController;

	class Actor
	{
	public:
		Actor(GameScene&, Actor* parentActor, const std::string& name, const Transform& t = Transform());
	/*private:
		/**
		 * @brief Copy constructor only to be used in Actor::CopyChild. The constructor won't copy its children and won't add itself as a child of the original copied Actor.
		* /
		Actor(const Actor&);
	public: */
		Actor(Actor&&);

		void PassToDifferentParent(Actor& newParent)
		{
			if (ParentActor)
			{
				for (auto it = ParentActor->Children.begin(); it != ParentActor->Children.end(); it++)
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

		std::string GetName() const { return Name; }
		GEEID GetGEEID() const { return ActorGEEID; }

		Component* GetRoot() const;
		GEEID GetID() const { return ActorGEEID; }
		Transform* GetTransform() const;
		std::vector<Actor*> GetChildren();
		GameScene& GetScene() { return Scene; }
		const GameScene& GetScene() const { return Scene; }
		GameManager* GetGameHandle() { return GameHandle; }
		const Actor* GetParentActor() const { return ParentActor; }

		/**
		 * @brief Get a pointer to an Actor further in the hierarchy (kids, kids' kids, ...). Limit the use of this function at runtime, as dynamic_cast has a significant overhead.
		 * @tparam ActorClass: The sought Actor must be dynamic_castable to ActorClass.
		 * @param name: The name of the sought Actor.
		 * @return: A pointer to the sought Actor of ActorClass type.
		*/
		template <class ActorClass = Actor> ActorClass* GetActor(const std::string& name);

		bool IsBeingKilled() const;

		void SetName(const std::string&);

		void DebugHierarchy(int nrTabs = 0);
		void DebugActorHierarchy(int nrTabs = 0);

		void ReplaceRoot(UniquePtr<Component>);
		void MoveCompToRoot(Component&);
		void SetTransform(Transform);
		Component& AddComponent(UniquePtr<Component>);
		virtual Actor& AddChild(UniquePtr<Actor>);
		template <typename ChildClass, typename... Args> ChildClass& CreateChild(Args&&...);
		template <typename CompClass, typename... Args> CompClass& CreateComponent(Args&&...);	//Adds a new component that is a child of RootComponent.
		void SetSetupStream(std::stringstream* stream);

		virtual void Setup();

		virtual void HandleEvent(const Event& ev);
		virtual void HandleEventAll(const Event& ev);

		virtual void Update(Time dt);
		void UpdateAll(Time dt);

		void MarkAsKilled();

		void Delete();

		virtual void DebugRender(SceneMatrixInfo info, Shader* shader) const; //this method should only be called to render the components as something (usually a textured billboard) to debug the project.
		void DebugRenderAll(SceneMatrixInfo info, Shader* shader) const;

		Actor* FindActor(const std::string& name);
		Actor* FindActor(GEEID);
		const Actor* FindActor(const std::string& name) const
		{
			return const_cast<Actor*>(this)->FindActor(name);
		}
		const Actor* FindActor(GEEID geeid) const
		{
			return const_cast<Actor*>(this)->FindActor(geeid);
		}

		/**
		 * @brief Creates UI elements to edit the Actor in the editor
		 * @param descBuilder: an EditorDescriptionBuilder used to easily create UI elements
		*/
		virtual void GetEditorDescription(EditorDescriptionBuilder descBuilder);

		template<class ActorClass> void GetAllActors(std::vector <ActorClass*>* comps)	//this function returns every element further in the hierarchy (kids, kids' kids, ...) that is of ActorClass type
		{
			if (!comps)
				return;
			for (auto& i : Children)
			{
				ActorClass* c = dynamic_cast<ActorClass*>(i.get());
				if (c)					//if dynamic cast was succesful - this children is of ActorClass type
					comps->push_back(c);
			}
			for (auto& i : Children)
				i->GetAllActors<ActorClass>(comps);	//do it reccurently in every child
		}

		template <typename Archive> void Save(Archive& archive) const;
		template <typename Archive> void Load(Archive& archive);

		virtual ~Actor() = default;

	protected:
		std::string Name;
		GEEID ActorGEEID;

		UniquePtr<Component> RootComponent;
		std::vector<UniquePtr<Actor>> Children;
		Actor* ParentActor;
		std::stringstream* SetupStream;
		GameScene& Scene;
		GameManager* GameHandle;

		// If it equals 0, killing process has not started.
		unsigned long long KillingProcessFrame;
	};


	template <typename ChildClass, typename... Args>
	ChildClass& Actor::CreateChild(Args&&... args)
	{
		UniquePtr<ChildClass> createdChild = MakeUnique<ChildClass>(Scene, this, std::forward<Args>(args)...);
		ChildClass& childRef = *createdChild.get();
		AddChild(std::move(createdChild));

		return childRef;
	}

	template <typename CompClass, typename... Args>
	CompClass& Actor::CreateComponent(Args&&... args)
	{
		CompClass& comp = RootComponent->CreateComponent<CompClass>(std::forward<Args>(args)...);
		return comp;
	}

	template<class ActorClass>
	ActorClass* Actor::GetActor(const std::string& name)
	{
		if (Name == name)
			return dynamic_cast<ActorClass*>(this);

		for (const auto& it : Children)
			if (auto found = it->GetActor<ActorClass>(name))
				return found;

		return nullptr;
	}

	struct CerealActorSerializationData
	{
		static GEE::GameScene* ScenePtr;
	};

	// Load and construct actor
	template <typename ActorClass, class Archive>
	static void LoadAndConstructDefaultActor(Archive& ar, cereal::construct<ActorClass>& construct)
	{
		if (!CerealActorSerializationData::ScenePtr)
			return;

		// We set the name to serialization-error since it will be replaced by its original name anyways.
		construct(*CerealActorSerializationData::ScenePtr, nullptr, std::string("serialization-error"));
		construct->Load(ar);
	}
}

#define GEE_SERIALIZABLE_ACTOR(Type, ...) GEE_REGISTER_TYPE(Type);									  							 \
namespace cereal																						  						 \
{																										  						 \
	template <> struct LoadAndConstruct<Type>															  						 \
	{																									  						 \
		template <class Archive>																		  						 \
		static void load_and_construct(Archive& ar, cereal::construct<Type>& construct)					  						 \
		{																								  						 \
			if (!GEE::CerealActorSerializationData::ScenePtr)																	 \
				return;																											 \
																																 \
			/* We set the name to serialization-error since it will be replaced by its original name anyways. */				 \
			construct(*GEE::CerealActorSerializationData::ScenePtr, nullptr, std::string("serialization-error"), ##__VA_ARGS__);	 \
			construct->Load(ar);																					 			 \
		}																								  			 			 \
	};																									  						 \
}
#define GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(Base, Derived, ...) GEE_SERIALIZABLE_ACTOR(Derived, ##__VA_ARGS__); CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, Derived);

#define GEE_EDITOR_ACTOR(Type) 


GEE_SERIALIZABLE_ACTOR(GEE::Actor)