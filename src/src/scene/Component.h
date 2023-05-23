#pragma once
#include <utility/CerealNames.h>
#include <cereal/access.hpp>
#include <cereal/types/polymorphic.hpp>
#include <math/Transform.h>
#include <game/GameManager.h>

#include "utility/SerialRegistry.h"

namespace GEE
{
	class SceneMatrixInfo;
	class AtlasMaterial;
	class Shader;
	class Event;
	struct MaterialInstance;

	namespace Physics
	{
		struct CollisionObject;
		struct CollisionObjectDeleter
		{
			void operator()(CollisionObject*);
		};
	}

	enum class ButtonMaterialType;

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

	class ComponentDescriptionBuilder;

	class Killable
	{
	public:
		virtual ~Killable() = default;
		virtual bool IsBeingKilled();
		template <typename FieldType> void AddReference(FieldType&);
		template <typename FieldType> void RemoveReference(FieldType&);
	protected:
		virtual void MarkAsKilled();

	};

	class ComponentBase
	{
	public:
		virtual ~ComponentBase() = default;
		virtual Component& AddComponent(UniquePtr<Component> component) = 0;
	};

	class Component : public ComponentBase
	{
	public:
		Component(Actor&, Component* parentComp, const String& name = "A Component", const Transform& t = Transform());

		Component(const Component&) = delete;
		Component(Component&&);// = delete;
		//Component(Actor&, Component&&);

	protected:
		template <typename CompClass> friend class Hierarchy::Node;
		Component& operator=(const Component&);

		template <typename FieldType> void AddToGarbageCollector(FieldType*& field);
	public:
		Component& operator=(Component&&);

		virtual void OnStart();
		virtual void OnStartAll();

		String GetName() const { return Name; }	
		GEEID GetGEEID() const { return ComponentGEEID; }

		void DebugHierarchy(int nrTabs = 0);

		Actor& GetActor() const;
		Transform& GetTransform();
		const Transform& GetTransform() const;
		Component* GetParent()
		{
			return ParentComponent;
		}
		std::vector<Component*> GetChildren();
		GameScene& GetScene() const;
		Physics::CollisionObject* GetCollisionObj() const;
		MaterialInstance* GetDebugMatInst();
		GameManager* GetGameHandle() const { return GameHandle; }

		/**
		 * @brief Find by name and get a pointer of type CompClass to a Component further in the hierarchy (kids, kids' kids, ...). Limit the use of this function at runtime, as dynamic_cast has a significant overhead.
		 * @tparam CompClass: The sought Component must be dynamic_castable to CompClass. 
		 * @param name: The name of the sought Component.
		 * @return: A pointer to the sought Component.
		*/
		template <class CompClass = Component> CompClass* GetComponent(const String& name);

		/**
		 * @brief Find by GEEID and get a pointer of type CompClass to a Component further in the hierarchy (kids, kids' kids, ...). Limit the use of this function at runtime, as dynamic_cast has a significant overhead.
		 * @tparam CompClass: The sought Component must be dynamic_castable to CompClass.
		 * @param name: The GEEID of the sought Component.
		 * @return: A pointer to the sought Component.
		*/
		template <class CompClass = Component> CompClass* GetComponent(GEEID geeid);

		/**
		 * @brief This function returns every element further in the hierarchy that is of CompClass type
		*/
		template<class CompClass> void GetAllComponents(std::vector <CompClass*>* comps);
		/**
		 * @brief This function works like the previous method, but every element must also be of CheckClass type (for ex. CastToClass = RenderableComponent, CheckClass = ModelComponent)
		 * It's useful when you want to get a vector of CastToClass type pointers to objects, that are also of CheckClass type.
		*/
		template<class CastToClass, class CheckClass> void GetAllComponents(std::vector <CastToClass*>* comps);

		bool IsBeingKilled() const;

		void SetName(const String& name);
		virtual void SetTransform(const Transform& transform);
		Physics::CollisionObject* SetCollisionObjectCopy(const Physics::CollisionObject&);
		Physics::CollisionObject* SetCollisionObject(UniquePtr<Physics::CollisionObject>);
		Component& AddComponent(UniquePtr<Component> component) override;
		void AddComponents(std::vector<UniquePtr<Component>> components);

		template<typename ChildClass, typename... Args> ChildClass& CreateComponent(Args&&... args);


		virtual void Update(Time dt);
		void UpdateAll(Time dt);

		virtual void HandleEvent(const Event& ev) {}
		void HandleEventAll(const Event& ev);

		virtual void QueueAnimation(Animation*);
		void QueueAnimationAll(Animation*);

		virtual void QueueKeyFrame(AnimationChannel&);
		static void QueueKeyFrameAll(AnimationChannel&);

		virtual void DebugRender(SceneMatrixInfo info, Shader& shader, const Vec3f& debugIconScale = Vec3f(0.05f)) const; //this method should only be called to render the component as something (usually a textured billboard) to debug the project.
		void DebugRenderAll(SceneMatrixInfo info, Shader& shader) const;

	protected:
		virtual SharedPtr<AtlasMaterial> LoadDebugRenderMaterial(const String& materialName, const String& path);
	public:
		virtual	MaterialInstance GetDebugMatInst(ButtonMaterialType);

		Component* SearchForComponent(String name);

		virtual void GetEditorDescription(ComponentDescriptionBuilder);

		void MarkAsKilled();

		template <typename Archive> void Save(Archive& archive) const;
		template <typename Archive> void Load(Archive& archive);
		virtual ~Component() override;

	private:
		friend class Actor;
		UniquePtr<Component> DetachChild(Component& soughtChild);	//Find child in hierarchy and detach it from its parent
		void MoveChildren(Component& moveTo);
		void Delete();

		String Name;
		GEEID ComponentGEEID;

		Transform ComponentTransform;
		std::vector <UniquePtr<Component>> Children;
		UniquePtr<Physics::CollisionObject, Physics::CollisionObjectDeleter> CollisionObj;

		GameScene& Scene; //The scene that this Component is present in
		Actor& ActorRef;
		Component* ParentComponent;
		GameManager* GameHandle;

		unsigned long long KillingProcessFrame;

		SharedPtr<AtlasMaterial> DebugRenderMat;
		SharedPtr<MaterialInstance> DebugRenderMatInst;
		mutable Mat4f DebugRenderLastFrameMVP;
	};


	template<typename ChildClass, typename... Args>
	ChildClass& Component::CreateComponent(Args&&... args)
	{
		UniquePtr<ChildClass> createdChild = MakeUnique<ChildClass>(ActorRef, this, std::forward<Args>(args)...);
		ChildClass& childRef = *createdChild;
		AddComponent(std::move(createdChild));

		return (ChildClass&)childRef;
	}

	template<class CompClass>
	CompClass* Component::GetComponent(const String& name)
	{
		if (Name == name)
			return dynamic_cast<CompClass*>(this);

		for (const auto& it : Children)
			if (auto found = it->GetComponent<CompClass>(name))
				return found;

		return nullptr;
	}
	template<class CompClass>
	CompClass* Component::GetComponent(GEEID geeid)
	{
		if (ComponentGEEID == geeid)
			return dynamic_cast<CompClass*>(this);

		for (const auto& it : Children)
			if (auto found = it->GetComponent<CompClass>(geeid))
				return found;

		return nullptr;
	}
	template<class CompClass>
	void Component::GetAllComponents(std::vector<CompClass*>* comps)
	{
		if (!comps)
			return;
		for (unsigned int i = 0; i < Children.size(); i++)
		{
			if (Children[i]->IsBeingKilled())
				continue;

			CompClass* c = dynamic_cast<CompClass*>(Children[i].get());
			if (c)					//if dynamic cast was succesful - this children is of CompCLass type
				comps->push_back(c);
		}
		for (unsigned int i = 0; i < Children.size(); i++)
			Children[i]->GetAllComponents<CompClass>(comps);	//do it reccurently in every child
	}
	template<class CastToClass, class CheckClass>
	void Component::GetAllComponents(std::vector<CastToClass*>* comps) 
	{																		
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
	void CollisionObjRendering(const SceneMatrixInfo& info, GameManager& gameHandle, Physics::CollisionObject& obj, const Transform& t, const Vec3f& color = Vec3f(0.1f, 0.6f, 0.3f));
}

 
#define GEE_SERIALIZABLE_COMPONENT(Type, ...) GEE_REGISTER_TYPE(Type);	   													 										\
namespace cereal																				 																	\
{																								 																	\
	template <> struct LoadAndConstruct<Type>													 																	\
	{																							 																	\
		template <class Archive>																 																	\
		static void load_and_construct(Archive& ar, cereal::construct<Type>& construct)			 																	\
		{																						 																	\
			if (!GEE::CerealComponentSerializationData::ActorRef)																											\
				return;																																				\
																																									\
			/* We set the name to serialization-error since it will be replaced by its original name anyways. */													\
			construct(*GEE::CerealComponentSerializationData::ActorRef, GEE::CerealComponentSerializationData::ParentComp, "serialization-error", ##__VA_ARGS__);		\
			construct->Load(ar); 																																	\
		}																						  																	\
	};																							 													 				\
}
#define GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(Base, Derived, ...) GEE_SERIALIZABLE_COMPONENT(Derived, ##__VA_ARGS__); CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, Derived);

#define GEE_EDITOR_COMPONENT(Type) 


GEE_SERIALIZABLE_COMPONENT(GEE::Component)