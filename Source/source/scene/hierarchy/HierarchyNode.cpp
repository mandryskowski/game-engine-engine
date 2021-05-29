#include <scene/hierarchy/HierarchyNode.h>

namespace GEE
{
	using namespace HierarchyTemplate;

	template <typename CompType>
	HierarchyNode<CompType>::HierarchyNode(Actor& tempActor, const std::string& name) :
		CompT(CompType(tempActor, nullptr, name))
	{ }

	template<typename CompType>
	HierarchyTemplate::HierarchyNode<CompType>::HierarchyNode(const HierarchyNode<CompType>& node, Actor& tempActor, bool copyChildren) :
		CompT(CompType(tempActor, nullptr, node.GetCompT().GetName()))
	{
		CompT = node.CompT;
		if (node.CollisionObj)
			CollisionObj = std::make_unique<CollisionObject>(*node.CollisionObj);
		if (copyChildren)
		{
			Children.reserve(node.Children.size());
			std::transform(node.Children.begin(), node.Children.end(), std::back_inserter(Children), [&tempActor, copyChildren](const std::unique_ptr<HierarchyNodeBase>& child) { return child->Copy(tempActor, copyChildren); });
		}
	}

	template<typename CompType>
	HierarchyTemplate::HierarchyNode<CompType>::HierarchyNode(HierarchyNode<CompType>&& node, Actor& tempActor, bool copyChildren) :
		HierarchyNode(static_cast<const HierarchyNode<CompType>&>(node), tempActor, copyChildren)
	{
	}

	/*
	template<typename CompType>
	std::unique_ptr<CompType> HierarchyTemplate::HierarchyNode<CompType>::Instantiate(Component& parent, const std::string& name) const
	{
		InstantiateToComp(parent.CreateComponent<CompType>(name));
	}*/

	template<typename CompType>
	void HierarchyTemplate::HierarchyNode<CompType>::InstantiateToComp(Component& comp) const
	{
		CompType* compCast = dynamic_cast<CompType*>(&comp);

		if (compCast)
			*compCast = CompT;
		else
			comp = CompT;

		if (CollisionObj)
			comp.SetCollisionObject(std::make_unique<CollisionObject>(*CollisionObj));
	}

	template<typename CompType>
	HierarchyNodeBase* HierarchyTemplate::HierarchyNode<CompType>::GetChild(unsigned int index) const
	{
		if (index > Children.size() - 1)
			return nullptr;

		return Children[index].get();
	}

	template<typename CompType>
	unsigned int HierarchyTemplate::HierarchyNode<CompType>::GetChildCount() const
	{
		return Children.size();
	}

	template<typename CompType>
	CompType& HierarchyTemplate::HierarchyNode<CompType>::GetCompT()
	{
		return CompT;
	}

	template<typename CompType>
	const CompType& HierarchyTemplate::HierarchyNode<CompType>::GetCompT() const
	{
		return CompT;
	}

	template<typename CompType>
	Component& HierarchyTemplate::HierarchyNode<CompType>::GetCompBaseType()
	{
		return CompT;
	}

	template<typename CompType>
	const Component& HierarchyTemplate::HierarchyNode<CompType>::GetCompBaseType() const
	{
		return static_cast<const Component&>(CompT);
	}

	template<typename CompType>
	CollisionObject* HierarchyTemplate::HierarchyNode<CompType>::GetCollisionObject()
	{
		return CollisionObj.get();
	}

	template<typename CompType>
	HierarchyNodeBase& HierarchyTemplate::HierarchyNode<CompType>::AddChild(std::unique_ptr<HierarchyNodeBase> child)
	{
		Children.push_back(std::move(child));
		return *Children.back().get();
	}

	template<typename CompType>
	HierarchyNodeBase* HierarchyTemplate::HierarchyNode<CompType>::FindNode(const std::string& name)
	{
		if (GetCompBaseType().GetName() == name)
			return this;

		for (auto& it : Children)
			if (auto found = it->FindNode(name))
				return found;

		return nullptr;
	}

	template<typename CompType>
	std::unique_ptr<HierarchyNodeBase> HierarchyTemplate::HierarchyNode<CompType>::Copy(Actor& tempActor, bool copyChildren) const
	{
		return static_unique_pointer_cast<HierarchyNodeBase>(std::make_unique<HierarchyNode<CompType>>(*this, tempActor, copyChildren));
	}

	template<typename CompType>
	void HierarchyTemplate::HierarchyNode<CompType>::SetCollisionObject(std::unique_ptr<CollisionObject> collisionObj)
	{
		CollisionObj = std::move(collisionObj);
	}

	template<typename CompType>
	void HierarchyTemplate::HierarchyNode<CompType>::AddCollisionShape(std::shared_ptr<CollisionShape> shape)
	{
		if (!CollisionObj)
			CollisionObj = std::make_unique<CollisionObject>();

		CollisionObj->AddShape(shape);
	}


	template <> HierarchyNode<ModelComponent>::HierarchyNode(Actor& tempActor, const std::string& name) :
		CompT(ModelComponent(tempActor, nullptr, name))
	{
		tempActor.GetScene().GetRenderData()->EraseRenderable(CompT);
	}

	template <> HierarchyNode<ModelComponent>::HierarchyNode(const HierarchyNode<ModelComponent>& node, Actor& tempActor, bool copyChildren) :
		CompT(ModelComponent(tempActor, nullptr, node.GetCompT().GetName()))
	{
		CompT = node.CompT;
		if (node.CollisionObj)
			CollisionObj = std::make_unique<CollisionObject>(*node.CollisionObj);
		if (copyChildren)
		{
			Children.reserve(node.Children.size());
			std::transform(node.Children.begin(), node.Children.end(), std::back_inserter(Children), [&tempActor, copyChildren](const std::unique_ptr<HierarchyNodeBase>& child) { return child->Copy(tempActor, copyChildren); });
		}
		CompT.GetScene().GetRenderData()->EraseRenderable(CompT);
	}
	template <> HierarchyNode<ModelComponent>::HierarchyNode(HierarchyNode<ModelComponent>&& node, Actor& tempActor, bool copyChildren) :
		HierarchyNode(static_cast<const HierarchyNode<ModelComponent>&>(node), tempActor, copyChildren)
	{
		CompT.GetScene().GetRenderData()->EraseRenderable(CompT);
	}

	template class HierarchyNode<ModelComponent>;
	template class HierarchyNode<BoneComponent>;
	template class HierarchyNode<Component>;
}