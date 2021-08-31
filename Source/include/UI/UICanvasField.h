#pragma once
#include <UI/UIActor.h>
#include <functional>
#include <scene/UIWindowActor.h>
#include <scene/UIInputBoxActor.h>
#include <scene/TextComponent.h>

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

		template <unsigned int length, typename T = float> void VecInput(std::function<void(int, T)> setFunc, std::function<T(int)> getFunc);	//encapsulated
		template <unsigned int length, typename T> void VecInput(Vec<length, T>& modifiedVec);	//not encapsulated

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

	// Tied to a scene
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
	inline void UIElementTemplates::VecInput(std::function<void(int, T)> setFunc, std::function<T(int)> getFunc)
	{
		const std::string materialNames[] = {
			"GEE_Default_X_Axis", "GEE_Default_Y_Axis", "GEE_Default_Z_Axis", "GEE_Default_W_Axis"
		};

		GEE_CORE_ASSERT(length <= 4);

		for (int axis = 0; axis < length; axis++)
		{
			UIInputBoxActor& inputBoxActor = TemplateParent.CreateChild<UIInputBoxActor>("VecBox" + std::to_string(axis));
			inputBoxActor.SetTransform(Transform(Vec2f(float(axis) * 2.0f, 0.0f), Vec2f(1.0f)));
			inputBoxActor.SetOnInputFunc([axis, setFunc](float val) {setFunc(axis, val); }, [axis, getFunc]() -> float { return getFunc(axis); }, true);

			if (Material* found = GameHandle.GetRenderEngineHandle()->FindMaterial(materialNames[axis]).get())
				inputBoxActor.SetMatIdle(*found);
		}
	}

	template<unsigned int length, typename T>
	inline void UIElementTemplates::VecInput(Vec<length, T>& modifiedVec)
	{
		VecInput<length, T>([&](float x, T val) { modifiedVec[x] = val; }, [&](float x) -> T { return modifiedVec[x]; });
	}


	template<typename ObjectType>
	inline void UIElementTemplates::ObjectInput(std::function<std::vector<ObjectType*>()> getObjectsFunc, std::function<void(ObjectType*)> setFunc, const std::string& currentObjName)
	{
		GameScene* scenePtr = &Scene;
		auto& compNameButton = TemplateParent.CreateChild<UIButtonActor>("CompNameBox", currentObjName);
		compNameButton.SetTransform(Transform(Vec2f(2.0f, 0.0f), Vec2f(3.0f, 1.0f)));
		compNameButton.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText")->Unstretch();

		compNameButton.SetOnClickFunc([scenePtr, getObjectsFunc, setFunc, &compNameButton]() {
			UIWindowActor& window = scenePtr->CreateActorAtRoot<UIWindowActor>("CompInputWindow");
			auto lol = compNameButton.GetCanvasPtr();
			Transform windowT = *compNameButton.GetTransform();
			windowT.Move(Vec2f(0.0f, -3.0f));
			windowT.ApplyScale(Vec2f(1.0f, 2.0f));
			windowT = compNameButton.GetTransform()->GetParentTransform()->GetWorldTransform() * windowT;
			windowT.SetPosition(Vec3f(dynamic_cast<UICanvasActor*>(lol)->GetTransform()->GetWorldTransformMatrix() *
				Vec4f(Vec3f(lol->GetProjection() * lol->GetViewMatrix() * Vec4f(windowT.GetPos(), 1.0f)), 1.0f)));
			windowT.SetScale(Vec2f(dynamic_cast<UICanvasActor*>(lol)->GetTransform()->GetWorldTransformMatrix() *
				Vec4f(Vec3f(lol->GetProjection() * lol->GetViewMatrix() * Vec4f(windowT.GetScale(), 0.0f)), 0.0f)));
			
			window.SetTransform(windowT);
			window.SetViewScale(Vec2f(1.0f, 2.0f));
			window.SetCloseIfClickedOutside(true);
			window.KillScrollBars();
			window.KillResizeBars();
			window.KillDragButton();
			window.KillCloseButton();

			UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("Available comp list");

			std::vector<ObjectType*> availableObjects = getObjectsFunc();

			auto& deleteMe123 = list.CreateChild<UIButtonActor>("Nullptr object button", [&window, setFunc, &compNameButton]() { setFunc(nullptr); window.MarkAsKilled(); if (auto buttonText = compNameButton.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText")) buttonText->SetContent("nullptr"); });
			deleteMe123.CreateComponent<ScrollingTextComponent>("ButtonText", Transform(Vec2f(0.0f), Vec2f(0.25f, 1.0f)), "Nullptr", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER));
			for (auto& it : availableObjects)
			{
				auto& a = list.CreateChild<UIButtonActor>(it->GetName() + ", Matching object button", [&window, setFunc, it, &compNameButton]() { setFunc(it); window.MarkAsKilled(); if (auto buttonText = compNameButton.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText")) { buttonText->SetContent(it->GetName()); buttonText->Unstretch(); } });
				a.CreateComponent<TextConstantSizeComponent>("ButtonText", Transform(Vec2f(0.0f), Vec2f(0.25f, 1.0f)), it->GetName(), "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER)).SetMaxSize(Vec2f(0.8f));
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
		std::cout << "WYWOLALEM LIST SELECTION " << &(*first) << &(*last) << '\n';
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
		textActor.CreateComponent<TextConstantSizeComponent>("TextComponent", Transform(), promptText, "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::TOP));

		auto& declineButton = confirmationWindow.CreateChild<UIButtonActor>("DeclineButton", "No", [=, &confirmationWindow]() { onDenial(); confirmationWindow.MarkAsKilled(); }, Transform(Vec2f(-0.5f, -0.8f), Vec2f(0.2f)));
		auto& confirmButton = confirmationWindow.CreateChild<UIButtonActor>("ConfirmButton", "Yes", [=, &confirmationWindow]() {  onConfirmation(); confirmationWindow.MarkAsKilled(); }, Transform(Vec2f(0.5f, -0.8f), Vec2f(0.2f)));
	}
}