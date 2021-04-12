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

class Mesh;
struct CollisionObject;
struct CollisionShape;
class Material;

/*
	Meshes loaded from files are placed in a tree structure - 1 tree per 1 file.
	I made it this way because that's the way some file format store their meshes (Assimp, which we use in the project does too).
	Thanks to using the same structure we get an easy access to previously loaded meshes to reuse them (without name conflicts).

	A MeshNode can be instantiated to ModelComponent.
	A ModelComponent is an instance of only one MeshNode, but a MeshNode can be instantiated by multiple ModelComponents.
	Instantiating a MeshNode is associated with instantiating its Meshes and CollisionObject (if there is one).

	In the future you will be able to create your own MeshTree from existing MeshNodes and create your own MeshNodes for the MeshTree from existing Meshes.
*/

namespace HierarchyTemplate
{
	template <typename CompType> class HierarchyNode;
	class HierarchyNodeBase
	{
	public:
		virtual Component& GetCompBaseType() = 0;
		virtual const Component& GetCompBaseType() const = 0;
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

		virtual void SetCollisionObject(std::unique_ptr<CollisionObject>) = 0;
		virtual void AddCollisionShape(std::shared_ptr<CollisionShape> shape) = 0;
	};

	template <typename CompType = Component> class HierarchyNode : public HierarchyNodeBase
	{
	public:
		HierarchyNode(Actor& tempActor, const std::string& name) :
			CompT(CompType(tempActor, nullptr, name)) { }
		HierarchyNode(const HierarchyNode<CompType>& node, Actor& tempActor, bool copyChildren = false) :
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
		HierarchyNode(HierarchyNode<CompType>&& node, Actor& tempActor, bool copyChildren = true) :
			HierarchyNode(static_cast<const HierarchyNode<CompType>&>(node), tempActor, copyChildren)
		{
		}
		HierarchyNode(const HierarchyNode<CompType>&) = delete;
		HierarchyNode(HierarchyNode<CompType>&&) = delete;
		std::unique_ptr<CompType> Instantiate(Component& parent, const std::string& name) const
		{
			InstantiateToComp(parent.CreateComponent<CompType>(CompType(parent.GetScene(), name)));
		}
		virtual	void InstantiateToComp(Component& comp) const override
		{
			CompType* compCast = dynamic_cast<CompType*>(&comp);

			if (compCast)
				*compCast = CompT;
			else
				comp = CompT;

			if (CollisionObj)
				comp.SetCollisionObject(std::make_unique<CollisionObject>(*CollisionObj));
		}
		virtual HierarchyNodeBase* GetChild(unsigned int index) const
		{
			if (index > Children.size() - 1)
				return nullptr;

			return Children[index].get();
		}
		virtual unsigned int GetChildCount() const
		{
			return Children.size();
		}
		CompType& GetCompT()
		{
			return CompT;
		}
		const CompType& GetCompT() const
		{
			return CompT;
		}
		virtual Component& GetCompBaseType() override
		{
			return CompT;
		}
		virtual const Component& GetCompBaseType() const override
		{
			return static_cast<const Component&>(CompT);
		}
		virtual HierarchyNodeBase& AddChild(std::unique_ptr<HierarchyNodeBase> child) override
		{
			Children.push_back(std::move(child));
			return *Children.back().get();
		}
		virtual HierarchyNodeBase* FindNode(const std::string& name)
		{
			if (GetCompBaseType().GetName() == name)
				return this;

			for (auto& it : Children)
				if (auto found = it->FindNode(name))
					return found;

			return nullptr;
		}
		virtual std::unique_ptr<HierarchyNodeBase> Copy(Actor& tempActor, bool copyChildren = false) const override
		{
			return static_unique_pointer_cast<HierarchyNodeBase>(std::make_unique<HierarchyNode<CompType>>(*this, tempActor, copyChildren));
		}

		virtual void SetCollisionObject(std::unique_ptr<CollisionObject> collisionObj) override
		{
			CollisionObj = std::move(collisionObj);
		}
		virtual void AddCollisionShape(std::shared_ptr<CollisionShape> shape) override
		{
			if (!CollisionObj)
				CollisionObj = std::make_unique<CollisionObject>();

			CollisionObj->AddShape(shape);
		}

	private:
		//friend CompType& CompType::operator=(const ComponentTemplate<CompType>&);
		std::vector<std::unique_ptr<HierarchyNodeBase>> Children;
		CompType CompT;
		std::unique_ptr<CollisionObject> CollisionObj;
	};

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

	class HierarchyTreeT
	{
	public:
		HierarchyTreeT(GameScene& scene, const std::string& name) :
			Scene(scene),
			Name(name),
			Root(nullptr),
			TempActor(std::make_unique<Actor>(scene, name + "TempActor")),
			TreeBoneMapping(nullptr)
		{
			Root = static_unique_pointer_cast<HierarchyNodeBase>(std::make_unique<HierarchyNode<Component>>(*TempActor, name));	//root has the same name as the tree
		}
		HierarchyTreeT(const HierarchyTreeT& tree) :
			Scene(tree.Scene),
			Name(tree.Name),
			Root(nullptr),
			TempActor(std::make_unique<Actor>(tree.Scene, tree.Name + "TempActor")),
			TreeBoneMapping((tree.TreeBoneMapping) ? (std::make_unique<BoneMapping>(*tree.TreeBoneMapping)) : (nullptr))
		{
			if (tree.Root)
				Root = tree.Root->Copy(*TempActor, true);
			else
				Root = static_unique_pointer_cast<HierarchyNodeBase>(std::make_unique<HierarchyNode<Component>>(*TempActor, Name));	//root has the same name as the tree
		}
		HierarchyTreeT(HierarchyTreeT&& tree) :
			Scene(tree.Scene),
			Name(tree.Name),
			Root(std::move(tree.Root)),
			TempActor(std::move(tree.TempActor)),
			TreeBoneMapping((tree.TreeBoneMapping) ? (std::move(tree.TreeBoneMapping)) : (nullptr))
		{
			if (!Root)
				Root = static_unique_pointer_cast<HierarchyNodeBase>(std::make_unique<HierarchyNode<Component>>(*TempActor, Name));	//root has the same name as the tree
			if (!TempActor)
				TempActor = std::make_unique<Actor>(Scene, Name + "TempActor");
		}
		const std::string& GetName() const
		{
			return Name;
		}
		HierarchyNodeBase& GetRoot()
		{
			return *Root;
		}
		BoneMapping& GetBoneMapping() const
		{
			if (!TreeBoneMapping)
				TreeBoneMapping = std::make_unique<BoneMapping>();
			return *TreeBoneMapping;
		}
		Animation& GetAnimation(unsigned int index)
		{
			return *TreeAnimations[index];
		}
		unsigned int GetAnimationCount()
		{
			return TreeAnimations.size();
		}
		void SetRoot(std::unique_ptr<HierarchyNodeBase> root)
		{
			Root = std::move(root);
		}
		void AddAnimation(const Animation& anim)
		{
			TreeAnimations.push_back(std::make_unique<Animation>(Animation(anim)));
		}
		Mesh* FindMesh(const std::string& name)
		{
			std::function<Mesh* (HierarchyNodeBase&, const std::string&)> findMeshFunc = [&findMeshFunc](HierarchyNodeBase& node, const std::string& meshName) -> Mesh* {
				if (auto cast = dynamic_cast<HierarchyNode<ModelComponent>*>(&node))
				{
					if (MeshInstance* found = cast->GetCompT().FindMeshInstance(meshName))
						return const_cast<Mesh*>(&found->GetMesh());
				}

				for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
					if (auto found = findMeshFunc(*node.GetChild(i), meshName))
						return found;

				return nullptr;
			};


			return findMeshFunc(*Root, name);
		}
	private:
		std::string Name;	//(Path)
		GameScene& Scene;
		std::unique_ptr<HierarchyNodeBase> Root;
		std::unique_ptr<Actor> TempActor;

		std::vector<std::unique_ptr<Animation>> TreeAnimations;
		mutable std::unique_ptr<BoneMapping> TreeBoneMapping;
	};

	template <typename ActorType> class ActorTemplate : public ActorType
	{

	};
}

bool isInteger(std::string);	//works for int() >= 0