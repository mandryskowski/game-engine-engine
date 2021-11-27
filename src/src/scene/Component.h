#pragma once
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <math/Transform.h>
#include <game/GameManager.h>
#include <editor/EditorManager.h>
#include <physics/CollisionObject.h>
#include <rendering/Material.h>
#include <game/GameScene.h>

namespace GEE
{

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

	class EditorDescriptionBuilder;

	class Killable
	{
	public:
		virtual bool IsBeingKilled();
		template <typename FieldType> void AddReference(FieldType&);
		template <typename FieldType> void RemoveReference(FieldType&);
	protected:
		virtual void MarkAsKilled();

	};

	class ComponentBase
	{
	public:
		virtual Component& AddComponent(UniquePtr<Component> component) = 0;
	};

	class Component : public ComponentBase
	{
	public:
		Component(Actor&, Component* parentComp, const std::string& name = "A Component", const Transform& t = Transform());

		Component(const Component&) = delete;
		Component(Component&&);// = delete;
		//Component(Actor&, Component&&);

	protected:
		template <typename CompClass> friend class HierarchyTemplate::HierarchyNode;
		Component& operator=(const Component&);

		template <typename FieldType> void AddToGarbageCollector(FieldType*& field);
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

		Actor& GetActor() const;
		const std::string& GetName() const;
		Transform& GetTransform();
		const Transform& GetTransform() const;
		std::vector<Component*> GetChildren();
		GameScene& GetScene() const;
		Physics::CollisionObject* GetCollisionObj() const;
		MaterialInstance* GetDebugMatInst();

		/**
		 * @brief Get a pointer to a Component further in the hierarchy (kids, kids' kids, ...). Limit the use of this function at runtime, as dynamic_cast has a significant overhead.
		 * @tparam CompClass: The sought Component must be dynamic_castable to CompClass. 
		 * @param name: The name of the sought Component.
		 * @return: A pointer to the sought Component.
		*/
		template <class CompClass = Component> CompClass* GetComponent(const std::string& name);
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

		void SetName(std::string name);
		void SetTransform(Transform transform);
		Physics::CollisionObject* SetCollisionObject(UniquePtr<Physics::CollisionObject>);
		virtual Component& AddComponent(UniquePtr<Component> component) override;
		void AddComponents(std::vector<UniquePtr<Component>> components);

		template<typename ChildClass, typename... Args> ChildClass& CreateComponent(Args&&... args);


		virtual void Update(float);
		void UpdateAll(float dt);

		virtual void HandleEvent(const Event& ev) {}
		void HandleEventAll(const Event& ev);

		virtual void QueueAnimation(Animation*);
		void QueueAnimationAll(Animation*);

		virtual void QueueKeyFrame(AnimationChannel&);
		static void QueueKeyFrameAll(AnimationChannel&);

		virtual void DebugRender(SceneMatrixInfo info, Shader& shader, const Vec3f& debugIconScale = Vec3f(0.05f)) const; //this method should only be called to render the component as something (usually a textured billboard) to debug the project.
		void DebugRenderAll(SceneMatrixInfo info, Shader& shader) const;

		virtual SharedPtr<AtlasMaterial> LoadDebugRenderMaterial(const std::string& materialName, const std::string& path);

		Component* SearchForComponent(std::string name);

		virtual void GetEditorDescription(EditorDescriptionBuilder);

		void MarkAsKilled();

		template <typename Archive> void Save(Archive& archive)	 const;
		template <typename Archive> void Load(Archive& archive);
		virtual ~Component();

	public:
		friend class Actor;
		UniquePtr<Component> DetachChild(Component& soughtChild);	//Find child in hierarchy and detach it from its parent
		void MoveChildren(Component& moveTo);
		void Delete();

		virtual MaterialInstance LoadDebugMatInst(EditorIconState);

		std::string Name;

		Transform ComponentTransform;
		std::vector <UniquePtr<Component>> Children;
		UniquePtr<Physics::CollisionObject> CollisionObj;

		GameScene& Scene; //The scene that this Component is present in
		Actor& ActorRef;
		Component* ParentComponent;
		GameManager* GameHandle;

		unsigned long long KillingProcessFrame;

		SharedPtr<AtlasMaterial> DebugRenderMat;
		SharedPtr<MaterialInstance> DebugRenderMatInst;
		mutable Mat4f DebugRenderLastFrameMVP;
	};

	/*template<typename ChildClass>
	inline ChildClass& Component::CreateComponent(ChildClass&& objectCRef)
	{
		UniquePtr<ChildClass> createdChild = MakeUnique<ChildClass>(std::move(objectCRef));
		ChildClass& childRef = *createdChild;
		AddComponent(std::move(createdChild));

		return (ChildClass&)childRef;
	}*/


	template<typename ChildClass, typename... Args>
	inline ChildClass& Component::CreateComponent(Args&&... args)
	{
		UniquePtr<ChildClass> createdChild = MakeUnique<ChildClass>(ActorRef, this, std::forward<Args>(args)...);
		ChildClass& childRef = *createdChild;
		AddComponent(std::move(createdChild));

		if (GameHandle->HasStarted())
			childRef.OnStartAll();

		return (ChildClass&)childRef;
	}
	template<typename Archive>
	void Component::Save(Archive& archive) const
	{
		archive(CEREAL_NVP(Name), CEREAL_NVP(ComponentTransform), CEREAL_NVP(CollisionObj));
		archive(CEREAL_NVP(Children));
	}
	template<typename Archive>
	void Component::Load(Archive& archive)
	{
		archive(CEREAL_NVP(Name), CEREAL_NVP(ComponentTransform), CEREAL_NVP(CollisionObj));
		if (CollisionObj)
			Scene.GetPhysicsData()->AddCollisionObject(*CollisionObj, ComponentTransform);

		std::cout << "Serializing comp " << Name << '\n';
		if (GameHandle->HasStarted())
			OnStartAll();

		/*
		This code is not very elegant; it's because Components and Actors are not default_constructible.
		We always need to pass at least one reference to either the GameScene (in the case of Actors) or to the Actor (of the constructed Component).
		Furthermore, if an Actor/Component is not at the root of hierarchy (of a scene/an actor) we should always pass the pointer to the parent to the constructor, since users might wrongly assume that an Actor/Component is the root if the parent pointer passed to the constructor is nullptr.
		This might lead to bugs, so we avoid that.
		*/

		CerealComponentSerializationData::ParentComp = this;	//Set the static parent pointer to this
		archive(CEREAL_NVP(Children));	//We want all children to be constructed using the pointer. IMPORTANT: The first child will be serialized (the Load method will be called) before the next child is constructed! So the ParentComp pointer will be changed to the address of the new child when we construct its children. We counteract that in the next line.
		CerealComponentSerializationData::ParentComp = ParentComponent;	//There are no more children of this to serialize, so we "move up" the hierarchy and change the parent pointer to the parent of this, to allow brothers (other children of parent of this) to be constructed.

		for (auto& it : Children)
			it->GetTransform().SetParentTransform(&ComponentTransform);
	}
	template<class CompClass>
	inline CompClass* Component::GetComponent(const std::string& name)
	{
		if (Name == name)
			return dynamic_cast<CompClass*>(this);

		for (const auto& it : Children)
			if (auto found = it->GetComponent<CompClass>(name))
				return found;

		return nullptr;
	}
	template<class CompClass>
	inline void Component::GetAllComponents(std::vector<CompClass*>* comps)
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
	template<class CastToClass, class CheckClass>
	inline void Component::GetAllComponents(std::vector<CastToClass*>* comps) 
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
	void CollisionObjRendering(SceneMatrixInfo& info, GameManager& gameHandle, Physics::CollisionObject& obj, const Transform& t, const Vec3f& color = Vec3f(0.1f, 0.6f, 0.3f));
}

 
#define GEE_SERIALIZABLE_COMPONENT(Type, ...) CEREAL_REGISTER_TYPE(Type);	   													 									\
namespace cereal																				 																	\
{																								 																	\
	template <> struct LoadAndConstruct<Type>													 																	\
	{																							 																	\
		template <class Archive>																 																	\
		static void load_and_construct(Archive& ar, cereal::construct<Type>& construct)			 																	\
		{																						 																	\
			if (!GEE::CerealComponentSerializationData::ActorRef)																									\
				return;																																				\
																																									\
			/* We set the name to serialization-error since it will be replaced by its original name anyways. */													\
			construct(*GEE::CerealComponentSerializationData::ActorRef, GEE::CerealComponentSerializationData::ParentComp, "serialization-error", __VA_ARGS__);		\
			construct->Load(ar); 																																	\
		}																						  																	\
	};																							 													 				\
}
#define GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(Base, Derived, ...) GEE_SERIALIZABLE_COMPONENT(Derived, __VA_ARGS__); CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, Derived);

#define GEE_EDITOR_COMPONENT(Type) 


GEE_SERIALIZABLE_COMPONENT(GEE::Component)