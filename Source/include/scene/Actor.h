#pragma once
#include <game/GameManager.h>
#include <math/Transform.h>
#include <input/Event.h>
#include <iostream>
#include <map>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>

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
	class FPSController;
	class CueController;

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
			CerealComponentSerializationData::ActorRef = this;	//For constructing the root component and its children
			CerealComponentSerializationData::ParentComp = nullptr;	//The root component doesn't have a parent
			RootComponent = nullptr;
			archive(CEREAL_NVP(Name), CEREAL_NVP(RootComponent));
			std::cout << "Serializing actor " << Name << '\n';

			if (GameHandle->HasStarted())
				OnStartAll();

			Children.clear();
			archive(CEREAL_NVP(Children));

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

#define GEE_SERIALIZABLE_ACTOR(Type, ...) CEREAL_REGISTER_TYPE(Type);									  						 \
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
			construct(*GEE::CerealActorSerializationData::ScenePtr, nullptr, std::string("serialization-error"), __VA_ARGS__);	 \
			construct->Load(ar);																					 			 \
		}																								  			 			 \
	};																									  						 \
}
#define GEE_POLYMORPHIC_SERIALIZABLE_ACTOR(Base, Derived, ...) GEE_SERIALIZABLE_ACTOR(Derived, __VA_ARGS__); CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, Derived);

#define GEE_EDITOR_ACTOR(Type) 


GEE_SERIALIZABLE_ACTOR(GEE::Actor)