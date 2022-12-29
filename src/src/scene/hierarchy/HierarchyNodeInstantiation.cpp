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
			void TreeInstantiation::ToComponents(NodeBase& node, Component& parentComp, const std::vector<Hierarchy::NodeBase*>& limitToComponents = std::vector<Hierarchy::NodeBase*>())
			{
				PostInstantiation(Impl.ToComponentsImpl(TreeData, node, parentComp, limitToComponents));
			}
			void TreeInstantiation::ToComponentsAndRoot(NodeBase& node, Actor& actorToReplaceRoot, const std::vector<Hierarchy::NodeBase*>& limitToComponents = std::vector<Hierarchy::NodeBase*>())
			{
				actorToReplaceRoot.ReplaceRoot(node.GenerateComp(TreeData, actorToReplaceRoot));
				auto& generatedComp = *actorToReplaceRoot.GetRoot();

				for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
					Impl.ToComponentsImpl(TreeData, *node.GetChild(i), generatedComp, limitToComponents);

				PostInstantiation(generatedComp);

			}
			void TreeInstantiation::ToActors(NodeBase& node, Actor& parentActor, std::function<Actor& (Actor&)> constructChildActorFunc, const std::vector<Hierarchy::NodeBase*>& limitToActors = std::vector<Hierarchy::NodeBase*>())
			{
				PostInstantiation(*Impl.ToActorsImpl(TreeData, node, parentActor, constructChildActorFunc, limitToActors).GetRoot());
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
			Component& TreeInstantiation::Implementation::ToComponentsImpl(SharedPtr<Data> data, NodeBase& node, Component& parentComp, const std::vector<Hierarchy::NodeBase*>& limitToComponents = std::vector<Hierarchy::NodeBase*>())
			{
				bool bGenComp = true;
				if (!limitToComponents.empty())
				{
					auto found = std::find_if(limitToComponents.begin(), limitToComponents.end(), [&node](const Hierarchy::NodeBase* nodeVec) { return nodeVec == &node; });
					if (found == limitToComponents.end())
						bGenComp = false;
				}

				auto generatedComp = (bGenComp) ? (&parentComp.AddComponent(node.GenerateComp(data, parentComp.GetActor()))) : (nullptr);

				for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
				{
					auto childNode = node.GetChild(i);

					ToComponentsImpl(data, *childNode, (generatedComp) ? (*generatedComp) : (parentComp), limitToComponents);
				}

				return (generatedComp) ? (*generatedComp) : (parentComp);
			}
			Actor& TreeInstantiation::Implementation::ToActorsImpl(SharedPtr<Data> data, NodeBase& node, Actor& parentActor, std::function<Actor& (Actor&)> constructChildActorFunc, const std::vector<Hierarchy::NodeBase*>& limitToActors = std::vector<Hierarchy::NodeBase*>())
			{
				bool bGenActor = true;
				if (!limitToActors.empty())
				{
					auto found = std::find_if(limitToActors.begin(), limitToActors.end(), [&node](const Hierarchy::NodeBase* nodeVec) { return nodeVec == &node; });
					if (found == limitToActors.end())
						bGenActor = false;
				}

				Actor* generatedActor = nullptr;
				if (bGenActor)
				{
					generatedActor = &constructChildActorFunc(parentActor);
					generatedActor->ReplaceRoot(node.GenerateComp(data, *generatedActor));
					if (generatedActor->GetName().empty()) generatedActor->SetName(generatedActor->GetRoot()->GetName());
				}

				for (int i = 0; i < static_cast<int>(node.GetChildCount()); i++)
					ToActorsImpl(data, *node.GetChild(i), (generatedActor) ? (*generatedActor) : (parentActor), constructChildActorFunc, limitToActors);

				return (generatedActor) ? (*generatedActor) : (parentActor);
			}
		}
	}
}