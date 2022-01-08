#include <scene/hierarchy/HierarchyTree.h>
#include <scene/hierarchy/HierarchyNode.h>
#include <scene/BoneComponent.h>
#include <rendering/Mesh.h>
#include <animation/Animation.h>
#include <scene/Actor.h>

GEE::GameScene* GEE::CerealTreeSerializationData::TreeScene = nullptr;
namespace GEE
{
	using namespace HierarchyTemplate;

	HierarchyTreeT::HierarchyTreeT(GameScene& scene, const HierarchyLocalization& name) :
		Scene(scene),
		Name(name),
		Root(nullptr),
		TempActor(MakeUnique<Actor>(scene, nullptr, name.GetPath() + "TempActor")),
		TreeBoneMapping(nullptr)
	{
		Root = static_unique_pointer_cast<HierarchyNodeBase>(MakeUnique<HierarchyNode<Component>>(*TempActor, name.GetPath()));	//root has the same name as the tree
	}

	HierarchyTreeT::HierarchyTreeT(const HierarchyTreeT& tree) :
		Scene(tree.Scene),
		Name(HierarchyLocalization::LocalResourceName(tree.Name.GetPath() + "_Copy")),
		Root(nullptr),
		TempActor(MakeUnique<Actor>(tree.Scene, nullptr, tree.Name.GetPath() + "TempActor")),
		TreeBoneMapping((tree.TreeBoneMapping) ? (MakeUnique<BoneMapping>(*tree.TreeBoneMapping)) : (nullptr))
	{
		if (tree.Root)
			Root = tree.Root->Copy(*TempActor, true);
		else
			Root = static_unique_pointer_cast<HierarchyNodeBase>(MakeUnique<HierarchyNode<Component>>(*TempActor, Name.GetPath()));	//root has the same name as the tree

		TreeAnimations.reserve(tree.TreeAnimations.size());
		std::transform(tree.TreeAnimations.begin(), tree.TreeAnimations.end(), std::back_inserter(TreeAnimations), [](const UniquePtr<Animation>& anim) { return MakeUnique<Animation>(*anim); });
	}

	HierarchyTreeT::HierarchyTreeT(HierarchyTreeT&& tree) :
		Scene(tree.Scene),
		Name(tree.Name),
		Root(std::move(tree.Root)),
		TempActor(std::move(tree.TempActor)),
		TreeBoneMapping((tree.TreeBoneMapping) ? (std::move(tree.TreeBoneMapping)) : (nullptr)),
		TreeAnimations(std::move(tree.TreeAnimations))
	{
		if (!Root)
			Root = static_unique_pointer_cast<HierarchyNodeBase>(MakeUnique<HierarchyNode<Component>>(*TempActor, Name.GetPath()));	//root has the same name as the tree
		if (!TempActor)
			TempActor = MakeUnique<Actor>(Scene, nullptr, Name.GetPath() + "TempActor");
	}

	const HierarchyLocalization& HierarchyTreeT::GetName() const
	{
		return Name;
	}

	HierarchyNodeBase& HierarchyTreeT::GetRoot()
	{
		return *Root;
	}

	BoneMapping* HierarchyTreeT::GetBoneMapping() const
	{
		return TreeBoneMapping.get();
	}

	Animation& HierarchyTreeT::GetAnimation(unsigned int index)
	{
		return *TreeAnimations[index];
	}

	unsigned int HierarchyTreeT::GetAnimationCount()
	{
		return TreeAnimations.size();
	}

	Actor& HierarchyTemplate::HierarchyTreeT::GetTempActor()
	{
		return *TempActor;
	}

	void HierarchyTreeT::SetRoot(UniquePtr<HierarchyNodeBase> root)
	{
		Root = std::move(root);
	}

	void HierarchyTemplate::HierarchyTreeT::ResetBoneMapping(bool createNewMapping)
	{
		TreeBoneMapping = (createNewMapping) ? (MakeUnique<BoneMapping>()) : (nullptr);
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

	template<typename Archive>
	void HierarchyTreeT::Save(Archive& archive) const
	{
		archive(cereal::make_nvp("LocalResourcePath", Name.GetPath()), CEREAL_NVP(Root));
	}

	template<typename Archive>
	void HierarchyTreeT::Load(Archive& archive)
	{
		std::cout << "^^^ Loading tree\n";
		std::string localPath;
		CerealNodeSerializationData::TempActor = &GetTempActor();
		archive(cereal::make_nvp("LocalResourcePath", localPath), CEREAL_NVP(Root));
		CerealNodeSerializationData::TempActor = nullptr;
		Name = HierarchyLocalization(localPath, true);
		std::cout << "vvv Finished loading tree\n";
	}

	template void HierarchyTreeT::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
	template void HierarchyTreeT::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

	//HierarchyTree::Root points to an incomplete type. We are forced to define a constructor, but we don't really need to write anything in it, so we just keep the default one.
	HierarchyTreeT::~HierarchyTreeT() = default;

	void HierarchyTemplate::HierarchyTreeT::SetName(const std::string& filepath)
	{
		Name = HierarchyLocalization::LocalResourceName(filepath);
	}

}