#include "HierarchyNodeInstantiation.h"
#include <animation/AnimationManagerComponent.h>
#include <animation/SkeletonInfo.h>
#include <game/GameScene.h>

namespace GEE
{
	namespace Hierarchy
	{
		namespace Instantiation
		{
			Data::Data(Tree& tree) :
				_Tree(tree),
				SkelInfo(nullptr)
			{
			}
			TreeInstantiation::TreeInstantiation(Tree& tree, bool allowSkeletonsCreation):
				TreeData(MakeShared<Data>(tree))
			{
				if (allowSkeletonsCreation && tree.GetBoneMapping())
					TreeData->SkelInfo = tree.GetScene().GetRenderData()->AddSkeletonInfo().get();
			}
			void TreeInstantiation::ToComponents(NodeBase& node, Component& parentComp, const std::vector<Hierarchy::NodeBase*>& limitToComponents)
			{
				PostInstantiation(Impl.ToComponentsImpl(TreeData, node, parentComp, limitToComponents));
			}
			void TreeInstantiation::ToComponentsAndRoot(NodeBase& node, Actor& actorToReplaceRoot, const std::vector<Hierarchy::NodeBase*>& limitToComponents)
			{
				actorToReplaceRoot.ReplaceRoot(node.GenerateComp(TreeData, actorToReplaceRoot));
				auto& generatedComp = *actorToReplaceRoot.GetRoot();

				for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
					Impl.ToComponentsImpl(TreeData, *node.GetChild(i), generatedComp, limitToComponents);

				PostInstantiation(generatedComp);

			}
			void TreeInstantiation::ToActors(NodeBase& node, Actor& parentActor, std::function<Actor& (Actor&)> constructChildActorFunc, const std::vector<Hierarchy::NodeBase*>& limitToComponents)
			{
				PostInstantiation(*Impl.ToActorsImpl(TreeData, node, parentActor, constructChildActorFunc, limitToComponents).GetRoot());
			}
			void TreeInstantiation::PostInstantiation(Component& instantiationRoot)
			{
				const auto& tree = TreeData->GetTree();
				auto skelInfo = TreeData->GetSkeletonInfo();

				if (skelInfo)
				{
					skelInfo->SetGlobalInverseTransformCompPtr(&instantiationRoot);
					skelInfo->SortBones();

					if (tree.GetAnimationCount() > 0)
					{
						AnimationManagerComponent& animManager = instantiationRoot.CreateComponent<AnimationManagerComponent>("An AnimationManagerComponent");
						for (int i = 0; i < tree.GetAnimationCount(); i++)
							animManager.AddAnimationInstance(AnimationInstance(const_cast<Tree&>(tree).GetAnimation(i), instantiationRoot));
					}

					skelInfo->GetBatchPtr()->RecalculateBoneCount();
				}
			}
			Component& TreeInstantiation::Implementation::ToComponentsImpl(SharedPtr<Data> data, NodeBase& node, Component& parentComp, const std::vector<Hierarchy::NodeBase*>& limitToComponents)
			{
				auto& generatedComp = parentComp.AddComponent(node.GenerateComp(data, parentComp.GetActor()));

				for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				{
					ToComponentsImpl(data, *node.GetChild(i), generatedComp, limitToComponents);
				}

				return generatedComp;
			}
			Actor& TreeInstantiation::Implementation::ToActorsImpl(SharedPtr<Data> data, NodeBase& node, Actor& parentActor, std::function<Actor& (Actor&)> constructChildActorFunc, const std::vector<Hierarchy::NodeBase*>& limitToComponents)
			{
				auto& generatedActor = constructChildActorFunc(parentActor);
				generatedActor.ReplaceRoot(node.GenerateComp(data, generatedActor));
				auto& generatedComp = *generatedActor.GetRoot();

				for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
					ToActorsImpl(data, *node.GetChild(i), generatedActor, constructChildActorFunc, limitToComponents);

				return generatedActor;
			}
		}
	}
}