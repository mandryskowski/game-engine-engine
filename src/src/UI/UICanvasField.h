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

	struct CanvasFieldStyle
	{
		Transform CaptionTextArea = Transform(Vec2f(-3.5f, 0.0f), Vec2f(3.5f, 1.0f));
		Transform ContentArea = Transform(Vec2f(3.5f, 0.0f), Vec2f(3.5f, 1.0f));

		Boxf<Vec2f> GetBoundingBox()
		{
			return Boxf<Vec2f>::FromMinMaxCorners(glm::min(CaptionTextArea.GetPos2D() - CaptionTextArea.GetScale2D(), ContentArea.GetPos2D() - ContentArea.GetScale2D()), 
				glm::max(CaptionTextArea.GetPos2D() + CaptionTextArea.GetScale2D(), ContentArea.GetPos2D() + ContentArea.GetScale2D()));
		}

	}; 

	class UICanvasField : public Actor, public UIActor
	{
	public:
		UICanvasField(GameScene&, Actor* parentActor, const std::string& name, const BuilderStyle& defaultStyle = BuilderStyle());
		virtual void OnStart() override;
		UIElementTemplates GetTemplates();
		TextComponent* GetTitleComp() const;

	private:
		TextComponent* TitleComp;
		BuilderStyle DefaultStyle;
	};

	class UICanvasFieldCategory : public UIAutomaticListActor
	{
	public:
		UICanvasFieldCategory(GameScene& scene, Actor* parentActor, const std::string& name, const BuilderStyle& defaultStyle = BuilderStyle());

		virtual void OnStart() override;
		UIButtonActor* GetExpandButton() const;

		UIElementTemplates GetTemplates();

		UICanvasField& AddField(const std::string& name, const std::function<Vec3f()>& getElementOffset = nullptr);
		UICanvasFieldCategory& AddCategory(const std::string& name);

		virtual Vec3f GetListOffset() override;
		void SetOnExpansionFunc(const std::function<void()>& onExpansionFunc);

		virtual void HandleEventAll(const Event& ev) override;

		virtual void Refresh() override;

	private:
		std::function<void()> OnExpansionFunc;
		UIButtonActor* ExpandButton;
		ModelComponent* CategoryBackgroundQuad;
		BuilderStyle DefaultStyle;
	};


	namespace uiCanvasFieldUtil
	{
		void SetupComponents(Actor& actor, const Vec2f& pos = Vec2f(0.0f), const Vec4f& backgroundColor = Vec4f(hsvToRgb(Vec3f(218.0f, 0.521f, 0.188f)), 1.0f), const Vec4f& separatorColor = Vec4f(0.18f, 0.25f, 0.39f, 1.0f));
	}


	// Tied to an actor
	class UIElementTemplates
	{
	public:
		UIElementTemplates(Actor&, const BuilderStyle& style = BuilderStyle());


		void TickBox(bool& modifiedBool) const;
		void TickBox(const std::function<bool()>& setFunc) const;
		void TickBox(const std::function<void(bool)>& setFunc, const std::function<bool()>& getFunc) const;
		
		struct VecInputDescription
		{
			std::vector<UIInputBoxActor*> VecInputBoxes;
		};
		template <MathContainerUnit length, typename T = float> VecInputDescription VecInput(std::function<void(int, T)> setFunc, std::function<T(int)> getFunc);	//encapsulated
		template <MathContainerUnit length, typename T, glm::qualifier Q> VecInputDescription VecInput(Vec<length, T, Q>& modifiedVec);	//not encapsulated

		/**
		 * @brief Creates a slider which is a child of the actor tied to this UIElementTemplates object.
		 * @param processUnitValue: the function used to process the selected value which is from 0 to 1 (unit interval).
		 * @param defaultValue: initial value that the slider is set to.
		*/
		void SliderUnitInterval(std::function<void(float)> processUnitValue, float defaultValue = 0.5f) const;
		void SliderRawInterval(float begin, float end, const std::function<void(float)>& processRawValue, float defaultValue = 0.5f) const;

		//
		template <typename ObjectType> void ObjectInput(std::function<std::vector<ObjectType*>()> getObjectsFunc, std::function<void(ObjectType*)> setFunc, const std::string& currentObjName);					//encapsulated
		template <typename ObjectType> void ObjectInput(std::function<std::vector<ObjectType*>()> getObjectsFunc, std::function<void(ObjectType*)> setFunc, const ObjectType* currentObjNameGetter = nullptr)	//encapsulated
		{
			ObjectInput<ObjectType>(getObjectsFunc, setFunc, (currentObjNameGetter) ? (currentObjNameGetter->GetName()) : ("<nothing>"));
		}
		template <typename ObjectBase, typename ObjectType> void ObjectInput(ObjectBase& hierarchyRoot, std::function<void(ObjectType*)> setFunc, const std::string& currentObjName);							//encapsulated
		template <typename ObjectBase, typename ObjectType> void ObjectInput(ObjectBase& hierarchyRoot, std::function<void(ObjectType*)> setFunc, const ObjectType* currentObjNameGetter = nullptr)				//encapsulated
		{
			ObjectInput<ObjectBase, ObjectType>(hierarchyRoot, setFunc, (currentObjNameGetter) ? (currentObjNameGetter->GetName()) : ("<nothing>"));
		}
		template <typename ObjectBase, typename ObjectType> void ObjectInput(ObjectBase& hierarchyRoot, ObjectType*& inputTo);																					//not encapsulated

		template <typename T, typename InputIt> void ListSelection(InputIt first, InputIt last, std::function<void(UIButtonActor&, T&)> buttonFunc);
		template <typename T, typename InputIt> void ListSelection(InputIt first, InputIt last, std::function<void(UIAutomaticListActor&, T&)> buttonFunc);


	private:
		void HierarchyTreeInput(UIAutomaticListActor& list, GameScene&, const std::function<void(Hierarchy::Tree&)>&
		                        setFunc) const;
	public:
		void HierarchyTreeInput(GameScene&, std::function<void(Hierarchy::Tree&)> setFunc) const;
		void HierarchyTreeInput(GameManager&, const std::function<void(Hierarchy::Tree&)>& setFunc) const;

		void PathInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc, const std::vector<const char*>
		               & extensions) const;	//encapsulated
		void FolderInput(std::function<void(const std::string&)> setFunc, std::function<std::string()> getFunc, const std::string& defaultPath = "C:/") const;	//encapsulated

	private:
		Actor& TemplateParent;
		GameScene& Scene;
		GameManager& GameHandle;

		BuilderStyle Style;
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
	template <MathContainerUnit length, typename T>
	UIElementTemplates::VecInputDescription UIElementTemplates::VecInput(std::function<void(int, T)> setFunc, std::function<T(int)> getFunc)
	{
		const std::string materialNames[] = {
			"GEE_Default_X_Axis", "GEE_Default_Y_Axis", "GEE_Default_Z_Axis", "GEE_Default_W_Axis"
		};

		GEE_CORE_ASSERT(length <= 4);

		VecInputDescription desc;
		char axisChar[] = {'x', 'y', 'z', 'w'};
		auto vecButtonT = Style.GetVecButtonT();
		for (int axis = 0; axis < length; axis++)
		{
			UIInputBoxActor& inputBoxActor = TemplateParent.CreateChild<UIInputBoxActor>("VecBox" + std::to_string(axis));
			desc.VecInputBoxes.push_back(&inputBoxActor);

			inputBoxActor.SetTransform(vecButtonT);
			vecButtonT.Move(Vec2f(2.0f, 0.0f));

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
			if (GameHandle.GetDefInputRetriever().IsKeyPressed(Key::LeftAlt))
				for (auto box : allBoxes)
					box->SetActive(true);
			}); 

		return desc;
	}

	template<MathContainerUnit length, typename T, glm::qualifier Q>
	UIElementTemplates::VecInputDescription UIElementTemplates::VecInput(Vec<length, T, Q>& modifiedVec)
	{
		return VecInput<length, T>([&](int x, T val) { modifiedVec[static_cast<MathContainerUnit>(x)] = val; }, [&](int x) -> T { return modifiedVec[static_cast<MathContainerUnit>(x)]; });
	}


	template<typename ObjectType>
	void UIElementTemplates::ObjectInput(std::function<std::vector<ObjectType*>()> getObjectsFunc, std::function<void(ObjectType*)> setFunc, const std::string& currentObjName)
	{
		GameScene* scenePtr = &Scene;
		auto& compNameButton = TemplateParent.CreateChild<UIButtonActor>("CompNameBox", currentObjName, nullptr, Style.GetLongButtonT());

		compNameButton.SetOnClickFunc([scenePtr, getObjectsFunc, setFunc, &compNameButton]() {
			UIWindowActor& window = scenePtr->CreateActorAtRoot<UIWindowActor>("CompInputWindow");
			auto canvas = compNameButton.GetCanvasPtr();
			Transform windowT = *compNameButton.GetTransform();
			windowT.Move(Vec2f(0.0f, -2.0f));
			windowT.ApplyScale(Vec2f(1.0f, 1.0f));
			windowT = compNameButton.GetTransform()->GetParentTransform()->GetWorldTransform() * windowT;

			Mat4f optionalCanvasMatrix(1.0f);
			if (canvas)
				optionalCanvasMatrix = dynamic_cast<UICanvasActor*>(canvas)->GetTransform()->GetWorldTransformMatrix() * canvas->GetProjection() * canvas->GetViewMatrix();
			
			windowT.SetPosition(Vec2f(optionalCanvasMatrix * Vec4f(windowT.GetPos(), 1.0f)));
			windowT.SetScale(Vec2f(optionalCanvasMatrix * Vec4f(windowT.GetScale(), 0.0f)) * Vec2f(1.0f, 3.0f));
			
			window.SetTransform(windowT);
			window.SetCanvasView(Transform(Vec2f(0.0f), Vec2f(1.0f, 3.0f)), true);
			window.SetCloseIfClickedOutside(true);
			window.KillScrollBars();
			window.KillResizeBars();
			window.KillDragButton();
			window.KillCloseButton();

			UIAutomaticListActor& list = window.CreateChild<UIAutomaticListActor>("Available comp list");
			//list.GetTransform()->SetScale(Vec2f(1.0f, 1.0f/ 3.0f));

			std::vector<ObjectType*> availableObjects = getObjectsFunc();

			
			auto& deleteMe123 = list.CreateChild<UIButtonActor>("Nullptr object button", "<nothing>", [&window, setFunc, &compNameButton]() { setFunc(nullptr); window.MarkAsKilled(); if (auto buttonText = compNameButton.GetRoot()->GetComponent<TextComponent>("ButtonText")) buttonText->SetContent("nullptr"); });
			//deleteMe123.CreateComponent<ScrollingTextComponent>("ButtonText", Transform(Vec2f(0.0f), Vec2f(0.25f, 1.0f)), "Nullptr", "", Alignment2D::LeftCenter());
			for (auto& it : availableObjects)
			{
				auto& a = list.CreateChild<UIButtonActor>(it->GetName() + ", Matching object button", it->GetName(), [&window, setFunc, it, &compNameButton]() { setFunc(it); window.MarkAsKilled(); if (auto buttonText = compNameButton.GetRoot()->GetComponent<TextComponent>("ButtonText")) { buttonText->SetContent(it->GetName()); buttonText->Unstretch(); } });
			}
			list.Refresh();

			std::cout << availableObjects.size() << " available objects.\n";

			window.ClampViewToElements();
			});
	}

	template<typename ObjectBase, typename ObjectType>
	void UIElementTemplates::ObjectInput(ObjectBase& hierarchyRoot, std::function<void(ObjectType*)> setFunc, const std::string& currentObjName)
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
	void UIElementTemplates::ObjectInput(ObjectBase& hierarchyRoot, ObjectType*& inputTo)
	{
		ObjectInput<ObjectBase, ObjectType>(hierarchyRoot, [&inputTo](ObjectType* comp) { inputTo = comp; }, (inputTo) ? (inputTo->GetName()) : ("<nothing>"));
	}

	template<typename T, typename InputIt>
	void UIElementTemplates::ListSelection(InputIt first, InputIt last, std::function<void(UIButtonActor&, T&)> buttonFunc)
	{
		ListSelection<T, InputIt>(first, last, [buttonFunc](UIAutomaticListActor& listActor, T& object) { buttonFunc(listActor.CreateChild<UIButtonActor>("ListElementActor"), object); });
	}
	template<typename T, typename InputIt>
	void UIElementTemplates::ListSelection(InputIt first, InputIt last, std::function<void(UIAutomaticListActor&, T&)> buttonFunc)
	{
		UIAutomaticListActor& listActor = TemplateParent.CreateChild<UIAutomaticListActor>("ListSelectionActor");
		std::for_each(first, last, [&listActor, buttonFunc](T& object) { buttonFunc(listActor, object); std::cout << " TWORZE PRZYCISKA"; });
		listActor.Refresh();
	}

	// Generic UI templates
	template<typename FunctorConfirm, typename FunctorDeny>
	void GenericUITemplates::ConfirmationBox(FunctorConfirm&& onConfirmation, FunctorDeny&& onDenial, const std::string& promptText)
	{
		auto& confirmationWindow = Scene.CreateActorAtRoot<UIWindowActor>("Confirmation window");
		confirmationWindow.KillResizeBars();

		auto& textActor = confirmationWindow.CreateChild<UIActorDefault>("TextActor", Transform(Vec2f(0.0f, 1.0f), Vec2f(0.9f)));
		textActor.CreateComponent<TextComponent>("TextComponent", Transform(), promptText, "", Alignment2D::Top()).SetMaxSize(Vec2f(1.0f));

		auto& declineButton = confirmationWindow.CreateChild<UIButtonActor>("DeclineButton", "No", [=, &confirmationWindow]() { onDenial(); confirmationWindow.MarkAsKilled(); }, Transform(Vec2f(-0.5f, -0.8f), Vec2f(0.2f)));
		auto& confirmButton = confirmationWindow.CreateChild<UIButtonActor>("ConfirmButton", "Yes", [=, &confirmationWindow]() {  onConfirmation(); confirmationWindow.MarkAsKilled(); }, Transform(Vec2f(0.5f, -0.8f), Vec2f(0.2f)));
	}
} 