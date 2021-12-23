#pragma once
#include <UI/UIActor.h>
#include <functional>
#include <scene/UIWindowActor.h>
#include <scene/UIInputBoxActor.h>
#include <scene/TextComponent.h>
#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	class UIElementTemplates;
	class UIAutomaticListActor;

	class UICanvasField : public Actor, public UIActor
	{
	public:
		UICanvasField(GameScene&, Actor* parentActor, const std::string& name);
		virtual void OnStart() override;
		UIElementTemplates GetTemplates();
		TextComponent* GetTitleComp();

	private:
		TextComponent* TitleComp;
	};

	namespace uiCanvasFieldUtil
	{
		void SetupComponents(Actor& actor, const Vec2f& pos = Vec2f(0.0f), const Vec4f& backgroundColor = Vec4f(hsvToRgb(Vec3f(218.0f, 0.521f, 0.188f)), 1.0f), const Vec4f& separatorColor = Vec4f(0.18f, 0.25f, 0.39f, 1.0f));
	}


	// Tied to an actor
	class UIElementTemplates
	{
	public:
		UIElementTemplates(Actor&);


		void TickBox(bool& modifiedBool);
		void TickBox(std::function<bool()> setFunc);
		void TickBox(std::function<void(bool)> setFunc, std::function<bool()> getFunc);
		
		struct VecInputDescription
		{
			std::vector<UIInputBoxActor*> VecInputBoxes;
		};
		template <unsigned int length, typename T = float> VecInputDescription VecInput(std::function<void(int, T)> setFunc, std::function<T(int)> getFunc);	//encapsulated
		template <unsigned int length, typename T> VecInputDescription VecInput(Vec<length, T>& modifiedVec);	//not encapsulated

		/**
		 * @brief Creates a slider which is a child of the actor tied to this UIElementTemplates object.
		 * @param processUnitValue: the function used to process the selected value which is from 0 to 1 (unit interval).
		*/
		void SliderUnitInterval(std::function<void(float)> processUnitValue, float defaultValue = 0.5f);
		void SliderRawInterval(float begin, float end, std::function<void(float)> processRawValue, float defaultValue = 0.5f);

		//
		template <typename ObjectType> void ObjectInput(std::function<std::vector<ObjectType*>()> getObjectsFunc, std::function<void(ObjectType*)> setFunc, const std::string& currentObjName = std::string());	//encapsulated
		template <typename ObjectBase, typename ObjectType> void ObjectInput(ObjectBase& hierarchyRoot, std::function<void(ObjectType*)> setFunc, const std::string& currentObjName = std::string());	//encapsulated
		template <typename ObjectBase, typename ObjectType> void ObjectInput(ObjectBase& hierarchyRoot, ObjectType*& inputTo);	//not encapsulated

		template <typename T, typename InputIt> void ListSelection(InputIt first, InputIt last, std::function<void(UIButtonActor&, T&)> buttonFunc);
		template <typename T, typename InputIt> void ListSelection(InputIt first, InputIt last, std::function<void(UIAutomaticListActor&, T&)> buttonFunc);


	private:
		void HierarchyTreeInput(UIAutomaticListActor& list, GameScene&, std::function<void(HierarchyTemplate::HierarchyTreeT&)> setFunc);
	public:
		void HierarchyTreeInput(GameScene&, std::function<void(HierarchyTemplate::HierarchyTreeT&)> setFunc);
		void HierarchyTreeInput(GameManager&, std::function<void(HierarchyTemplate::HierarchyTreeT&)> setFunc);

		void PathInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc, std::vector<const char*> extensions);	//encapsulated
		void FolderInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc);	//encapsulated

	private:
		Actor& TemplateParent;
		GameScene& Scene;
		GameManager& GameHandle;
	};

	//s Tied to a scene
	class GenericUITemplates
	{
	public:
		GenericUITemplates(GameScene& scene) : Scene(scene) {}
		template <typename FunctorConfirm, typename FunctorDeny>
		void ConfirmationBox(FunctorConfirm&& onConfirmation, FunctorDeny&& onDenial, const std::string& promptText);
	private:
		GameScene& Scene;
	};

	template <typename ObjectType> void GetAllObjects(Component& hierarchyRoot, std::vector<ObjectType*>& comps)
	{
		hierarchyRoot.GetAllComponents<ObjectType>(&comps);
	}
	template <typename ObjectType> void GetAllObjects(Actor& hierarchyRoot, std::vector<ObjectType*>& actors)
	{
		hierarchyRoot.GetAllActors<ObjectType>(&actors);
	}

	// UIElementTemplates template methods definitions
	template <unsigned int length, typename T>
	inline UIElementTemplates::VecInputDescription UIElementTemplates::VecInput(std::function<void(int, T)> setFunc, std::function<T(int)> getFunc)
	{
		const std::string materialNames[] = {
			"GEE_Default_X_Axis", "GEE_Default_Y_Axis", "GEE_Default_Z_Axis", "GEE_Default_W_Axis"
		};

		GEE_CORE_ASSERT(length <= 4);

		VecInputDescription desc;
		char axisChar[] = {'x', 'y', 'z', 'w'};
		for (int axis = 0; axis < length; axis++)
		{
			UIInputBoxActor& inputBoxActor = TemplateParent.CreateChild<UIInputBoxActor>("VecBox" + std::to_string(axis));
			desc.VecInputBoxes.push_back(&inputBoxActor);
			inputBoxActor.SetTransform(Transform(Vec2f(float(axis) * 2.0f, 0.0f), Vec2f(1.0f, 0.6f)));

			std::function<void(float)> input = nullptr;
			std::function<float()> getter = nullptr;
			if (setFunc) input = [axis, setFunc](float val) {setFunc(axis, val); };
			if (getFunc) getter = [axis, getFunc]() -> float { return getFunc(axis); };

			inputBoxActor.SetOnInputFunc(input, getter, true);

			auto& axisText = inputBoxActor.GetRoot()->CreateComponent<TextComponent>("AxisText", Transform(Vec2f(0.0f, -1.0f), Vec2f(0.35f)), std::string(1, axisChar[axis]), "", Alignment2D::Top());
			axisText.Unstretch();

			if (auto found = GameHandle.GetRenderEngineHandle()->FindMaterial(materialNames[axis]))
			{
				inputBoxActor.SetMatIdle(found);
				axisText.SetMaterialInst(found);
			}
		}
		for (int axis = 0; axis < length; axis++)
			desc.VecInputBoxes[axis]->SetOnClickFunc([*this, allBoxes = desc.VecInputBoxes]()
			{
			std::cout << "all boxes size: " << allBoxes.size() << '\n';
			if (GameHandle.GetInputRetriever().IsKeyPressed(Key::LeftAlt))
				for (auto box : allBoxes)
					box->SetActive(true);
			});

		return desc;
	}

	template<unsigned int length, typename T>
	inline UIElementTemplates::VecInputDescription UIElementTemplates::VecInput(Vec<length, T>& modifiedVec)
	{
		return VecInput<length, T>([&](float x, T val) { modifiedVec[x] = val; }, [&](float x) -> T { return modifiedVec[x]; });
	}


	template<typename ObjectType>
	inline void UIElementTemplates::ObjectInput(std::function<std::vector<ObjectType*>()> getObjectsFunc, std::function<void(ObjectType*)> setFunc, const std::string& currentObjName)
	{
		GameScene* scenePtr = &Scene;
		auto& compNameButton = TemplateParent.CreateChild<UIButtonActor>("CompNameBox", currentObjName, nullptr, Transform(Vec2f(2.0f, 0.0f), Vec2f(3.0f, 1.0f)));
		compNameButton.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText")->Unstretch();

		compNameButton.SetOnClickFunc([scenePtr, getObjectsFunc, setFunc, &compNameButton]() {
			UIWindowActor& window = scenePtr->CreateActorAtRoot<UIWindowActor>("CompInputWindow");
			auto lol = compNameButton.GetCanvasPtr();
			Transform windowT = *compNameButton.GetTransform();
			windowT.Move(Vec2f(0.0f, -3.0f));
			windowT.ApplyScale(Vec2f(1.0f, 1.0f));
			windowT = compNameButton.GetTransform()->GetParentTransform()->GetWorldTransform() * windowT;
			windowT.SetPosition(Vec3f(dynamic_cast<UICanvasActor*>(lol)->GetTransform()->GetWorldTransformMatrix() *
				Vec4f(Vec3f(lol->GetProjection() * lol->GetViewMatrix() * Vec4f(windowT.GetPos(), 1.0f)), 1.0f)));
			windowT.SetScale(Vec2f(dynamic_cast<UICanvasActor*>(lol)->GetTransform()->GetWorldTransformMatrix() *
				Vec4f(Vec3f(lol->GetProjection() * lol->GetViewMatrix() * Vec4f(windowT.GetScale(), 0.0f)), 0.0f)));
			
			window.SetTransform(windowT);
			window.SetCloseIfClickedOutside(true);
			window.KillScrollBars();
			window.KillResizeBars();
			window.KillDragButton();
			window.KillCloseButton();

			UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("Available comp list");
			list.GetTransform()->SetScale(Vec2f(0.5f));

			std::vector<ObjectType*> availableObjects = getObjectsFunc();

			auto& deleteMe123 = list.CreateChild<UIButtonActor>("Nullptr object button", "nullptr", [&window, setFunc, &compNameButton]() { setFunc(nullptr); window.MarkAsKilled(); if (auto buttonText = compNameButton.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText")) buttonText->SetContent("nullptr"); });
			//deleteMe123.CreateComponent<ScrollingTextComponent>("ButtonText", Transform(Vec2f(0.0f), Vec2f(0.25f, 1.0f)), "Nullptr", "", Alignment2D::LeftCenter());
			for (auto& it : availableObjects)
			{
				auto& a = list.CreateChild<UIButtonActor>(it->GetName() + ", Matching object button", it->GetName(), [&window, setFunc, it, &compNameButton]() { setFunc(it); window.MarkAsKilled(); if (auto buttonText = compNameButton.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText")) { buttonText->SetContent(it->GetName()); buttonText->Unstretch(); } });
				a.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText")->Unstretch();
				//a.CreateComponent<TextConstantSizeComponent>("ButtonText", Transform(Vec2f(0.0f), Vec2f(0.25f, 1.0f)), it->GetName(), "", Alignment2D::Center()).SetMaxSize(Vec2f(0.8f));
			}
			list.Refresh();

			std::cout << availableObjects.size() << " available objects.\n";

			window.ClampViewToElements();
			});
	}

	template<typename ObjectBase, typename ObjectType>
	inline void UIElementTemplates::ObjectInput(ObjectBase& hierarchyRoot, std::function<void(ObjectType*)> setFunc, const std::string& currentObjName)
	{
		ObjectInput<ObjectType>([&]() -> std::vector<ObjectType*> {
			std::vector<ObjectType*> availableObjects;
			if (auto cast = dynamic_cast<ObjectType*>(&hierarchyRoot))
				availableObjects.push_back(cast);

			GetAllObjects<ObjectType>(hierarchyRoot, availableObjects);

			return availableObjects;
			}, setFunc, currentObjName);
	}

	template<typename ObjectBase, typename ObjectType>
	inline void UIElementTemplates::ObjectInput(ObjectBase& hierarchyRoot, ObjectType*& inputTo)
	{
		ObjectInput<ObjectBase, ObjectType>(hierarchyRoot, [&inputTo](ObjectType* comp) { inputTo = comp; }, (inputTo) ? (inputTo->GetName()) : (std::string()));
	}

	template<typename T, typename InputIt>
	inline void UIElementTemplates::ListSelection(InputIt first, InputIt last, std::function<void(UIButtonActor&, T&)> buttonFunc)
	{
		ListSelection<T, InputIt>(first, last, [](UIAutomaticListActor& listActor, T& object) { buttonFunc(listActor.CreateChild<UIButtonActor>("ListElementActor"), object); });
	}
	template<typename T, typename InputIt>
	inline void UIElementTemplates::ListSelection(InputIt first, InputIt last, std::function<void(UIAutomaticListActor&, T&)> buttonFunc)
	{
		UIAutomaticListActor& listActor = TemplateParent.CreateChild<UIAutomaticListActor>("ListSelectionActor");
		std::for_each(first, last, [&listActor, buttonFunc](T& object) { buttonFunc(listActor, object); std::cout << " TWORZE PRZYCISKA"; });
		listActor.Refresh();
	}

	// Generic UI templates
	template<typename FunctorConfirm, typename FunctorDeny>
	inline void GenericUITemplates::ConfirmationBox(FunctorConfirm&& onConfirmation, FunctorDeny&& onDenial, const std::string& promptText)
	{
		auto& confirmationWindow = Scene.CreateActorAtRoot<UIWindowActor>("Confirmation window");
		confirmationWindow.KillResizeBars();

		auto& textActor = confirmationWindow.CreateChild<UIActorDefault>("TextActor", Transform(Vec2f(0.0f, 1.0f), Vec2f(0.9f)));
		textActor.CreateComponent<TextConstantSizeComponent>("TextComponent", Transform(), promptText, "", Alignment2D::Top());

		auto& declineButton = confirmationWindow.CreateChild<UIButtonActor>("DeclineButton", "No", [=, &confirmationWindow]() { onDenial(); confirmationWindow.MarkAsKilled(); }, Transform(Vec2f(-0.5f, -0.8f), Vec2f(0.2f)));
		auto& confirmButton = confirmationWindow.CreateChild<UIButtonActor>("ConfirmButton", "Yes", [=, &confirmationWindow]() {  onConfirmation(); confirmationWindow.MarkAsKilled(); }, Transform(Vec2f(0.5f, -0.8f), Vec2f(0.2f)));
	}
}