#pragma once
#include <utility/Utility.h>
#include <string>
#include <animation/Animation.h>

namespace GEE
{
	class Actor;
	class GameScene;
	struct Animation;
	class Mesh;
	class BoneMapping;
	namespace Physics
	{
		struct CollisionShape;
	}

	namespace Hierarchy
	{
		/*
			Meshes loaded from files are placed in a tree structure - 1 tree per 1 file.
			I made it this way because that's the way some file format store their meshes (Assimp, which we use in the project does too).
			Thanks to using the same structure we get an easy access to previously loaded meshes to reuse them (without name conflicts).

			A MeshNode can be instantiated to ModelComponent.
			A ModelComponent is an instance of only one MeshNode, but a MeshNode can be instantiated by multiple ModelComponents.
			Instantiating a MeshNode is associated with instantiating its Meshes and CollisionObject (if there is one).

			In the future you will be able to create your own MeshTree from existing MeshNodes and create your own MeshNodes for the MeshTree from existing Meshes.
		*/

		class NodeBase;

		class HierarchyLocalization
		{
		public:
			static HierarchyLocalization Filepath(const std::string& str)
			{
				return HierarchyLocalization(str, false);
			}
			static HierarchyLocalization LocalResourceName(const std::string& str)
			{
				return HierarchyLocalization(str, true);
			}
			std::string GetPath() const
			{
				return Path;
			}
			bool IsALocalResource() const
			{
				return bLocalResource;
			}
		private:
			HierarchyLocalization(const std::string& path, bool bLocalResource) : Path(path), bLocalResource(bLocalResource) {}

			friend class Tree;
			std::string Path;
			bool bLocalResource;
		};

		

		class Tree
		{
		public:
			Tree(GameScene& scene, const HierarchyLocalization& name);
			Tree(const Tree& tree);
			Tree(Tree&& tree);

			const HierarchyLocalization& GetName() const;
			NodeBase& GetRoot();
			BoneMapping* GetBoneMapping() const;
			Animation& GetAnimation(unsigned int index);
			unsigned int GetAnimationCount() const;
			Actor& GetTempActor();
			GameScene& GetScene() { return Scene; }
			const GameScene& GetScene() const { return Scene; }
			bool ContainsBones() const { return TreeBoneMapping != nullptr; }

			void SetRoot(UniquePtr<NodeBase> root);

			/**
			 * @brief Destroys (clears) the current BoneMapping and allows to create a new one.
			 * @param createNewMapping: if true, a new BoneMapping is created for this tree.
			*/
			
			void ResetBoneMapping(bool createNewMapping = true);

			void AddAnimation(const Animation& anim);
			Mesh* FindMesh(const std::string& nodeName, const std::string& specificMeshName = std::string());
			Physics::CollisionShape* FindTriangleMeshCollisionShape(const std::string& nodeName, const std::string& specificMeshName = std::string());
			Animation* FindAnimation(const std::string& animLoc);

			void RemoveVertsData();
			std::vector<Mesh> GetMeshes();

			template <typename Archive> void Save(Archive& archive) const;
			template <typename Archive> void Load(Archive& archive);

			Tree& operator=(const Tree&) = delete;
			Tree& operator=(Tree&&) = delete;

			~Tree();

			void SetName(const std::string& filepath);
		private:


			HierarchyLocalization Name;	//(this can also be referred to as the path)
			GameScene& Scene;
			UniquePtr<NodeBase> Root;
			UniquePtr<Actor> TempActor;

			std::vector<UniquePtr<Animation>> TreeAnimations;
			mutable UniquePtr<BoneMapping> TreeBoneMapping;
		};
	}
}

namespace cereal
{
	template <> struct LoadAndConstruct<GEE::Hierarchy::Tree>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::Hierarchy::Tree>& construct)
		{
			if (!GEE::CerealTreeSerializationData::TreeScene)
				return;
			
			/* We set the name to serialization-error since it will be replaced by its original name anyways. */
			construct(*GEE::CerealTreeSerializationData::TreeScene, GEE::Hierarchy::HierarchyLocalization::LocalResourceName("serialization-error"));
			construct->Load(ar);
		}
	};																							 													 																			
}