#include <game/GameManager.h>
#include <scene/hierarchy/HierarchyTree.h>

namespace GEE
{
	GameScene* GameManager::DefaultScene = nullptr;
	GameManager* GameManager::GamePtr = nullptr;

	bool HTreeObjectLoc::IsValidTreeElement() const
	{
		return TreePtr != nullptr;
	}

	std::string HTreeObjectLoc::GetTreeName() const
	{
		if (!IsValidTreeElement())
			return std::string();

		return TreePtr->GetName().GetPath();
	}
	GameManager& GameManager::Get()
	{
		return *GamePtr;
	}
}