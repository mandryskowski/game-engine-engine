#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>
#include <scene/UIInputBoxActor.h>
#include <scene/ModelComponent.h>
#include <scene/BoneComponent.h>
#include <scene/TextComponent.h>
#include <rendering/Texture.h>
#include <scene/UIWindowActor.h>
#include <scene/SoundSourceComponent.h>
#include <animation/AnimationManagerComponent.h>
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
		UIActor(parentActor),
		TitleComp(nullptr)
	{
	}

	void UICanvasField::OnStart()
	{
		Actor::OnStart();

		uiCanvasFieldUtil::SetupComponents(*this);
		TitleComp = GetRoot()->GetComponent<TextComponent>("ElementText");
	}

	UIElementTemplates UICanvasField::GetTemplates()
	{
		return UIElementTemplates(*this);
	}

	TextComponent* UICanvasField::GetTitleComp()
	{
		return TitleComp;
	}

	UICanvasFieldCategory::UICanvasFieldCategory(GameScene& scene, Actor* parentActor, const std::string& name) :
		UIAutomaticListActor(scene, parentActor, name),
		OnExpansionFunc(nullptr),
		ExpandButton(nullptr),
		CategoryBackgroundQuad(nullptr)
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

				if (CategoryBackgroundQuad)
					CategoryBackgroundQuad->SetHide(!bExpanded);

				if (OnExpansionFunc)
					OnExpansionFunc();
			});
		ExpandButton->SetTransform(Transform(Vec2f(0.0f, 2.0f), Vec2f(4.0f, 1.0f)));

		//We do not want the list element to be hidden after retracting
		EraseListElement(*ExpandButton);
	}

	void UICanvasFieldCategory::OnStart()
	{
		UIListActor::OnStart();

		Vec3f titleBackgroundColor(0.045f, 0.06f, 0.09f);

		auto titleBackgroundMaterial = MakeShared<Material>("TitleBackgroundMaterial");
		titleBackgroundMaterial->SetColor(titleBackgroundColor);

		auto& titleBackgroundQuad = CreateComponent<ModelComponent>("BackgroundQuad", Transform(Vec2f(0.0f, 2.0f), Vec2f(30.0f, 1.0f)));
		titleBackgroundQuad.AddMeshInst(GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));
		titleBackgroundQuad.OverrideInstancesMaterial(titleBackgroundMaterial);

		CreateComponent<TextComponent>("ElementText", Transform(Vec2f(0.0f, 2.0f), Vec2f(0.75f)), GetName(), "Assets/Editor/Fonts/Atkinson-Hyperlegible-Bold-102.otf", Alignment2D::Center()).Unstretch();

		CategoryBackgroundQuad = &CreateComponent<ModelComponent>("BackgroundQuad", Transform(Vec2f(0.0f, 0.0f), Vec2f(30.0f, 0.0f)));
		CategoryBackgroundQuad->AddMeshInst(GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));
		CategoryBackgroundQuad->OverrideInstancesMaterial(MakeShared<Material>("CategoryBackgroundMaterial", hsvToRgb(Vec3f(218.0f, 0.521f, 0.188f))));

		//uiCanvasFieldUtil::SetupComponents(*this, Vec2f(0.0f, 2.0f), Vec4f(0.045f, 0.06f, 0.09f, 1.0f));
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

	UICanvasFieldCategory& UICanvasFieldCategory::AddCategory(const std::string& name)
	{
		UICanvasFieldCategory& category = CreateChild<UICanvasFieldCategory>(name);
		category.SetTransform(Transform(Vec2f(0.0f, 0.0f), Vec2f(1.0f)));	//position doesn't matter if this is not the first field
		AddElement(UIListElement(UIListElement::ReferenceToUIActor(&category), [&category]() {/* category.Refresh(); */ return category.GetListOffset(); }, [&category]() { return Vec3f(0.0f, -2.0f, 0.0f); }));
		category.SetOnExpansionFunc([this]() { dynamic_cast<UICanvasActor*>(GetCanvasPtr())->RefreshFieldsList(); });

		return category;
	}

	Vec3f UICanvasFieldCategory::GetListOffset()
	{
		return (bExpanded) ? (UIAutomaticListActor::GetListOffset() + Vec3f(0.0f, -1.0f, 0.0f)) : (Vec3f(0.0f, -1.0f, 0.0f));
	}

	void UICanvasFieldCategory::SetOnExpansionFunc(std::function<void()> onExpansionFunc)
	{
		OnExpansionFunc = [this, onExpansionFunc]() { if (onExpansionFunc) onExpansionFunc(); for (auto& it : Children) it->HandleEventAll(CursorMoveEvent(EventType::MouseMoved, Scene.GetUIData()->GetWindowData().GetMousePositionNDC())); };
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

	void UICanvasFieldCategory::Refresh()
	{
		if (CategoryBackgroundQuad)
		{
			CategoryBackgroundQuad->GetTransform().SetPosition(Vec2f(0.0f, static_cast<float>(1.0f + UIAutomaticListActor::GetListOffset().y)));
			CategoryBackgroundQuad->GetTransform().SetVecAxis<TVec::Scale, VecAxis::Y>(static_cast<float>(-UIAutomaticListActor::GetListOffset().y));
		}

		UIAutomaticListActor::Refresh();
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
		SharedPtr<AtlasMaterial> tickMaterial = MakeShared<AtlasMaterial>("TickMaterial", glm::ivec2(3, 1));
		tickMaterial->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/Editor/tick_icon.png", Texture::Format::RGBA(), true), "albedo1"));

		UIButtonActor& billboardTickBoxActor = TemplateParent.CreateChild<UIButtonActor>("BillboardTickBox");

		ModelComponent& billboardTickModel = billboardTickBoxActor.CreateComponent<ModelComponent>("TickModel");
		billboardTickModel.AddMeshInst(GameHandle.GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));
		billboardTickModel.SetHide(true);
		billboardTickModel.OverrideInstancesMaterialInstances(MakeShared<MaterialInstance>(tickMaterial, tickMaterial->GetTextureIDInterpolatorTemplate(0.0f)));

		auto tickHideFunc = [&billboardTickModel, setFunc]() { billboardTickModel.SetHide(!setFunc()); };
		billboardTickBoxActor.SetOnClickFunc(tickHideFunc);

		tickHideFunc();	//little hack that allows us to set tick hide accordingly
		tickHideFunc();	//notice that we don't have any getFunc here, so the only way to show the tick correctly is to click this button twice here - it won't alter the boolean, but will repair our tick icon.																															
	}

	void UIElementTemplates::TickBox(std::function<void(bool)> setFunc, std::function<bool()> getFunc)
	{
		auto tickMaterial = MakeShared<AtlasMaterial>("TickMaterial", glm::ivec2(3, 1));
		tickMaterial->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/Editor/tick_icon.png", Texture::Format::RGBA(), true), "albedo1"));

		UIButtonActor& billboardTickBoxActor = TemplateParent.CreateChild<UIButtonActor>("BillboardTickBox");

		ModelComponent& billboardTickModel = billboardTickBoxActor.CreateComponent<ModelComponent>("TickModel");
		billboardTickModel.AddMeshInst(GameHandle.GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));
		billboardTickModel.SetHide(!getFunc());
		billboardTickModel.OverrideInstancesMaterialInstances(MakeShared<MaterialInstance>(tickMaterial, tickMaterial->GetTextureIDInterpolatorTemplate(0.0f)));

		auto clickFunc = [&billboardTickModel, setFunc, getFunc]() { bool updatedVal = !getFunc(); setFunc(updatedVal); billboardTickModel.SetHide(!getFunc());  };
		billboardTickBoxActor.SetOnClickFunc(clickFunc);
	}

	void UIElementTemplates::SliderUnitInterval(std::function<void(float)> processUnitValue, float defaultValue)
	{
		SliderRawInterval(0.0f, 1.0f, processUnitValue, defaultValue);
	}

	void UIElementTemplates::SliderRawInterval(float begin, float end, std::function<void(float)> processRawValueFunc, float defaultValue)
	{
		/*auto& sliderBackground = TemplateParent.CreateChild<UIButtonActor>("SliderBackground");
		Material* sliderBackgroundMaterial;
		if (sliderBackgroundMaterial = GameHandle.GetRenderEngineHandle()->FindMaterial("GEE_E_Slider_Background_Material").get(); !sliderBackgroundMaterial)
		{
			sliderBackgroundMaterial = GameHandle.GetRenderEngineHandle()->AddMaterial(MakeShared<Material>("GEE_E_Slider_Background_Material", 0.0f, GameHandle.GetRenderEngineHandle()->FindShader("Forward_NoLight")));
			sliderBackgroundMaterial->SetColor(Vec4f(0.37f, 0.7f, 0.45f, 1.0f));
		}
		sliderBackground.SetMatIdle(*sliderBackgroundMaterial);*/


		auto& valueText = TemplateParent.CreateChild<UIActorDefault>("SliderValueTextActor").CreateComponent<TextConstantSizeComponent>("SliderValueText", Transform(), ToStringPrecision(defaultValue, 3), "", Alignment2D::Center());
		auto& slider = TemplateParent.CreateChild<UIScrollBarActor>("Slider");
		slider.GetTransform()->SetScale(Vec2f(0.1f, 1.0f));
		float scaleX = slider.GetTransform()->GetScale().x;
		slider.GetTransform()->SetVecAxis<TVec::Position, VecAxis::X>(glm::clamp(scaleX / 2.0f + ((defaultValue - begin) / (end - begin)) * (1.0f - scaleX), 0.0f, 1.0f) * 2.0f - 1.0f);

		auto sliderProcessClick = [*this, &slider, &valueText, processRawValueFunc, begin, end]()
		{
			float posScreenSpace = static_cast<float>(Scene.GetUIData()->GetWindowData().GetMousePositionNDC().x);
			Mat4f sliderMat = TemplateParent.GetTransform()->GetWorldTransformMatrix();
			float posLocalSpace = ((slider.GetCanvasPtr()) ? (uiCanvasUtil::ScreenToLocalInCanvas(*slider.GetCanvasPtr(), sliderMat, Vec4f(posScreenSpace, 0.0f, 0.0f, 1.0f))) : (Vec4f(sliderMat * Vec4f(posScreenSpace, 0.0f, 0.0f, 1.0f)))).x;
			float sliderSize = slider.GetTransform()->GetScale().x;

			slider.GetTransform()->SetVecAxis<TVec::Position, VecAxis::X>(glm::clamp(posLocalSpace, -1.0f + sliderSize, 1.0f - sliderSize));

			slider.SetClickPosNDC(Scene.GetUIData()->GetWindowData().GetMousePositionNDC());

			float unitValue = ((slider.GetTransform()->GetPos().x - sliderSize) * 0.5f + 0.5f) / (1.0f - sliderSize);
			if (processRawValueFunc)
				processRawValueFunc(unitValue * (end - begin) + begin);

			valueText.SetContent(ToStringPrecision(unitValue * (end - begin) + begin, 3));
		};


		slider.SetWhileBeingClickedFunc(sliderProcessClick);
		//sliderBackground.SetOnClickFunc(sliderProcessClick);
	}

	void UIElementTemplates::HierarchyTreeInput(UIAutomaticListActor& list, GameScene& scene, std::function<void(Hierarchy::Tree&)> setFunc)
	{
		for (int i = 0; i < scene.GetHierarchyTreeCount(); i++)
		{
			Hierarchy::Tree* tree = scene.GetHierarchyTree(i);
			UIButtonActor& treeButton = list.CreateChild<UIButtonActor>("TreeButton" + std::to_string(i), tree->GetName().GetPath(), [tree, setFunc]() { setFunc(*tree); });
		}

	}
	void UIElementTemplates::HierarchyTreeInput(GameScene& scene, std::function<void(Hierarchy::Tree&)> setFunc)
	{
		UIAutomaticListActor& treesList = TemplateParent.CreateChild<UIAutomaticListActor>("HierarchyTreesList");
		HierarchyTreeInput(treesList, scene, setFunc);

		treesList.Refresh();
	}

	void UIElementTemplates::HierarchyTreeInput(GameManager& gameHandle, std::function<void(Hierarchy::Tree&)> setFunc)
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
		SharedPtr<AtlasMaterial> moreMat = GameHandle.GetRenderEngineHandle()->FindMaterial<AtlasMaterial>("GEE_E_More_Mat");

		selectFileActor.SetMatIdle(MaterialInstance(moreMat, moreMat->GetTextureIDInterpolatorTemplate(0.0f)));
		selectFileActor.SetMatHover(MaterialInstance(moreMat, moreMat->GetTextureIDInterpolatorTemplate(1.0f)));
		selectFileActor.SetMatClick(MaterialInstance(moreMat, moreMat->GetTextureIDInterpolatorTemplate(2.0f)));

		selectFileActor.SetTransform(Transform(Vec2f(6.0f, 0.0f), Vec2f(1.0f, 1.0f)));

		//TextComponent& pathText = selectFileActor.NowyCreateComponent<TextComponent>("Text", Transform(), "", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
		selectFileActor.SetOnClickFunc([&pathInputBox, extensions]() { const char* path = tinyfd_openFileDialog("Select file", "C:\\", extensions.size(), (extensions.empty()) ? (nullptr) : (&extensions[0]), nullptr, 0); if (path) pathInputBox.PutString(path); });
	}

	void UIElementTemplates::FolderInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc, const std::string& defaultPath)
	{
		UIInputBoxActor& pathInputBox = TemplateParent.CreateChild<UIInputBoxActor>("PathInputBox");
		pathInputBox.SetTransform(Transform(Vec2f(2.0f, 0.0f), Vec2f(3.0f, 1.0f)));
		pathInputBox.SetOnInputFunc(setFunc, getFunc);
		UIButtonActor& selectFileActor = TemplateParent.CreateChild<UIButtonActor>("SelectFolderButton");
		SharedPtr<AtlasMaterial> moreMat = GameHandle.GetRenderEngineHandle()->FindMaterial<AtlasMaterial>("GEE_E_More_Mat");

		selectFileActor.SetMatIdle(MaterialInstance(moreMat, moreMat->GetTextureIDInterpolatorTemplate(0.0f)));
		selectFileActor.SetMatHover(MaterialInstance(moreMat, moreMat->GetTextureIDInterpolatorTemplate(1.0f)));
		selectFileActor.SetMatClick(MaterialInstance(moreMat, moreMat->GetTextureIDInterpolatorTemplate(2.0f)));

		selectFileActor.SetTransform(Transform(Vec2f(6.0f, 0.0f), Vec2f(1.0f, 1.0f)));
		selectFileActor.SetOnClickFunc([&pathInputBox, defaultPath]() { const char* path = tinyfd_selectFolderDialog("Select folder", defaultPath.c_str()); if (path) pathInputBox.PutString(path); });
	}

	void uiCanvasFieldUtil::SetupComponents(Actor& actor, const Vec2f& pos, const Vec4f& backgroundColor, const Vec4f& separatorColor)
	{
		auto& backgroundQuad = actor.CreateComponent<ModelComponent>("BackgroundQuad", Transform(Vec2f(0.0f, pos.y), Vec2f(30.0f, 1.0f)));
		backgroundQuad.AddMeshInst(actor.GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));

		auto backgroundMaterial = MakeShared<Material>("BackgroundMaterial");
		backgroundMaterial->SetColor(backgroundColor);
		backgroundQuad.OverrideInstancesMaterial(backgroundMaterial);

		actor.CreateComponent<TextComponent>("ElementText", Transform(Vec2f(-2.0f, pos.y), Vec2f(0.75f)), actor.GetName(), "", Alignment2D::RightCenter());
		
		auto& separatorLine = actor.CreateComponent<ModelComponent>("SeparatorLine", Transform(Vec2f(-1.5f, pos.y), Vec2f(0.08f, 0.8f)));
		separatorLine.AddMeshInst(actor.GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));

		auto separatorMaterial = MakeShared<Material>("SeparatorColor");
		separatorMaterial->SetColor(separatorColor);
		separatorLine.OverrideInstancesMaterial(separatorMaterial);
	}

}