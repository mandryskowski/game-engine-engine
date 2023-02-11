#include <scene/hierarchy/HierarchyTree.h>
#include <scene/hierarchy/HierarchyNode.h>
#include <scene/BoneComponent.h>
#include <rendering/Mesh.h>
#include <animation/Animation.h>
#include <scene/Actor.h>

GEE::GameScene* GEE::CerealTreeSerializationData::TreeScene = nullptr;
namespace GEE
{
	using namespace Hierarchy;

	Tree::Tree(GameScene& scene, const HierarchyLocalization& name) :
		Scene(scene),
		Name(name),
		Root(nullptr),
		TempActor(MakeUnique<Actor>(scene, nullptr, name.GetPath() + "TempActor")),
		TreeBoneMapping(nullptr)
	{
		Root = static_unique_pointer_cast<NodeBase>(MakeUnique<Node<Component>>(*TempActor, name.GetPath()));	//root has the same name as the tree
	}

	Tree::Tree(const Tree& tree) :
		Scene(tree.Scene),
		Name(HierarchyLocalization::LocalResourceName(tree.Name.GetPath() + "_Copy")),
		Root(nullptr),
		TempActor(MakeUnique<Actor>(tree.Scene, nullptr, tree.Name.GetPath() + "TempActor")),
		TreeBoneMapping((tree.TreeBoneMapping) ? (MakeUnique<BoneMapping>(*tree.TreeBoneMapping)) : (nullptr))
	{
		if (tree.Root)
			Root = tree.Root->Copy(*TempActor, true);
		else
			Root = static_unique_pointer_cast<NodeBase>(MakeUnique<Node<Component>>(*TempActor, Name.GetPath()));	//root has the same name as the tree

		TreeAnimations.reserve(tree.TreeAnimations.size());
		std::transform(tree.TreeAnimations.begin(), tree.TreeAnimations.end(), std::back_inserter(TreeAnimations), [](const UniquePtr<Animation>& anim) { return MakeUnique<Animation>(*anim); });
	}

	Tree::Tree(Tree&& tree) :
		Scene(tree.Scene),
		Name(tree.Name),
		Root(std::move(tree.Root)),
		TempActor(std::move(tree.TempActor)),
		TreeBoneMapping((tree.TreeBoneMapping) ? (std::move(tree.TreeBoneMapping)) : (nullptr)),
		TreeAnimations(std::move(tree.TreeAnimations))
	{
		if (!Root)
			Root = static_unique_pointer_cast<NodeBase>(MakeUnique<Node<Component>>(*TempActor, Name.GetPath()));	//root has the same name as the tree
		if (!TempActor)
			TempActor = MakeUnique<Actor>(Scene, nullptr, Name.GetPath() + "TempActor");
	}

	const HierarchyLocalization& Tree::GetName() const
	{
		return Name;
	}

	NodeBase& Tree::GetRoot()
	{
		return *Root;
	}

	BoneMapping* Tree::GetBoneMapping() const
	{
		return TreeBoneMapping.get();
	}

	Animation& Tree::GetAnimation(unsigned int index)
	{
		return *TreeAnimations[index];
	}

	unsigned int Tree::GetAnimationCount() const
	{
		return static_cast<unsigned int>(TreeAnimations.size());
	}

	Actor& Hierarchy::Tree::GetTempActor()
	{
		return *TempActor;
	}

	void Tree::SetRoot(UniquePtr<NodeBase> root)
	{
		Root = std::move(root);
	}

	void Hierarchy::Tree::ResetBoneMapping(bool createNewMapping)
	{
		TreeBoneMapping = (createNewMapping) ? (MakeUnique<BoneMapping>()) : (nullptr);
	}

	void Tree::AddAnimation(const Animation& anim)
	{
		TreeAnimations.push_back(MakeUnique<Animation>(anim));
	}

	Mesh* Tree::FindMesh(const std::string& nodeName, const std::string& specificMeshName)	//pass empty string to ignore one of the names
	{
		std::function<Mesh* (NodeBase&, const std::string&, const std::string&)> findMeshFunc = [&findMeshFunc](NodeBase& node, const std::string& nodeName, const std::string& specificMeshName) -> Mesh* {
			if (auto cast = dynamic_cast<Node<ModelComponent>*>(&node))
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

	Physics::CollisionShape* Tree::FindTriangleMeshCollisionShape(const std::string& nodeName, const std::string& specificMeshName)
	{
		std::function<Physics::CollisionShape* (NodeBase&, const std::string&, const std::string&)> findShape = [&findShape](NodeBase& node, const std::string& meshNodeName, const std::string& meshSpecificName) -> Physics::CollisionShape* {
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

	Animation* Tree::FindAnimation(const std::string& animLoc)
	{
		for (auto& it : TreeAnimations)
			if (it->Localization.Name == animLoc)
				return it.get();

		return nullptr;
	}

	void Tree::RemoveVertsData()
	{
		std::function<void(NodeBase&)> removeVertsFunc = [&removeVertsFunc](NodeBase& node) {
			if (auto cast = dynamic_cast<Node<ModelComponent>*>(&node))
			{
				for (int i = 0; i < static_cast<int>(cast->GetCompT().GetMeshInstanceCount()); i++)
					cast->GetCompT().GetMeshInstance(i).GetMesh().RemoveVertsAndIndicesData();
			}

			for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				removeVertsFunc(*node.GetChild(i));
		};

		removeVertsFunc(*Root);
	}

	std::vector<Mesh> Tree::GetMeshes()
	{
		std::function<void(NodeBase&, std::vector<Mesh>&)> getMeshesFunc = [&getMeshesFunc](NodeBase& node, std::vector<Mesh>& meshes) {
			if (auto cast = dynamic_cast<Node<ModelComponent>*>(&node))
			{
				for (int i = 0; i < static_cast<int>(cast->GetCompT().GetMeshInstanceCount()); i++)
					meshes.push_back(cast->GetCompT().GetMeshInstance(i).GetMesh());
			}

			for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				getMeshesFunc(*node.GetChild(i), meshes);
		};

		std::vector<Mesh> meshes;
		getMeshesFunc(*Root, meshes);

		return meshes;
	}

	template<typename Archive>
	void Tree::Save(Archive& archive) const
	{
		archive(cereal::make_nvp("LocalResourcePath", Name.GetPath()), CEREAL_NVP(Root));
	}

	template<typename Archive>
	void Tree::Load(Archive& archive)
	{
		std::cout << "^^^ Loading tree\n";
		std::string localPath;
		CerealNodeSerializationData::TempActor = &GetTempActor();
		archive(cereal::make_nvp("LocalResourcePath", localPath), CEREAL_NVP(Root));
		CerealNodeSerializationData::TempActor = nullptr;
		Name = HierarchyLocalization(localPath, true);
		std::cout << "vvv Finished loading tree\n";
	}

	template void Tree::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
	template void Tree::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

	//HierarchyTree::Root points to an incomplete type. We are forced to define a constructor, but we don't really need to write anything in it, so we just keep the default one.
	Tree::~Tree() = default;

	void Hierarchy::Tree::SetName(const std::string& filepath)
	{
		Name = HierarchyLocalization::LocalResourceName(filepath);
	}

}