#include <scene/hierarchy/HierarchyNode.h>
#include <game/GameScene.h>
#include <scene/Actor.h>
#include <scene/LightComponent.h>

GEE::Actor* GEE::CerealNodeSerializationData::TempActor = nullptr;
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
			CollisionObj = MakeUnique<Physics::CollisionObject>(*node.CollisionObj);
		if (copyChildren)
		{
			Children.reserve(node.Children.size());
			std::transform(node.Children.begin(), node.Children.end(), std::back_inserter(Children), [&tempActor, copyChildren](const UniquePtr<HierarchyNodeBase>& child) { return child->Copy(tempActor, copyChildren); });
		}
	}

	template<typename CompType>
	HierarchyTemplate::HierarchyNode<CompType>::HierarchyNode(HierarchyNode<CompType>&& node, Actor& tempActor, bool copyChildren) :
		HierarchyNode(static_cast<const HierarchyNode<CompType>&>(node), tempActor, copyChildren)
	{
	}

	/*
	template<typename CompType>
	UniquePtr<CompType> HierarchyTemplate::HierarchyNode<CompType>::Instantiate(Component& parent, const std::string& name) const
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
			comp.SetCollisionObject(MakeUnique<Physics::CollisionObject>(*CollisionObj));
	}

	template<typename CompType>
	UniquePtr<Component> HierarchyTemplate::HierarchyNode<CompType>::GenerateComp(Actor& compActor) const
	{
		UniquePtr<CompType> comp = MakeUnique<CompType>(compActor, nullptr, "generated");
		InstantiateToComp(*comp);

		return static_unique_pointer_cast<Component, CompType>(std::move(comp));
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
	Physics::CollisionObject* HierarchyTemplate::HierarchyNode<CompType>::GetCollisionObject()
	{
		return CollisionObj.get();
	}

	template<typename CompType>
	HierarchyNodeBase& HierarchyTemplate::HierarchyNode<CompType>::AddChild(UniquePtr<HierarchyNodeBase> child)
	{
		child->SetParent(this);
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
	UniquePtr<HierarchyNodeBase> HierarchyTemplate::HierarchyNode<CompType>::Copy(Actor& tempActor, bool copyChildren) const
	{
		return static_unique_pointer_cast<HierarchyNodeBase>(MakeUnique<HierarchyNode<CompType>>(*this, tempActor, copyChildren));
	}

	template<typename CompType>
	Physics::CollisionObject* HierarchyTemplate::HierarchyNode<CompType>::SetCollisionObject(UniquePtr<Physics::CollisionObject> collisionObj)
	{
		CollisionObj = std::move(collisionObj);
		return CollisionObj.get();
	}

	template<typename CompType>
	void HierarchyTemplate::HierarchyNode<CompType>::AddCollisionShape(SharedPtr<Physics::CollisionShape> shape)
	{
		if (!CollisionObj)
			CollisionObj = MakeUnique<Physics::CollisionObject>();

		CollisionObj->AddShape(shape);
	}

	template<typename CompType>
	void HierarchyTemplate::HierarchyNode<CompType>::Delete()
	{
		GEE_CORE_ASSERT(Parent);
		Parent->RemoveChild(this);
	}

	template<typename CompType>
	void HierarchyTemplate::HierarchyNode<CompType>::SetParent(HierarchyNodeBase* parent)
	{
		Parent = parent;
	}

	template<typename CompType>
	void HierarchyTemplate::HierarchyNode<CompType>::RemoveChild(HierarchyNodeBase* child)
	{
		Children.erase(std::remove_if(Children.begin(), Children.end(), [child](UniquePtr<HierarchyNodeBase>& popup) { return popup.get() == child; }), Children.end());
	}

	template<typename CompType>
	template<typename Archive>
	void HierarchyNode<CompType>::Serialize(Archive& archive)
	{
		std::cout << "^^^ Serializing node.\n";
		archive(CEREAL_NVP(CompT), CEREAL_NVP(CollisionObj), CEREAL_NVP(Children));
	}

	template void HierarchyNode<>::Serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);
	template void HierarchyNode<>::Serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);

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
			CollisionObj = MakeUnique<Physics::CollisionObject>(*node.CollisionObj);
		if (copyChildren)
		{
			Children.reserve(node.Children.size());
			std::transform(node.Children.begin(), node.Children.end(), std::back_inserter(Children), [&tempActor, copyChildren](const UniquePtr<HierarchyNodeBase>& child) { return child->Copy(tempActor, copyChildren); });
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

	template class HierarchyNode<LightComponent>;
}