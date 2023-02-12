#pragma once
#include <utility/Utility.h>
#include <game/GameManager.h>

namespace GEE
{
	namespace Hierarchy
	{
		template <typename CompType> class Node;

		namespace Instantiation
		{
			struct Data;
		}

		class NodeBase	// Interface
		{
		public:
			virtual Component& GetCompBaseType() = 0;
			virtual const Component& GetCompBaseType() const = 0;
			virtual Physics::CollisionObject* GetCollisionObject() = 0;
			virtual const Physics::CollisionObject* GetCollisionObject() const = 0;
			virtual unsigned int GetChildCount() const = 0;
			virtual NodeBase* GetChild(unsigned int index) const = 0;
			virtual NodeBase& AddChild(UniquePtr<NodeBase> child) = 0;
			virtual NodeBase* FindNode(const std::string& name) = 0;
			template <typename CompType> Node<CompType>& CreateChild(const std::string& name)
			{
				UniquePtr<Node<CompType>> nodeSmartPtr = MakeUnique<Node<CompType>>(GetCompBaseType().GetActor(), name);
				Node<CompType>& nodeRef = *nodeSmartPtr;
				AddChild(std::move(nodeSmartPtr));

				return nodeRef;
			}

			virtual void InstantiateToComp(SharedPtr<Instantiation::Data>, Component& comp) const = 0;
			virtual UniquePtr<Component> GenerateComp(SharedPtr<Instantiation::Data>, Actor& compActor) const = 0;

			virtual UniquePtr<NodeBase> Copy(Actor& tempActor, bool copyChildren = false) const = 0;

			virtual Physics::CollisionObject* SetCollisionObject(UniquePtr<Physics::CollisionObject>) = 0;
			virtual void AddCollisionShape(SharedPtr<Physics::CollisionShape> shape) = 0;

			virtual void Delete() = 0;

		private:
			virtual void SetParent(NodeBase*) = 0;
			virtual void RemoveChild(NodeBase*) = 0;

			friend class Tree;
			template <typename> friend class Node;
		};
	}
}