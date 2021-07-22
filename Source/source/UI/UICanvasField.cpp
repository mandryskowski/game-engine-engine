#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>
#include <scene/UIInputBoxActor.h>
#include <scene/ModelComponent.h>
#include <scene/BoneComponent.h>
#include <scene/TextComponent.h>
#include <rendering/Texture.h>
#include <scene/UIWindowActor.h>
#include <scene/SoundSourceComponent.h>
#include <animation/AnimationManagerActor.h>
#include <scene/GunActor.h>
#include <input/InputDevicesStateRetriever.h>

#include <tinyfiledialogs/tinyfiledialogs.h>
#include <scene/hierarchy/HierarchyTree.h>
#include <scene/LightComponent.h>


namespace GEE
{
	/*
UIListActor::UIListActor(GameScene& scene, const std::string& name, Vec3f elementOffset):
	UIActor(scene, name),
	ElementOffset(elementOffset)
{
}

UIListActor::UIListActor(UIListActor&& listActor):
	UIActor(std::move(listActor)),
	ElementOffset(listActor.ElementOffset)
{
}

Actor& UIListActor::AddChild(UniquePtr<Actor> actor)
{
	Actor& actorRef = Actor::AddChild(std::move(actor));
	return actorRef;
}
void UIListActor::Refresh()
{
	MoveElements(GetChildren(), 0);
}

Vec3f UIListActor::GetListOffset()
{
	Vec3f offset = ElementOffset * static_cast<float>(Children.size());
	for (auto& it : Children)
		if (UIListActor* cast = dynamic_cast<UIListActor*>(it.get()))
			offset += cast->GetListOffset() - ElementOffset;	//subtract ElementOffset to avoid offseting twice

	return Mat3f(GetTransform()->GetMatrix()) * offset;
}

Vec3f UIListActor::MoveElement(Actor& element, Vec3f nextElementBegin, float level)
{
	element.GetTransform()->SetPosition(nextElementBegin + ElementOffset / 2.0f);

	return nextElementBegin + ElementOffset;
}

Vec3f UIListActor::MoveElements(std::vector<Actor*> currentLevelElements, unsigned int level)
{
	if (currentLevelElements.empty())
		return Vec3f(0.0f);

	Vec3f nextElementBegin = currentLevelElements.front()->GetTransform()->PositionRef - ElementOffset / 2.0f;
	for (auto& element : currentLevelElements)
		nextElementBegin = MoveElement(*element, nextElementBegin, level);

	return nextElementBegin;
}

Vec3f UIMultipleListActor::MoveElements(std::vector<Actor*> currentLevelElements, unsigned int level)
{
	if (currentLevelElements.empty())
		return Vec3f(0.0f);

	Vec3f nextElementBegin = currentLevelElements.front()->GetTransform()->PositionRef - ElementOffset / 2.0f;
	for (auto& element : currentLevelElements)
	{
		if (element->GetName() == "OGAR")
			continue;
		nextElementBegin = MoveElement(*element, nextElementBegin, level);

		if (!NestedLists.empty())
		{
			auto found = std::find(NestedLists.begin(), NestedLists.end(), element);
			if (found != NestedLists.end())
				nextElementBegin += (*found)->GetListOffset() - ElementOffset;	//subtract ElementOffset to avoid offseting twice (we already applied it in MoveElement)
		}
	}

	for (auto& list : NestedLists)
	{
		auto children = list->GetChildren();
		if (!children.empty())
			nextElementBegin = list->MoveElements(children, level + 1);
	}

	return nextElementBegin;
}

void UIMultipleListActor::NestList(UIListActor& list)
{
	NestedLists.push_back(&list);
}*/


	UICanvasField::UICanvasField(GameScene& scene, Actor* parentActor, const std::string& name) :
		Actor(scene, parentActor, name),
		UIActor(parentActor)
	{
	}

	void UICanvasField::OnStart()
	{
		Actor::OnStart();

		CreateComponent<TextComponent>("ElementText", Transform(Vec2f(-2.0f, 0.0f), Vec2f(1.0f)), Name, "", std::pair<TextAlignment, TextAlignment>(TextAlignment::RIGHT, TextAlignment::CENTER));
	}

	UIElementTemplates UICanvasField::GetTemplates()
	{
		return UIElementTemplates(*this);
	}

	UICanvasFieldCategory::UICanvasFieldCategory(GameScene& scene, Actor* parentActor, const std::string& name) :
		UIAutomaticListActor(scene, parentActor, name),
		OnExpansionFunc(nullptr),
		ExpandButton(nullptr)
	{
		ExpandButton = &CreateChild<UIButtonActor>(name + "ExpandButton", [this]()
			{
				std::vector<Actor*> actors;
				std::vector<RenderableComponent*> renderables;
				for (auto& it : ListElements)
				{
					it.GetActorRef().GetRoot()->GetAllComponents<RenderableComponent>(&renderables);
					it.GetActorRef().GetAllActors(&actors);
				}
				for (auto& it : actors)
					it->GetRoot()->GetAllComponents<RenderableComponent>(&renderables);

				bExpanded = !bExpanded;
				for (auto& it : renderables)
					it->SetHide(!bExpanded);

				if (OnExpansionFunc)
					OnExpansionFunc();
			});
		ExpandButton->SetTransform(Transform(Vec2f(0.0f, 2.0f), Vec2f(1.0f)));

		//We do not want the list element to be hidden after retracting
		EraseListElement(*ExpandButton);
	}

	void UICanvasFieldCategory::OnStart()
	{
		UIListActor::OnStart();

		CreateComponent<TextComponent>("CategoryText", Transform(Vec2f(-2.0f, 2.0f), Vec2f(1.0f)), Name, "", std::pair<TextAlignment, TextAlignment>(TextAlignment::RIGHT, TextAlignment::CENTER));
	}

	UIButtonActor* UICanvasFieldCategory::GetExpandButton()
	{
		return ExpandButton;
	}

	UIElementTemplates UICanvasFieldCategory::GetTemplates()
	{
		return UIElementTemplates(*this);
	}

	UICanvasField& UICanvasFieldCategory::AddField(const std::string& name, std::function<Vec3f()> getElementOffset)
	{
		UICanvasField& field = CreateChild<UICanvasField>(name);
		if (getElementOffset)
			SetListElementOffset(GetListElementCount() - 1, getElementOffset);

		return field;
	}

	Vec3f UICanvasFieldCategory::GetListOffset()
	{
		return (bExpanded) ? (UIAutomaticListActor::GetListOffset() + Vec3f(0.0f, -1.0f, 0.0f)) : (Vec3f(0.0f, -1.0f, 0.0f));
	}

	void UICanvasFieldCategory::SetOnExpansionFunc(std::function<void()> onExpansionFunc)
	{
		OnExpansionFunc = [this, onExpansionFunc]() { if (onExpansionFunc) onExpansionFunc(); for (auto& it : Children) it->HandleEventAll(CursorMoveEvent(EventType::MouseMoved, GameHandle->GetInputRetriever().GetMousePositionNDC())); };
	}

	void UICanvasFieldCategory::HandleEventAll(const Event& ev)
	{
		HandleEvent(ev);
		ExpandButton->HandleEvent(ev);

		if (bExpanded)
			for (auto& it : Children)
				if (it.get() != ExpandButton)
					it->HandleEventAll(ev);
	}

	UIElementTemplates::UIElementTemplates(Actor& templateParent) :
		TemplateParent(templateParent),
		Scene(templateParent.GetScene()),
		GameHandle(*templateParent.GetScene().GetGameHandle())
	{
	}

	void UIElementTemplates::TickBox(bool& modifiedBool)
	{
		TickBox([&modifiedBool]() { modifiedBool = !modifiedBool; return modifiedBool; });
	}

	void UIElementTemplates::TickBox(std::function<bool()> setFunc)
	{
		AtlasMaterial* tickMaterial = new AtlasMaterial(Material("TickMaterial", 0.0f, GameHandle.GetRenderEngineHandle()->FindShader("Forward_NoLight")), glm::ivec2(3, 1));
		tickMaterial->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("EditorAssets/tick_icon.png", Texture::Format::RGBA(), true), "albedo1"));

		UIButtonActor& billboardTickBoxActor = TemplateParent.CreateChild<UIButtonActor>("BillboardTickBox");

		ModelComponent& billboardTickModel = billboardTickBoxActor.CreateComponent<ModelComponent>("TickModel");
		billboardTickModel.AddMeshInst(GameHandle.GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		billboardTickModel.SetHide(true);
		billboardTickModel.OverrideInstancesMaterialInstances(MakeShared<MaterialInstance>(MaterialInstance(*tickMaterial, tickMaterial->GetTextureIDInterpolatorTemplate(0.0f))));

		auto tickHideFunc = [&billboardTickModel, setFunc]() { billboardTickModel.SetHide(!setFunc()); };
		billboardTickBoxActor.SetOnClickFunc(tickHideFunc);

		tickHideFunc();	//little hack that allows us to set tick hide accordingly
		tickHideFunc();	//notice that we don't have any getFunc here, so the only way to show the tick correctly is to click this button twice here - it won't alter the boolean, but will repair our tick icon.																															
	}

	void UIElementTemplates::TickBox(std::function<void(bool)> setFunc, std::function<bool()> getFunc)
	{
		AtlasMaterial* tickMaterial = new AtlasMaterial(Material("TickMaterial", 0.0f, GameHandle.GetRenderEngineHandle()->FindShader("Forward_NoLight")), glm::ivec2(3, 1));
		tickMaterial->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("EditorAssets/tick_icon.png", Texture::Format::RGBA(), true), "albedo1"));

		UIButtonActor& billboardTickBoxActor = TemplateParent.CreateChild<UIButtonActor>("BillboardTickBox");

		ModelComponent& billboardTickModel = billboardTickBoxActor.CreateComponent<ModelComponent>("TickModel");
		billboardTickModel.AddMeshInst(GameHandle.GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		billboardTickModel.SetHide(!getFunc());
		billboardTickModel.OverrideInstancesMaterialInstances(MakeShared<MaterialInstance>(MaterialInstance(*tickMaterial, tickMaterial->GetTextureIDInterpolatorTemplate(0.0f))));

		auto clickFunc = [&billboardTickModel, setFunc, getFunc]() { bool updatedVal = !getFunc(); setFunc(updatedVal); billboardTickModel.SetHide(!updatedVal);  };
		billboardTickBoxActor.SetOnClickFunc(clickFunc);
	}

	void UIElementTemplates::HierarchyTreeInput(UIAutomaticListActor& list, GameScene& scene, std::function<void(HierarchyTemplate::HierarchyTreeT&)> setFunc)
	{
		for (int i = 0; i < scene.GetHierarchyTreeCount(); i++)
		{
			HierarchyTemplate::HierarchyTreeT* tree = scene.GetHierarchyTree(i);
			UIButtonActor& treeButton = list.CreateChild<UIButtonActor>("TreeButton" + std::to_string(i), tree->GetName(), [tree, setFunc]() { setFunc(*tree); });
		}

	}
	void UIElementTemplates::HierarchyTreeInput(GameScene& scene, std::function<void(HierarchyTemplate::HierarchyTreeT&)> setFunc)
	{
		UIAutomaticListActor& treesList = TemplateParent.CreateChild<UIAutomaticListActor>("HierarchyTreesList");
		HierarchyTreeInput(treesList, scene, setFunc);

		treesList.Refresh();
	}

	void UIElementTemplates::HierarchyTreeInput(GameManager& gameHandle, std::function<void(HierarchyTemplate::HierarchyTreeT&)> setFunc)
	{
		UIAutomaticListActor& treesList = TemplateParent.CreateChild<UIAutomaticListActor>("HierarchyTreesList");
		for (GameScene* scene : gameHandle.GetScenes())
			HierarchyTreeInput(treesList, *scene, setFunc);

		treesList.Refresh();
	}

	void UIElementTemplates::PathInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc, std::vector<const char*> extensions)
	{
		UIInputBoxActor& pathInputBox = TemplateParent.CreateChild<UIInputBoxActor>("PathInputBox");
		pathInputBox.SetTransform(Transform(Vec2f(2.0f, 0.0f), Vec2f(3.0f, 1.0f)));
		pathInputBox.SetOnInputFunc(setFunc, getFunc);
		UIButtonActor& selectFileActor = TemplateParent.CreateChild<UIButtonActor>("SelectFileButton");
		AtlasMaterial* moreMat = dynamic_cast<AtlasMaterial*>(GameHandle.GetRenderEngineHandle()->FindMaterial("GEE_E_More_Mat").get());

		selectFileActor.SetMatIdle(MaterialInstance(*moreMat, moreMat->GetTextureIDInterpolatorTemplate(0.0f)));
		selectFileActor.SetMatHover(MaterialInstance(*moreMat, moreMat->GetTextureIDInterpolatorTemplate(1.0f)));
		selectFileActor.SetMatClick(MaterialInstance(*moreMat, moreMat->GetTextureIDInterpolatorTemplate(2.0f)));

		selectFileActor.SetTransform(Transform(Vec2f(6.0f, 0.0f), Vec2f(1.0f, 1.0f)));

		//TextComponent& pathText = selectFileActor.NowyCreateComponent<TextComponent>("Text", Transform(), "", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
		selectFileActor.SetOnClickFunc([&pathInputBox, extensions]() { const char* path = tinyfd_openFileDialog("Select file", "C:\\", extensions.size(), (extensions.empty()) ? (nullptr) : (&extensions[0]), nullptr, 0); if (path) pathInputBox.PutString(path); });
	}

	void UIElementTemplates::FolderInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc)
	{
		UIInputBoxActor& pathInputBox = TemplateParent.CreateChild<UIInputBoxActor>("PathInputBox");
		pathInputBox.SetTransform(Transform(Vec2f(2.0f, 0.0f), Vec2f(3.0f, 1.0f)));
		pathInputBox.SetOnInputFunc(setFunc, getFunc);
		UIButtonActor& selectFileActor = TemplateParent.CreateChild<UIButtonActor>("SelectFolderButton");
		AtlasMaterial* moreMat = dynamic_cast<AtlasMaterial*>(GameHandle.GetRenderEngineHandle()->FindMaterial("GEE_E_More_Mat").get());

		selectFileActor.SetMatIdle(MaterialInstance(*moreMat, moreMat->GetTextureIDInterpolatorTemplate(0.0f)));
		selectFileActor.SetMatHover(MaterialInstance(*moreMat, moreMat->GetTextureIDInterpolatorTemplate(1.0f)));
		selectFileActor.SetMatClick(MaterialInstance(*moreMat, moreMat->GetTextureIDInterpolatorTemplate(2.0f)));

		selectFileActor.SetTransform(Transform(Vec2f(6.0f, 0.0f), Vec2f(1.0f, 1.0f)));
		selectFileActor.SetOnClickFunc([&pathInputBox]() { const char* path = tinyfd_selectFolderDialog("Select folder", "C:\\"); if (path) pathInputBox.PutString(path); });
	}
}