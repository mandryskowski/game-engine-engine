#pragma once
#include <vector>
#include <string>
#include <memory>
#include <physics/CollisionObject.h>
#include <scene/BoneComponent.h>
#include <utility/Utility.h>
#include <scene/ModelComponent.h>	//move
#include <game/GameScene.h>
#include <scene/hierarchy/HierarchyNodeBase.h>

namespace GEE
{
	namespace Hierarchy
	{

		namespace Instantiation
		{
			struct Data;
		}

		template <typename CompType = Component> class Node : public NodeBase
		{
		public:
			Node(Actor& tempActor, const std::string& name);
			Node(const Node<CompType>& node, Actor& tempActor, bool copyChildren = false);
			Node(Node<CompType>&& node, Actor& tempActor, bool copyChildren = true);
			Node(const Node<CompType>&) = delete;
			Node(Node<CompType>&&) = delete;

			//UniquePtr<CompType> Instantiate(Component& parent, const std::string& name) const;
			virtual	void InstantiateToComp(SharedPtr<Instantiation::Data>, Component& comp) const override;

			/**
			 * @brief Creates a new Component of type CompType. To add it to the specified actor, pass the returned value to AddComponent of one of the actor's existing components (or replace its root).
			 * @param compActor: a reference to the Actor this generated component will belong to. Note that the generated component will NOT be added to this actor; do it manually.
			 * @return a unique pointer to the generated component. You can use std::dynamic_pointer_cast to cast it to CompType.
			*/
			virtual UniquePtr<Component> GenerateComp(SharedPtr<Instantiation::Data>, Actor& compActor) const override;

			virtual NodeBase* GetChild(unsigned int index) const;
			virtual unsigned int GetChildCount() const;
			CompType& GetCompT();
			const CompType& GetCompT() const;
			virtual Component& GetCompBaseType() override;
			virtual const Component& GetCompBaseType() const override;
			virtual Physics::CollisionObject* GetCollisionObject() override;
			virtual const Physics::CollisionObject* GetCollisionObject() const override;

			virtual NodeBase& AddChild(UniquePtr<NodeBase> child) override;
			virtual NodeBase* FindNode(const std::string& name);
			virtual UniquePtr<NodeBase> Copy(Actor& tempActor, bool copyChildren = false) const override;

			virtual Physics::CollisionObject* SetCollisionObject(UniquePtr<Physics::CollisionObject> collisionObj) override;
			virtual void AddCollisionShape(SharedPtr<Physics::CollisionShape> shape) override;

			template <typename Archive> void Serialize(Archive& archive);

			virtual void Delete() override;

		private:
			virtual void SetParent(NodeBase*) override;
			virtual void RemoveChild(NodeBase*) override;
			//friend CompType& CompType::operator=(const ComponentTemplate<CompType>&);

			std::vector<UniquePtr<NodeBase>> Children;
			NodeBase* Parent;
			CompType CompT;
			UniquePtr<Physics::CollisionObject> CollisionObj;
		};
	}
}

#define GEE_SERIALIZABLE_NODE(Type) GEE_REGISTER_TYPE(Type);	 GEE_REGISTER_POLYMORPHIC_RELATION(GEE::Hierarchy::NodeBase, Type)   													 																											\
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
			construct(*GEE::CerealNodeSerializationData::TempActor, "serialization-error");																																		    \
			construct->Serialize(ar); 																																																\
		}																						  																																	\
	};																							 													 																				\
}
GEE_REGISTER_TYPE(GEE::Hierarchy::NodeBase)
GEE_SERIALIZABLE_NODE(GEE::Hierarchy::Node<GEE::Component>);
GEE_SERIALIZABLE_NODE(GEE::Hierarchy::Node<GEE::ModelComponent>);
GEE_SERIALIZABLE_NODE(GEE::Hierarchy::Node<GEE::BoneComponent>);