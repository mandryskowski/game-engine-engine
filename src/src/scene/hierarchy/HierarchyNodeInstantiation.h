#pragma once
#include <scene/hierarchy/HierarchyTree.h>
#include <game/GameScene.h>

namespace GEE
{
	namespace HierarchyTemplate
	{
		namespace NodeInstantiation
		{
			struct Data
			{
				Data(HierarchyTreeT& tree, bool allowSkeletonsCreation = true) :
					Tree(tree)
				{
					if (allowSkeletonsCreation && Tree.GetBoneMapping())
						SkelInfo = tree.GetScene().GetRenderData()->AddSkeletonInfo().get();
				}
				const HierarchyTreeT& GetTree() { return Tree; }
			private:
				const HierarchyTreeT& Tree;

				SkeletonInfo* SkelInfo;
			};

			template <typename CompType = Component> struct Impl
			{
				static UniquePtr<CompType> ConstructComp(Actor& actor, const std::string& name = "treeInstantiatedComp")
				{
					return MakeUnique<CompType>(actor, nullptr, name);
				}
				static void InstantiateToComp(SharedPtr<NodeInstantiation::Data> data, const HierarchyNode<CompType>& instantiatedNode, CompType& targetComp)
				{
					if (instantiatedNode.GetCollisionObject())
						targetComp.SetCollisionObject(MakeUnique<Physics::CollisionObject>(*instantiatedNode.GetCollisionObject()));
				}

			
			};

			/*template <> UniquePtr<ModelComponent> Impl<ModelComponent>::ConstructComp(Actor& tempActor, const std::string& name)
			{
				auto created = MakeUnique<ModelComponent>(tempActor, nullptr, name);
				tempActor.GetScene().GetRenderData()->EraseRenderable(*created);

				return std::move(created);
			}*/
		}
	}
}