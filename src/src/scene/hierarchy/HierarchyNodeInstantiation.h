#pragma once
#include <scene/hierarchy/HierarchyTree.h>
#include <scene/hierarchy/HierarchyNodeBase.h>
#include <scene/Component.h>

namespace GEE
{
	namespace Hierarchy
	{
		namespace Instantiation
		{
			struct Data
			{
				Data(Tree& tree);
				const Tree& GetTree() { return _Tree; }
				SkeletonInfo* GetSkeletonInfo() { return SkelInfo; }

			private:
				const Tree& _Tree;
				SkeletonInfo* SkelInfo;

				friend struct TreeInstantiation;
			};

			/**
			 * @brief Instantiate a Tree to another hierarchical structure (e.g. actor-component). This way the hierarchy is retained.
			 * @param  
			*/

			struct TreeInstantiation
			{
				TreeInstantiation(Tree& tree, bool allowSkeletonsCreation);
				void ToComponents(NodeBase&, Component& parentComp, const std::vector<Hierarchy::NodeBase*>& limitToComponents);
				void ToComponentsAndRoot(NodeBase&, Actor& actorToReplaceRoot, const std::vector<Hierarchy::NodeBase*>& limitToComponents);


				void ToActors(NodeBase&, Actor& parentActor, std::function<Actor& (Actor&)> constructChildActorFunc, const std::vector<Hierarchy::NodeBase*>& limitToComponents);

				void PostInstantiation(Component& instantiationRoot);

				struct Implementation
				{
					Component& ToComponentsImpl(SharedPtr<Data>, NodeBase&, Component& parentComp, const std::vector<Hierarchy::NodeBase*>& limitToComponents);
					Actor& ToActorsImpl(SharedPtr<Data>, NodeBase&, Actor& parentActor, std::function<Actor& (Actor&)> constructChildActorFunc, const std::vector<Hierarchy::NodeBase*>& limitToComponents);
				} Impl;
			private:
				SharedPtr<Data> TreeData;
			};


			template <typename CompType = Component> struct Impl
			{
				static UniquePtr<CompType> ConstructComp(Actor& actor, const std::string& name = "treeInstantiatedComp")
				{
					return MakeUnique<CompType>(actor, nullptr, name);
				}
				static void InstantiateToComp(SharedPtr<Instantiation::Data> data, const NodeBase& instantiatedNode, CompType& targetComp)
				{
					if (instantiatedNode.GetCollisionObject())
						targetComp.SetCollisionObject(MakeUnique<Physics::CollisionObject>(*instantiatedNode.GetCollisionObject()));
				}

			
			};
		}
	}
}