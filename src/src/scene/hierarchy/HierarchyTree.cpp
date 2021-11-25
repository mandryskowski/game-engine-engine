#include <scene/hierarchy/HierarchyTree.h>
#include <scene/hierarchy/HierarchyNode.h>
#include <scene/BoneComponent.h>
#include <rendering/Mesh.h>
#include <animation/Animation.h>
#include <scene/Actor.h>

namespace GEE
{
	using namespace HierarchyTemplate;

	HierarchyTreeT::HierarchyTreeT(GameScene& scene, const std::string& name) :
		Scene(scene),
		Name(name),
		Root(nullptr),
		TempActor(MakeUnique<Actor>(scene, nullptr, name + "TempActor")),
		TreeBoneMapping(nullptr)
	{
		Root = static_unique_pointer_cast<HierarchyNodeBase>(MakeUnique<HierarchyNode<Component>>(*TempActor, name));	//root has the same name as the tree
	}

	HierarchyTreeT::HierarchyTreeT(const HierarchyTreeT& tree) :
		Scene(tree.Scene),
		Name(tree.Name),
		Root(nullptr),
		TempActor(MakeUnique<Actor>(tree.Scene, nullptr, tree.Name + "TempActor")),
		TreeBoneMapping((tree.TreeBoneMapping) ? (MakeUnique<BoneMapping>(*tree.TreeBoneMapping)) : (nullptr))
	{
		if (tree.Root)
			Root = tree.Root->Copy(*TempActor, true);
		else
			Root = static_unique_pointer_cast<HierarchyNodeBase>(MakeUnique<HierarchyNode<Component>>(*TempActor, Name));	//root has the same name as the tree
	}

	HierarchyTreeT::HierarchyTreeT(HierarchyTreeT&& tree) :
		Scene(tree.Scene),
		Name(tree.Name),
		Root(std::move(tree.Root)),
		TempActor(std::move(tree.TempActor)),
		TreeBoneMapping((tree.TreeBoneMapping) ? (std::move(tree.TreeBoneMapping)) : (nullptr))
	{
		if (!Root)
			Root = static_unique_pointer_cast<HierarchyNodeBase>(MakeUnique<HierarchyNode<Component>>(*TempActor, Name));	//root has the same name as the tree
		if (!TempActor)
			TempActor = MakeUnique<Actor>(Scene, nullptr, Name + "TempActor");
	}

	const std::string& HierarchyTreeT::GetName() const
	{
		return Name;
	}

	HierarchyNodeBase& HierarchyTreeT::GetRoot()
	{
		return *Root;
	}

	BoneMapping& HierarchyTreeT::GetBoneMapping() const
	{
		if (!TreeBoneMapping)
			TreeBoneMapping = MakeUnique<BoneMapping>();
		return *TreeBoneMapping;
	}

	Animation& HierarchyTreeT::GetAnimation(unsigned int index)
	{
		return *TreeAnimations[index];
	}

	unsigned int HierarchyTreeT::GetAnimationCount()
	{
		return TreeAnimations.size();
	}

	void HierarchyTreeT::SetRoot(UniquePtr<HierarchyNodeBase> root)
	{
		Root = std::move(root);
	}

	void HierarchyTreeT::AddAnimation(const Animation& anim)
	{
		TreeAnimations.push_back(MakeUnique<Animation>(anim));
	}

	Mesh* HierarchyTreeT::FindMesh(const std::string& nodeName, const std::string& specificMeshName)	//pass empty string to ignore one of the names
	{
		std::function<Mesh* (HierarchyNodeBase&, const std::string&, const std::string&)> findMeshFunc = [&findMeshFunc](HierarchyNodeBase& node, const std::string& nodeName, const std::string& specificMeshName) -> Mesh* {
			if (auto cast = dynamic_cast<HierarchyNode<ModelComponent>*>(&node))
			{
				if (MeshInstance* found = cast->GetCompT().FindMeshInstance(nodeName, specificMeshName))
					return &found->GetMesh();
			}

			for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				if (auto found = findMeshFunc(*node.GetChild(i), nodeName, specificMeshName))
					return found;

			return nullptr;
		};

		Mesh* found = findMeshFunc(*Root, nodeName, specificMeshName);
		if (!found)	//If no mesh was found, perhaps the specific mesh name was put into the node name or vice versa. Search again.
		{
			if (nodeName.empty())
				findMeshFunc(*Root, specificMeshName, specificMeshName);
			else if (specificMeshName.empty())
				findMeshFunc(*Root, nodeName, nodeName);
		}

		return found;
	}

	Physics::CollisionShape* HierarchyTreeT::FindTriangleMeshCollisionShape(const std::string& nodeName, const std::string& specificMeshName)
	{
		std::function<Physics::CollisionShape* (HierarchyNodeBase&, const std::string&, const std::string&)> findShape = [&findShape](HierarchyNodeBase& node, const std::string& meshNodeName, const std::string& meshSpecificName) -> Physics::CollisionShape* {
			if (Physics::CollisionObject* colObject = node.GetCollisionObject())
				if (Physics::CollisionShape* found = colObject->FindTriangleMeshCollisionShape(meshNodeName, meshSpecificName))
					return found;

			for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				if (auto found = findShape(*node.GetChild(i), meshNodeName, meshSpecificName))
					return found;

			return nullptr;
		};


		Physics::CollisionShape* found = findShape(*Root, nodeName, specificMeshName);
		if (!found)	//If no mesh was found, perhaps the specific mesh name was put into the node name or vice versa. Search again.
		{
			if (nodeName.empty())
				findShape(*Root, specificMeshName, specificMeshName);
			else if (specificMeshName.empty())
				findShape(*Root, nodeName, nodeName);
		}

		return found;
	}

	Animation* HierarchyTreeT::FindAnimation(const std::string& animLoc)
	{
		for (auto& it : TreeAnimations)
			if (it->Localization.Name == animLoc)
				return it.get();

		return nullptr;
	}

	void HierarchyTreeT::RemoveVertsData()
	{
		std::function<void(HierarchyNodeBase&)> removeVertsFunc = [&removeVertsFunc](HierarchyNodeBase& node) {
			if (auto cast = dynamic_cast<HierarchyNode<ModelComponent>*>(&node))
			{
				for (int i = 0; i < cast->GetCompT().GetMeshInstanceCount(); i++)
					cast->GetCompT().GetMeshInstance(i).GetMesh().RemoveVertsAndIndicesData();
			}

			for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				removeVertsFunc(*node.GetChild(i));
		};

		removeVertsFunc(*Root);
	}

	std::vector<Mesh> HierarchyTreeT::GetMeshes()
	{
		std::function<void(HierarchyNodeBase&, std::vector<Mesh>&)> getMeshesFunc = [&getMeshesFunc](HierarchyNodeBase& node, std::vector<Mesh>& meshes) {
			if (auto cast = dynamic_cast<HierarchyNode<ModelComponent>*>(&node))
			{
				for (int i = 0; i < cast->GetCompT().GetMeshInstanceCount(); i++)
					meshes.push_back(cast->GetCompT().GetMeshInstance(i).GetMesh());
			}

			for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				getMeshesFunc(*node.GetChild(i), meshes);
		};

		std::vector<Mesh> meshes;
		getMeshesFunc(*Root, meshes);

		return meshes;
	}

	//HierarchyTree::Root points to an incomplete type. We are forced to write a constructor, but we don't really need to write anything in it, so we just keep the default one.
	HierarchyTreeT::~HierarchyTreeT() = default;

}