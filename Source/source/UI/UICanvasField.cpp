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
#include <UI/UICanvas.h>

/*
UIListActor::UIListActor(GameScene& scene, const std::string& name, glm::vec3 elementOffset):
	UIActor(scene, name),
	ElementOffset(elementOffset)
{
}

UIListActor::UIListActor(UIListActor&& listActor):
	UIActor(std::move(listActor)),
	ElementOffset(listActor.ElementOffset)
{
}

Actor& UIListActor::AddChild(std::unique_ptr<Actor> actor)
{
	Actor& actorRef = Actor::AddChild(std::move(actor));
	return actorRef;
}
void UIListActor::Refresh()
{
	MoveElements(GetChildren(), 0);
}

glm::vec3 UIListActor::GetListOffset()
{
	glm::vec3 offset = ElementOffset * static_cast<float>(Children.size());
	for (auto& it : Children)
		if (UIListActor* cast = dynamic_cast<UIListActor*>(it.get()))
			offset += cast->GetListOffset() - ElementOffset;	//subtract ElementOffset to avoid offseting twice

	return glm::mat3(GetTransform()->GetMatrix()) * offset;
}

glm::vec3 UIListActor::MoveElement(Actor& element, glm::vec3 nextElementBegin, float level)
{
	element.GetTransform()->SetPosition(nextElementBegin + ElementOffset / 2.0f);

	return nextElementBegin + ElementOffset;
}

glm::vec3 UIListActor::MoveElements(std::vector<Actor*> currentLevelElements, unsigned int level)
{
	if (currentLevelElements.empty())
		return glm::vec3(0.0f);

	glm::vec3 nextElementBegin = currentLevelElements.front()->GetTransform()->PositionRef - ElementOffset / 2.0f;
	for (auto& element : currentLevelElements)
		nextElementBegin = MoveElement(*element, nextElementBegin, level);

	return nextElementBegin;
}

glm::vec3 UIMultipleListActor::MoveElements(std::vector<Actor*> currentLevelElements, unsigned int level)
{
	if (currentLevelElements.empty())
		return glm::vec3(0.0f);

	glm::vec3 nextElementBegin = currentLevelElements.front()->GetTransform()->PositionRef - ElementOffset / 2.0f;
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

	CreateComponent<TextComponent>("ElementText", Transform(glm::vec2(-2.0f, 0.0f), glm::vec2(1.0f)), Name, "", std::pair<TextAlignment, TextAlignment>(TextAlignment::RIGHT, TextAlignment::CENTER));
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

	CreateComponent<TextComponent>("CategoryText", Transform(glm::vec2(-2.0f, 2.0f), glm::vec2(1.0f)), Name, "", std::pair<TextAlignment, TextAlignment>(TextAlignment::RIGHT, TextAlignment::CENTER));
}

UIButtonActor* UICanvasFieldCategory::GetExpandButton()
{
	return ExpandButton;
}

UIElementTemplates UICanvasFieldCategory::GetTemplates()
{
	return UIElementTemplates(*this);
}

UICanvasField& UICanvasFieldCategory::AddField(const std::string& name, std::function<glm::vec3()> getElementOffset)
{
	UICanvasField& field = CreateChild<UICanvasField>(name);
	if (getElementOffset)
		SetListElementOffset(GetListElementCount() - 1, getElementOffset);

	return field;
}

glm::vec3 UICanvasFieldCategory::GetListOffset()
{
	return (bExpanded) ? (UIAutomaticListActor::GetListOffset() + Vec3f(0.0f, -1.0f, 0.0f)) : (Vec3f(0.0f, -1.0f, 0.0f));
}

#include <input/InputDevicesStateRetriever.h>
void UICanvasFieldCategory::SetOnExpansionFunc(std::function<void()> onExpansionFunc)
{
	OnExpansionFunc = [this, onExpansionFunc]() { if (onExpansionFunc) onExpansionFunc(); for (auto& it : Children) it->HandleEventAll(CursorMoveEvent(EventType::MOUSE_MOVED, GameHandle->GetInputRetriever().GetMousePositionNDC())); };
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
	tickMaterial->AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/tick_icon.png", GL_RGBA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true), "albedo1"));

	UIButtonActor& billboardTickBoxActor = TemplateParent.CreateChild<UIButtonActor>("BillboardTickBox");

	ModelComponent& billboardTickModel = billboardTickBoxActor.CreateComponent<ModelComponent>("TickModel");
	billboardTickModel.AddMeshInst(GameHandle.GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
	billboardTickModel.SetHide(true);
	billboardTickModel.OverrideInstancesMaterialInstances(std::make_shared<MaterialInstance>(MaterialInstance(*tickMaterial, tickMaterial->GetTextureIDInterpolatorTemplate(0.0f))));

	auto tickHideFunc = [&billboardTickModel, setFunc]() { billboardTickModel.SetHide(!setFunc()); };
	billboardTickBoxActor.SetOnClickFunc(tickHideFunc);

	tickHideFunc();	//little hack that allows us to set tick hide accordingly
	tickHideFunc();	//notice that we don't have any getFunc here, so the only way to show the tick correctly is to click this button twice here - it won't alter the boolean, but will repair our tick icon.																															
}

void UIElementTemplates::TickBox(std::function<void(bool)> setFunc, std::function<bool()> getFunc)
{
	AtlasMaterial* tickMaterial = new AtlasMaterial(Material("TickMaterial", 0.0f, GameHandle.GetRenderEngineHandle()->FindShader("Forward_NoLight")), glm::ivec2(3, 1));
	tickMaterial->AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/tick_icon.png", GL_RGBA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true), "albedo1"));

	UIButtonActor& billboardTickBoxActor = TemplateParent.CreateChild<UIButtonActor>("BillboardTickBox");
	
	ModelComponent& billboardTickModel = billboardTickBoxActor.CreateComponent<ModelComponent>("TickModel");
	billboardTickModel.AddMeshInst(GameHandle.GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
	billboardTickModel.SetHide(!getFunc());
	billboardTickModel.OverrideInstancesMaterialInstances(std::make_shared<MaterialInstance>(MaterialInstance(*tickMaterial, tickMaterial->GetTextureIDInterpolatorTemplate(0.0f))));

	auto clickFunc = [&billboardTickModel, setFunc, getFunc]() { bool updatedVal = !getFunc(); setFunc(updatedVal); billboardTickModel.SetHide(!updatedVal);  };
	billboardTickBoxActor.SetOnClickFunc(clickFunc);
}

#include <tinyfiledialogs/tinyfiledialogs.h>
#include <scene/HierarchyTemplate.h>
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
	pathInputBox.SetTransform(Transform(glm::vec2(2.0f, 0.0f), glm::vec2(3.0f, 1.0f)));
	pathInputBox.SetOnInputFunc(setFunc, getFunc);
	UIButtonActor& selectFileActor = TemplateParent.CreateChild<UIButtonActor>("SelectFileButton");

	selectFileActor.SetTransform(Transform(glm::vec2(6.0f, 0.0f), glm::vec2(1.0f, 1.0f)));

	//TextComponent& pathText = selectFileActor.NowyCreateComponent<TextComponent>("Text", Transform(), "", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
	selectFileActor.SetOnClickFunc([&pathInputBox, extensions]() { const char* path = tinyfd_openFileDialog("Select file", "C:\\", extensions.size(), (extensions.empty()) ? (nullptr) : (&extensions[0]), nullptr, 0); if (path) pathInputBox.PutString(path); });
}

void UIElementTemplates::FolderInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc)
{
	UIInputBoxActor& pathInputBox = TemplateParent.CreateChild<UIInputBoxActor>("PathInputBox");
	pathInputBox.SetTransform(Transform(glm::vec2(2.0f, 0.0f), glm::vec2(3.0f, 1.0f)));
	pathInputBox.SetOnInputFunc(setFunc, getFunc);
	UIButtonActor& selectFileActor = TemplateParent.CreateChild<UIButtonActor>("SelectFolderButton");

	selectFileActor.SetTransform(Transform(glm::vec2(6.0f, 0.0f), glm::vec2(1.0f, 1.0f)));
	selectFileActor.SetOnClickFunc([&pathInputBox]() { const char* path = tinyfd_selectFolderDialog("Select folder", "C:\\"); if (path) pathInputBox.PutString(path); });
}

template <int vecSize> void UIElementTemplates::VecInput(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc)
{
	const std::string materialNames[] = {
		"GEE_Default_X_Axis", "GEE_Default_Y_Axis", "GEE_Default_Z_Axis", "GEE_Default_W_Axis"
	};

	static_assert(vecSize <= 4);

	for (int axis = 0; axis < vecSize; axis++)
	{
		UIInputBoxActor& inputBoxActor = TemplateParent.CreateChild<UIInputBoxActor>("VecBox" + std::to_string(axis));
		inputBoxActor.SetTransform(Transform(glm::vec2(float(axis) * 2.0f, 0.0f), glm::vec2(1.0f)));
		inputBoxActor.SetOnInputFunc([axis, setFunc](float val) {setFunc(axis, val); }, [axis, getFunc]() { return getFunc(axis); }, true);

		if (Material* found = GameHandle.GetRenderEngineHandle()->FindMaterial(materialNames[axis]).get())
			inputBoxActor.SetMatIdle(*found);
	}
}

template <typename VecType> void UIElementTemplates::VecInput(VecType& modifiedVec)
{
	VecInput<VecType>([&modifiedVec](float x, float val) { modifiedVec[x] = val; }, [&modifiedVec](float x) { return modifiedVec[x]; });
}
#include <scene/LightComponent.h>

template<typename ObjectBase, typename ObjectType>
void UIElementTemplates::ObjectInput(std::function<std::vector<ObjectBase*>()> getObjectsFunc, std::function<void(ObjectType*)> setFunc)
{
	GameScene* scenePtr = &Scene;
	UIButtonActor& compNameButton = TemplateParent.CreateChild<UIButtonActor>("CompNameBox", "", [scenePtr, getObjectsFunc, setFunc]() {
		UIWindowActor& window = scenePtr->CreateActorAtRoot<UIWindowActor>("CompInputWindow");
		window.SetTransform(Transform(glm::vec2(0.0f, -0.7f), glm::vec2(0.25f)));

		UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("Available comp list");
		std::vector<ObjectType*> availableObjects = getObjectsFunc();

		list.CreateChild<UIButtonActor>("Nullptr object button", "Nullptr", [&window, setFunc]() { setFunc(nullptr); window.MarkAsKilled(); });
		for (auto& it : availableObjects)
			list.CreateChild<UIButtonActor>(it->GetName() + ", Matching object button", it->GetName(), [&window, setFunc, it]() { setFunc(it); window.MarkAsKilled(); });
		list.Refresh();

		std::cout << availableObjects.size() << " available objects.\n";
		});
	compNameButton.SetTransform(Transform(glm::vec2(2.0f, 0.0f), glm::vec2(3.0f, 1.0f)));
}

template<typename ObjectBase, typename ObjectType>
void UIElementTemplates::ObjectInput(ObjectBase& hierarchyRoot, std::function<void(ObjectType*)> setFunc)
{
	GameScene* scenePtr = &Scene;
	UIButtonActor& compNameButton = TemplateParent.CreateChild<UIButtonActor>("CompNameBox", "", [scenePtr, &hierarchyRoot, setFunc]() {
		UIWindowActor& window = scenePtr->CreateActorAtRoot<UIWindowActor>("CompInputWindow");
		window.SetTransform(Transform(glm::vec2(0.0f, -0.7f), glm::vec2(0.25f)));

		UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("Available comp list");
		std::vector<ObjectType*> availableObjects;
		if (auto cast = dynamic_cast<ObjectType*>(&hierarchyRoot))
			availableObjects.push_back(cast);

		GetAllObjects<ObjectType>(hierarchyRoot, availableObjects);

		list.CreateChild<UIButtonActor>("Nullptr object button", "Nullptr", [&window, setFunc]() { setFunc(nullptr); window.MarkAsKilled(); });
		for (auto& it : availableObjects)
			list.CreateChild<UIButtonActor>(it->GetName() + ", Matching object button", it->GetName(), [&window, setFunc, it]() { setFunc(it); window.MarkAsKilled(); });
		list.Refresh();

		std::cout << availableObjects.size() << " available objects.\n";
		});
	compNameButton.SetTransform(Transform(glm::vec2(2.0f, 0.0f), glm::vec2(3.0f, 1.0f)));
}

template<typename ObjectBase, typename ObjectType>
void UIElementTemplates::ObjectInput(ObjectBase& hierarchyRoot, ObjectType*& inputTo)
{
	ObjectInput<ObjectBase, ObjectType>(hierarchyRoot, [&inputTo](ObjectType* comp) { inputTo = comp; });
}

template<typename CompType>
void UIElementTemplates::ComponentInput(Component& hierarchyRoot, std::function<void(CompType*)> setFunc)
{
	ObjectInput<Component, CompType>(hierarchyRoot, setFunc);
}

template<typename CompType>
void UIElementTemplates::ComponentInput(GameScene& scene, std::function<void(CompType*)> setFunc)
{
	std::vector<Actor*> allActors;
	const_cast<Actor*>(scene.GetRootActor())->GetAllActors(&allActors);

	std::vector<CompType*> allComponents;
	allComponents.resize(allActors.size());

	for (auto& it : allActors)
		it->GetRoot()->GetAllComponents<CompType>(&allComponents);
}

template<typename CompType>
void UIElementTemplates::ComponentInput(Component& hierarchyRoot, CompType*& inputTo)
{
	ComponentInput<CompType>(hierarchyRoot, [&inputTo](CompType* comp) { inputTo = comp; });
}


template <> void UIElementTemplates::VecInput<glm::vec2>(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc)
{
	VecInput<2>(setFunc, getFunc);
}

template <> void UIElementTemplates::VecInput<glm::vec3>(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc)
{
	VecInput<3>(setFunc, getFunc);
}

template <> void UIElementTemplates::VecInput<Vec3f>(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc)
{
	VecInput<3>(setFunc, getFunc);
}

template <> void UIElementTemplates::VecInput<Vec4f>(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc)
{
	VecInput<4>(setFunc, getFunc);
}

template <> void UIElementTemplates::VecInput<glm::vec4>(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc) 
{
	VecInput<4>(setFunc, getFunc);
}

template <> void UIElementTemplates::VecInput<glm::quat>(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc)
{
	VecInput<4>(setFunc, getFunc);
}

template <typename ObjectType> void GetAllObjects<Component>(Component& hierarchyRoot, std::vector<ObjectType*>& comps)
{
	hierarchyRoot.GetAllComponents<ObjectType>(&comps);
}

template <typename ObjectType> void GetAllObjects<Actor>(Actor& hierarchyRoot, std::vector<ObjectType*>& actors)
{
	hierarchyRoot.GetAllActors<ObjectType>(&actors);
}

template void UIElementTemplates::VecInput<glm::vec2>(glm::vec2&);
template void UIElementTemplates::VecInput<glm::vec3>(glm::vec3&);
template void UIElementTemplates::VecInput<Vec3f>(Vec3f&);
template void UIElementTemplates::VecInput<Vec4f>(Vec4f&);
template void UIElementTemplates::VecInput<glm::vec4>(glm::vec4&);
template void UIElementTemplates::VecInput<glm::quat>(glm::quat&);

template void UIElementTemplates::ObjectInput<Component, Component>(Component&, std::function<void(Component*)>);
template void UIElementTemplates::ObjectInput<Component, ModelComponent>(Component&, std::function<void(ModelComponent*)>);
template void UIElementTemplates::ObjectInput<Component, BoneComponent>(Component&, std::function<void(BoneComponent*)>);
template void UIElementTemplates::ObjectInput<Component, LightComponent>(Component&, std::function<void(LightComponent*)>);
template void UIElementTemplates::ObjectInput<Component, SoundSourceComponent>(Component&, std::function<void(SoundSourceComponent*)>);
template void UIElementTemplates::ObjectInput<Component, AnimationManagerComponent>(Component&, std::function<void(AnimationManagerComponent*)>);

template void UIElementTemplates::ObjectInput<Actor, Actor>(Actor&, std::function<void(Actor*)>);
template void UIElementTemplates::ObjectInput<Actor, Actor>(Actor&, Actor*&);
template void UIElementTemplates::ObjectInput<Actor, GunActor>(Actor&, GunActor*&);
template void UIElementTemplates::ObjectInput<Actor, GunActor>(Actor&, GunActor*&);

template void UIElementTemplates::ObjectInput<Actor, Actor>(std::function<std::vector<Actor*>()>, std::function<void(Actor*)>);
template void UIElementTemplates::ObjectInput<Material, Material>(std::function<std::vector<Material*>()>, std::function<void(Material*)>);

template void UIElementTemplates::ComponentInput<Component>(Component&, std::function<void(Component*)>);
template void UIElementTemplates::ComponentInput<ModelComponent>(Component&, std::function<void(ModelComponent*)>);
template void UIElementTemplates::ComponentInput<BoneComponent>(Component&, std::function<void(BoneComponent*)>);
template void UIElementTemplates::ComponentInput<LightComponent>(Component&, std::function<void(LightComponent*)>);
template void UIElementTemplates::ComponentInput<SoundSourceComponent>(Component&, std::function<void(SoundSourceComponent*)>);
template void UIElementTemplates::ComponentInput<AnimationManagerComponent>(Component&, std::function<void(AnimationManagerComponent*)>);

template void UIElementTemplates::ComponentInput<Component>(Component&, Component*&);
template void UIElementTemplates::ComponentInput<ModelComponent>(Component&, ModelComponent*&);
template void UIElementTemplates::ComponentInput<BoneComponent>(Component&, BoneComponent*&);
template void UIElementTemplates::ComponentInput<LightComponent>(Component&, LightComponent*&);
template void UIElementTemplates::ComponentInput<SoundSourceComponent>(Component&, SoundSourceComponent*&);
template void UIElementTemplates::ComponentInput<AnimationManagerComponent>(Component&, AnimationManagerComponent*&);