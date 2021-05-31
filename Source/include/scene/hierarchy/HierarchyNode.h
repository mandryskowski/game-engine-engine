#pragma once
#include <vector>
#include <string>
#include <memory>
#include <physics/CollisionObject.h>
#include <scene/BoneComponent.h>
#include <utility/Utility.h>
#include <functional>	//move
#include <scene/ModelComponent.h>	//move
#include <scene/Actor.h>

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
			virtual HierarchyNodeBase& AddChild(std::unique_ptr<HierarchyNodeBase> child) = 0;
			virtual HierarchyNodeBase* FindNode(const std::string& name) = 0;
			template <typename CompType> HierarchyNode<CompType>& CreateChild(const std::string& name)
			{
				std::unique_ptr<HierarchyNode<CompType>> nodeSmartPtr = std::make_unique<HierarchyNode<CompType>>(GetCompBaseType().ActorRef, name);
				HierarchyNode<CompType>& nodeRef = *nodeSmartPtr;
				AddChild(std::move(nodeSmartPtr));

				return nodeRef;
			}

			virtual void InstantiateToComp(Component& comp) const = 0;

			virtual std::unique_ptr<HierarchyNodeBase> Copy(Actor& tempActor, bool copyChildren = false) const = 0;

			virtual void SetCollisionObject(std::unique_ptr<Physics::CollisionObject>) = 0;
			virtual void AddCollisionShape(std::shared_ptr<Physics::CollisionShape> shape) = 0;
		};

		template <typename CompType = Component> class HierarchyNode : public HierarchyNodeBase
		{
		public:
			HierarchyNode(Actor& tempActor, const std::string& name);
			HierarchyNode(const HierarchyNode<CompType>& node, Actor& tempActor, bool copyChildren = false);
			HierarchyNode(HierarchyNode<CompType>&& node, Actor& tempActor, bool copyChildren = true);
			HierarchyNode(const HierarchyNode<CompType>&) = delete;
			HierarchyNode(HierarchyNode<CompType>&&) = delete;
			//std::unique_ptr<CompType> Instantiate(Component& parent, const std::string& name) const;
			virtual	void InstantiateToComp(Component& comp) const override;
			virtual HierarchyNodeBase* GetChild(unsigned int index) const;
			virtual unsigned int GetChildCount() const;
			CompType& GetCompT();
			const CompType& GetCompT() const;
			virtual Component& GetCompBaseType() override;
			virtual const Component& GetCompBaseType() const override;
			virtual Physics::CollisionObject* GetCollisionObject() override;
			virtual HierarchyNodeBase& AddChild(std::unique_ptr<HierarchyNodeBase> child) override;
			virtual HierarchyNodeBase* FindNode(const std::string& name);
			virtual std::unique_ptr<HierarchyNodeBase> Copy(Actor& tempActor, bool copyChildren = false) const override;

			virtual void SetCollisionObject(std::unique_ptr<Physics::CollisionObject> collisionObj) override;
			virtual void AddCollisionShape(std::shared_ptr<Physics::CollisionShape> shape) override;

		private:
			//friend CompType& CompType::operator=(const ComponentTemplate<CompType>&);
			std::vector<std::unique_ptr<HierarchyNodeBase>> Children;
			CompType CompT;
			std::unique_ptr<Physics::CollisionObject> CollisionObj;
		};
	}
}