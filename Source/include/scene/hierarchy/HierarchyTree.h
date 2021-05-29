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
	class CollisionShape;

	namespace HierarchyTemplate
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

		class HierarchyNodeBase;

		class HierarchyTreeT
		{
		public:
			HierarchyTreeT(GameScene& scene, const std::string& name);
			HierarchyTreeT(const HierarchyTreeT& tree);
			HierarchyTreeT(HierarchyTreeT&& tree);

			const std::string& GetName() const;
			HierarchyNodeBase& GetRoot();
			BoneMapping& GetBoneMapping() const;
			Animation& GetAnimation(unsigned int index);
			unsigned int GetAnimationCount();

			void SetRoot(std::unique_ptr<HierarchyNodeBase> root);

			void AddAnimation(const Animation& anim);
			Mesh* FindMesh(const std::string& nodeName, const std::string& specificMeshName = std::string());
			CollisionShape* FindTriangleMeshCollisionShape(const std::string& nodeName, const std::string& specificMeshName = std::string());
			Animation* FindAnimation(const std::string& animLoc);

			void RemoveVertsData();
			std::vector<Mesh> GetMeshes();

			~HierarchyTreeT();

		private:
			std::string Name;	//(this can also be referred to as the path)
			GameScene& Scene;
			std::unique_ptr<HierarchyNodeBase> Root;
			std::unique_ptr<Actor> TempActor;

			std::vector<std::unique_ptr<Animation>> TreeAnimations;
			mutable std::unique_ptr<BoneMapping> TreeBoneMapping;
		};
	}
}