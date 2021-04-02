#include <UI/UICanvasField.h>
#include <UI/UIListActor.h>
#include <scene/UIInputBoxActor.h>
#include <scene/ModelComponent.h>
#include <scene/TextComponent.h>
#include <rendering/Texture.h>
#include <scene/UIWindowActor.h>
#include <scene/SoundSourceComponent.h>
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


UICanvasField::UICanvasField(GameScene& scene, const std::string& name) :
	UIActor(scene, name)
{
}

void UICanvasField::OnStart()
{
	UIActor::OnStart();

	CreateComponent(TextComponent(Scene, "ElementText", Transform(glm::vec2(-3.0f, 0.0f), glm::vec2(1.0f)), Name, "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::RIGHT, TextAlignment::CENTER)));
}

UIElementTemplates UICanvasField::GetTemplates()
{
	return UIElementTemplates(*this, CanvasPtr);
}

UIElementTemplates::UIElementTemplates(Actor& templateParent, UICanvas* canvas) :
	TemplateParent(templateParent),
	Scene(templateParent.GetScene()),
	GameHandle(*templateParent.GetScene().GetGameHandle()),
	Canvas(canvas)
{
}

void UIElementTemplates::TickBox(bool& modifiedBool)
{
	TickBox([&modifiedBool]() { modifiedBool = !modifiedBool; return modifiedBool; });
}

void UIElementTemplates::TickBox(std::function<bool()> setFunc)
{
	AtlasMaterial* tickMaterial = new AtlasMaterial("TickMaterial", glm::ivec2(3, 1), 0.0f, 0.0f, GameHandle.GetRenderEngineHandle()->FindShader("Forward_NoLight"));
	tickMaterial->AddTexture(new NamedTexture(textureFromFile("EditorAssets/tick_icon.png", GL_RGBA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true), "albedo1"));

	UIButtonActor& billboardTickBoxActor = TemplateParent.CreateChild(UIButtonActor(Scene, "BillboardTickBox"));

	ModelComponent& billboardTickModel = billboardTickBoxActor.CreateComponent(ModelComponent(Scene, "TickModel"));
	billboardTickModel.AddMeshInst(GameHandle.GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
	billboardTickModel.SetHide(true);
	billboardTickModel.OverrideInstancesMaterialInstances(std::make_shared<MaterialInstance>(MaterialInstance(*tickMaterial, tickMaterial->GetTextureIDInterpolatorTemplate(0.0f))));

	auto tickHideFunc = [&billboardTickModel, setFunc]() { billboardTickModel.SetHide(!setFunc()); };
	billboardTickBoxActor.SetOnClickFunc(tickHideFunc);

	tickHideFunc();	//little hack that allows us to set tick hide accordingly
	tickHideFunc();	//notice that we don't have any getFunc here, so the only way to show the tick correctly is to click this button twice here - it won't alter the boolean, but will repair our tick icon.																															
}
#include <tinyfiledialogs/tinyfiledialogs.h>
void UIElementTemplates::PathInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc)
{
	UIInputBoxActor& pathInputBox = TemplateParent.CreateChild(UIInputBoxActor(Scene, "PathInputBox"));
	pathInputBox.SetTransform(Transform(glm::vec2(0.0f), glm::vec2(3.0f, 1.0f)));
	pathInputBox.SetOnInputFunc(setFunc, getFunc);
	UIButtonActor& selectFileActor = TemplateParent.CreateChild(UIButtonActor(Scene, "SelectFileButton"));
	selectFileActor.SetTransform(Transform(glm::vec2(4.0f, 0.0f), glm::vec2(1.0f, 1.0f)));

	//TextComponent& pathText = selectFileActor.CreateComponent(TextComponent(Scene, "Text", Transform(), "", "fonts/expressway rg.ttf", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER)));

	selectFileActor.SetOnClickFunc([&pathInputBox]() { const char* patterns[] = { "*.wav" }; const char* path = tinyfd_openFileDialog("Select audio file", "C:\\", 1, patterns, nullptr, 0); if (path) pathInputBox.PutString(path); });
}

template <int vecSize> void UIElementTemplates::VecInput(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc)
{
	for (int x = 0; x < vecSize; x++)
	{
		UIInputBoxActor& inputBoxActor = TemplateParent.CreateChild(UIInputBoxActor(Scene, "Vec3Box"));
		inputBoxActor.SetTransform(Transform(glm::vec2(-0.75f + float(x) * 2.0f, 0.0f), glm::vec2(1.0f)));
		inputBoxActor.SetOnInputFunc([x, setFunc](float val) {setFunc(x, val); }, [x, getFunc]() { return getFunc(x); }, true);
	}
}

template <typename VecType> void UIElementTemplates::VecInput(VecType& modifiedVec)
{
	VecInput<VecType>([&modifiedVec](float x, float val) { modifiedVec[x] = val; }, [&modifiedVec](float x) { return modifiedVec[x]; });
}
#include <scene/LightComponent.h>

template<typename CompType>
void UIElementTemplates::ComponentInput(std::function<void(CompType*)> setFunc, Component& hierarchyRoot)
{
	GameScene* scenePtr = &Scene;
	UIButtonActor& compNameButton = TemplateParent.CreateChild(UIButtonActor(Scene, "CompNameBox", "", [scenePtr, &hierarchyRoot, setFunc]() {
		UIWindowActor& window = scenePtr->CreateActorAtRoot(UIWindowActor(*scenePtr, "CompInputWindow"));
		window.SetTransform(Transform(glm::vec2(0.0f, -0.7f), glm::vec2(0.25f)));

		UIAutomaticListActor& list = window.CreateChild(UIAutomaticListActor(*scenePtr, "Available comp list"));
		std::vector<CompType*> comps;
		if (auto cast = dynamic_cast<CompType*>(&hierarchyRoot))
			comps.push_back(cast);
		hierarchyRoot.GetAllComponents<CompType>(&comps);

		for (auto& it : comps)
			window.AddUIElement(list.CreateChild(UIButtonActor(*scenePtr, it->GetName() + ", Matching component button", it->GetName(), [&window, setFunc, it]() { setFunc(it); window.MarkAsKilled(); })));
		list.Refresh();

		std::cout << comps.size() << " available models.\n";
		}));
	compNameButton.SetTransform(Transform(glm::vec2(0.0f, 0.0f), glm::vec2(3.0f, 1.0f)));
}

template<typename CompType>
void UIElementTemplates::ComponentInput(CompType*& inputTo, Component& hierarchyRoot)
{
	ComponentInput<CompType>([&inputTo](CompType* comp) { inputTo = comp; }, hierarchyRoot);
}

template <> void UIElementTemplates::VecInput<glm::vec2>(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc)
{
	VecInput<2>(setFunc, getFunc);
}

template <> void UIElementTemplates::VecInput<glm::vec3>(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc)
{
	VecInput<3>(setFunc, getFunc);
}

template <> void UIElementTemplates::VecInput<glm::vec4>(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc) 
{
	VecInput<4>(setFunc, getFunc);
}

template <> void UIElementTemplates::VecInput<glm::quat>(std::function<void(float, float)> setFunc, std::function<float(float)> getFunc)
{
	VecInput<4>(setFunc, getFunc);
}


template void UIElementTemplates::VecInput<glm::vec2>(glm::vec2&);
template void UIElementTemplates::VecInput<glm::vec3>(glm::vec3&);
template void UIElementTemplates::VecInput<glm::vec4>(glm::vec4&);
template void UIElementTemplates::VecInput<glm::quat>(glm::quat&);

template void UIElementTemplates::ComponentInput<ModelComponent>(std::function<void(ModelComponent*)>, Component&);
template void UIElementTemplates::ComponentInput<LightComponent>(std::function<void(LightComponent*)>, Component&);
template void UIElementTemplates::ComponentInput<SoundSourceComponent>(std::function<void(SoundSourceComponent*)>, Component&);

template void UIElementTemplates::ComponentInput<ModelComponent>(ModelComponent*&, Component&);
template void UIElementTemplates::ComponentInput<LightComponent>(LightComponent*&, Component&);
template void UIElementTemplates::ComponentInput<SoundSourceComponent>(SoundSourceComponent*&, Component&);