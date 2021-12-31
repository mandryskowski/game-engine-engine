#pragma once
#include <vector>
#include <string>
#include <memory>
#include <physics/CollisionObject.h>
#include <scene/BoneComponent.h>
#include <utility/Utility.h>
#include <scene/ModelComponent.h>	//move

namespace GEE
{
	namespace HierarchyTemplate
	{
		template <typename CompType> class HierarchyNode;

		class HierarchyNodeBase	// Interface
		{
		public:
			virtual Component& GetCompBaseType() = 0;
			virtual const Component& GetCompBaseType() const = 0;
			virtual Physics::CollisionObject* GetCollisionObject() = 0;
			virtual unsigned int GetChildCount() const = 0;
			virtual HierarchyNodeBase* GetChild(unsigned int index) const = 0;
			virtual HierarchyNodeBase& AddChild(UniquePtr<HierarchyNodeBase> child) = 0;
			virtual HierarchyNodeBase* FindNode(const std::string& name) = 0;
			template <typename CompType> HierarchyNode<CompType>& CreateChild(const std::string& name)
			{
				UniquePtr<HierarchyNode<CompType>> nodeSmartPtr = MakeUnique<HierarchyNode<CompType>>(GetCompBaseType().ActorRef, name);
				HierarchyNode<CompType>& nodeRef = *nodeSmartPtr;
				AddChild(std::move(nodeSmartPtr));

				return nodeRef;
			}

			virtual void InstantiateToComp(Component& comp) const = 0;
			virtual UniquePtr<Component> GenerateComp(Actor& compActor) const = 0;

			virtual UniquePtr<HierarchyNodeBase> Copy(Actor& tempActor, bool copyChildren = false) const = 0;

			virtual Physics::CollisionObject* SetCollisionObject(UniquePtr<Physics::CollisionObject>) = 0;
			virtual void AddCollisionShape(SharedPtr<Physics::CollisionShape> shape) = 0;

			virtual void Delete() = 0;

		private:
			virtual void SetParent(HierarchyNodeBase*) = 0;
			virtual void RemoveChild(HierarchyNodeBase*) = 0;

			template <typename> friend class HierarchyNode;
		};

		template <typename CompType = Component> class HierarchyNode : public HierarchyNodeBase
		{
		public:
			HierarchyNode(Actor& tempActor, const std::string& name);
			HierarchyNode(const HierarchyNode<CompType>& node, Actor& tempActor, bool copyChildren = false);
			HierarchyNode(HierarchyNode<CompType>&& node, Actor& tempActor, bool copyChildren = true);
			HierarchyNode(const HierarchyNode<CompType>&) = delete;
			HierarchyNode(HierarchyNode<CompType>&&) = delete;

			//UniquePtr<CompType> Instantiate(Component& parent, const std::string& name) const;
			virtual	void InstantiateToComp(Component& comp) const override; 

			/**
			 * @brief Creates a new Component of type CompType. To add it to the specified actor, pass the returned value to AddComponent of one of the actor's existing components (or replace its root).
			 * @param compActor: a reference to the Actor this generated component will belong to. Note that the generated component will NOT be added to this actor; do it manually.
			 * @return a unique pointer to the generated component. You can use std::dynamic_pointer_cast to cast it to CompType.
			*/
			virtual UniquePtr<Component> GenerateComp(Actor& compActor) const override;

			virtual HierarchyNodeBase* GetChild(unsigned int index) const;
			virtual unsigned int GetChildCount() const;
			CompType& GetCompT();
			const CompType& GetCompT() const;
			virtual Component& GetCompBaseType() override;
			virtual const Component& GetCompBaseType() const override;
			virtual Physics::CollisionObject* GetCollisionObject() override;

			virtual HierarchyNodeBase& AddChild(UniquePtr<HierarchyNodeBase> child) override;
			virtual HierarchyNodeBase* FindNode(const std::string& name);
			virtual UniquePtr<HierarchyNodeBase> Copy(Actor& tempActor, bool copyChildren = false) const override;

			virtual Physics::CollisionObject* SetCollisionObject(UniquePtr<Physics::CollisionObject> collisionObj) override;
			virtual void AddCollisionShape(SharedPtr<Physics::CollisionShape> shape) override;

			template <typename Archive> void Serialize(Archive& archive);

			virtual void Delete() override;

		private:
			virtual void SetParent(HierarchyNodeBase*) override;
			virtual void RemoveChild(HierarchyNodeBase*) override;
			//friend CompType& CompType::operator=(const ComponentTemplate<CompType>&);
			std::vector<UniquePtr<HierarchyNodeBase>> Children;
			HierarchyNodeBase* Parent;
			CompType CompT;
			UniquePtr<Physics::CollisionObject> CollisionObj;
		};
	}
}

#define GEE_SERIALIZABLE_NODE(Type) GEE_REGISTER_TYPE(Type);	 GEE_REGISTER_POLYMORPHIC_RELATION(GEE::HierarchyTemplate::HierarchyNodeBase, Type)   													 																											\
namespace cereal																				 																																	\
{																								 																																	\
	template <> struct LoadAndConstruct<Type>													 																																	\
	{																							 																																	\
		template <class Archive>																 																																	\
		static void load_and_construct(Archive& ar, cereal::construct<Type>& construct)			 																																	\
		{																						 																																	\
			if (!GEE::CerealNodeSerializationData::TempActor)																																										\
				return;																																																				\
																																																									\
			/* We set the name to serialization-error since it will be replaced by its original name anyways. */																													\
			construct(*GEE::CerealNodeSerializationData::TempActor, "serialization-error");																														    \
			construct->Serialize(ar); 																																														\
		}																						  																																	\
	};																							 													 																				\
}
GEE_REGISTER_TYPE(GEE::HierarchyTemplate::HierarchyNodeBase)
GEE_SERIALIZABLE_NODE(GEE::HierarchyTemplate::HierarchyNode<GEE::Component>);
GEE_SERIALIZABLE_NODE(GEE::HierarchyTemplate::HierarchyNode<GEE::ModelComponent>);
GEE_SERIALIZABLE_NODE(GEE::HierarchyTemplate::HierarchyNode<GEE::BoneComponent>);