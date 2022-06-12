#include <scene/hierarchy/HierarchyNode.h>
#include <scene/hierarchy/HierarchyNodeInstantiation.h>
#include <game/GameScene.h>
#include <scene/Actor.h>
#include <scene/LightComponent.h>

GEE::Actor* GEE::CerealNodeSerializationData::TempActor = nullptr;
namespace GEE
{
	using namespace Hierarchy;

	template <typename CompType>
	Node<CompType>::Node(Actor& tempActor, const std::string& name) :
		CompT(CompType(tempActor, nullptr, name))
	{ }

	template<typename CompType>
	Hierarchy::Node<CompType>::Node(const Node<CompType>& node, Actor& tempActor, bool copyChildren) :
		CompT(CompType(tempActor, nullptr, node.GetCompT().GetName()))
	{
		CompT = node.CompT;
		if (node.CollisionObj)
			CollisionObj = MakeUnique<Physics::CollisionObject>(*node.CollisionObj);
		if (copyChildren)
		{
			Children.reserve(node.Children.size());
			std::transform(node.Children.begin(), node.Children.end(), std::back_inserter(Children), [&tempActor, copyChildren](const UniquePtr<NodeBase>& child) { return child->Copy(tempActor, copyChildren); });
		}
	}

	template<typename CompType>
	Hierarchy::Node<CompType>::Node(Node<CompType>&& node, Actor& tempActor, bool copyChildren) :
		Node(static_cast<const Node<CompType>&>(node), tempActor, copyChildren)
	{
	}

	/*
	template<typename CompType>
	UniquePtr<CompType> Hierarchy::Node<CompType>::Instantiate(Component& parent, const std::string& name) const
	{
		InstantiateToComp(parent.CreateComponent<CompType>(name));
	}*/

	template<typename CompType>
	void Hierarchy::Node<CompType>::InstantiateToComp(SharedPtr<Instantiation::Data> instData, Component& comp) const
	{
		CompType* compCast = dynamic_cast<CompType*>(&comp);

		if (compCast)
			*compCast = CompT;
		else
			comp = CompT;

		if (compCast)
			Instantiation::Impl<CompType>::InstantiateToComp(instData, *this, *compCast);
	}

	template<typename CompType>
	UniquePtr<Component> Hierarchy::Node<CompType>::GenerateComp(SharedPtr<Instantiation::Data> instData, Actor& compActor) const
	{
		UniquePtr<CompType> comp = Instantiation::Impl<CompType>::ConstructComp(compActor);
		InstantiateToComp(instData, *comp);

		return static_unique_pointer_cast<Component, CompType>(std::move(comp));
	}

	template<typename CompType>
	NodeBase* Hierarchy::Node<CompType>::GetChild(unsigned int index) const
	{
		if (index > Children.size() - 1)
			return nullptr;

		return Children[index].get();
	}

	template<typename CompType>
	unsigned int Hierarchy::Node<CompType>::GetChildCount() const
	{
		return Children.size();
	}

	template<typename CompType>
	CompType& Hierarchy::Node<CompType>::GetCompT()
	{
		return CompT;
	}

	template<typename CompType>
	const CompType& Hierarchy::Node<CompType>::GetCompT() const
	{
		return CompT;
	}

	template<typename CompType>
	Component& Hierarchy::Node<CompType>::GetCompBaseType()
	{
		return CompT;
	}

	template<typename CompType>
	const Component& Hierarchy::Node<CompType>::GetCompBaseType() const
	{
		return static_cast<const Component&>(CompT);
	}

	template<typename CompType>
	Physics::CollisionObject* Hierarchy::Node<CompType>::GetCollisionObject()
	{
		return CollisionObj.get();
	}

	template<typename CompType>
	const Physics::CollisionObject* Hierarchy::Node<CompType>::GetCollisionObject() const
	{
		return CollisionObj.get();
	}

	template<typename CompType>
	NodeBase& Hierarchy::Node<CompType>::AddChild(UniquePtr<NodeBase> child)
	{
		child->SetParent(this);
		Children.push_back(std::move(child));
		return *Children.back().get();
	}

	template<typename CompType>
	NodeBase* Hierarchy::Node<CompType>::FindNode(const std::string& name)
	{
		if (GetCompBaseType().GetName() == name)
			return this;

		for (auto& it : Children)
			if (auto found = it->FindNode(name))
				return found;

		return nullptr;
	}

	template<typename CompType>
	UniquePtr<NodeBase> Hierarchy::Node<CompType>::Copy(Actor& tempActor, bool copyChildren) const
	{
		return static_unique_pointer_cast<NodeBase>(MakeUnique<Node<CompType>>(*this, tempActor, copyChildren));
	}

	template<typename CompType>
	Physics::CollisionObject* Hierarchy::Node<CompType>::SetCollisionObject(UniquePtr<Physics::CollisionObject> collisionObj)
	{
		CollisionObj = std::move(collisionObj);
		return CollisionObj.get();
	}

	template<typename CompType>
	void Hierarchy::Node<CompType>::AddCollisionShape(SharedPtr<Physics::CollisionShape> shape)
	{
		if (!CollisionObj)
			CollisionObj = MakeUnique<Physics::CollisionObject>();

		CollisionObj->AddShape(shape);
	}

	template<typename CompType>
	void Hierarchy::Node<CompType>::Delete()
	{
		GEE_CORE_ASSERT(Parent);
		Parent->RemoveChild(this);
	}

	template<typename CompType>
	void Hierarchy::Node<CompType>::SetParent(NodeBase* parent)
	{
		Parent = parent;
	}

	template<typename CompType>
	void Hierarchy::Node<CompType>::RemoveChild(NodeBase* child)
	{
		Children.erase(std::remove_if(Children.begin(), Children.end(), [child](UniquePtr<NodeBase>& popup) { return popup.get() == child; }), Children.end());
	}

	template<typename CompType>
	template<typename Archive>
	void Node<CompType>::Serialize(Archive& archive)
	{
		std::cout << "^^^ Serializing node.\n";
		archive(CEREAL_NVP(CompT), CEREAL_NVP(CollisionObj), CEREAL_NVP(Children));
	}

	template void Node<>::Serialize<cereal::JSONInputArchive>(cereal::JSONInputArchive&);
	template void Node<>::Serialize<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&);

	template <> Node<ModelComponent>::Node(Actor& tempActor, const std::string& name) :
		CompT(ModelComponent(tempActor, nullptr, name))
	{
		tempActor.GetScene().GetRenderData()->EraseRenderable(CompT);
	}

	template <> Node<ModelComponent>::Node(const Node<ModelComponent>& node, Actor& tempActor, bool copyChildren) :
		CompT(ModelComponent(tempActor, nullptr, node.GetCompT().GetName()))
	{
		CompT = node.CompT;
		if (node.CollisionObj)
			CollisionObj = MakeUnique<Physics::CollisionObject>(*node.CollisionObj);
		if (copyChildren)
		{
			Children.reserve(node.Children.size());
			std::transform(node.Children.begin(), node.Children.end(), std::back_inserter(Children), [&tempActor, copyChildren](const UniquePtr<NodeBase>& child) { return child->Copy(tempActor, copyChildren); });
		}
		CompT.GetScene().GetRenderData()->EraseRenderable(CompT);
	}
	template <> Node<ModelComponent>::Node(Node<ModelComponent>&& node, Actor& tempActor, bool copyChildren) :
		Node(static_cast<const Node<ModelComponent>&>(node), tempActor, copyChildren)
	{
		CompT.GetScene().GetRenderData()->EraseRenderable(CompT);
	}


	template class Node<ModelComponent>;
	template class Node<BoneComponent>;
	template class Node<Component>;

	template class Node<LightComponent>;
}