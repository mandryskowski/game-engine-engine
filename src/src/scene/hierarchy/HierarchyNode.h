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

		class HierarchyNodeBase
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

			virtual UniquePtr<HierarchyNodeBase> Copy(Actor& tempActor, bool copyChildren = false) const = 0;

			virtual void SetCollisionObject(UniquePtr<Physics::CollisionObject>) = 0;
			virtual void AddCollisionShape(SharedPtr<Physics::CollisionShape> shape) = 0;
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

			virtual void SetCollisionObject(UniquePtr<Physics::CollisionObject> collisionObj) override;
			virtual void AddCollisionShape(SharedPtr<Physics::CollisionShape> shape) override;

		private:
			//friend CompType& CompType::operator=(const ComponentTemplate<CompType>&);
			std::vector<UniquePtr<HierarchyNodeBase>> Children;
			CompType CompT;
			UniquePtr<Physics::CollisionObject> CollisionObj;
		};
	}
}