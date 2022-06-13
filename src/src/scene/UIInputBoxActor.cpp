#include <scene/UIInputBoxActor.h>
#include <game/GameScene.h>
#include <input/Event.h>
#include <scene/TextComponent.h>
#include <scene/ModelComponent.h>
#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	UIInputBoxActor::UIInputBoxActor(GameScene& scene, Actor* parentActor, const std::string& name, const Transform& transform) :
		UIActivableButtonActor(scene, parentActor, name, nullptr, nullptr, transform),
		ValueGetter(nullptr),
		ContentTextComp(nullptr),
		RetrieveContentEachFrame(false),
		CaretComponent(nullptr),
		CaretPosition(0),
		CaretDirtyFlag(GetTransform()->AddDirtyFlag()),
		CaretAnim(0.0f, 0.5f)
	{
		//if (name == "VecBox0")
			//ContentTextComp = &CreateComponent<ScrollingTextComponent>("ButtonText", Transform(Vec2f(0.0f), Vec2f(1.0f)), "0", "", Alignment2D::Center());
		//else
			ContentTextComp = &CreateButtonText("0");

		CaretComponent = &CreateComponent<ModelComponent>("CaretQuad", Transform(Vec2f(0.0f), Vec2f(0.03f, 1.0f)));

		auto caretMaterial = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_E_Caret_Material");
		if (!caretMaterial) 
		{
			caretMaterial = MakeShared<Material>("GEE_E_Caret_Material");
			caretMaterial->SetColor(Vec4f(1.0f));
			GameHandle->GetRenderEngineHandle()->AddMaterial(caretMaterial);
		}
		CaretComponent->AddMeshInst(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));
		CaretComponent->OverrideInstancesMaterial(caretMaterial);
		CaretComponent->SetHide(true);

		CaretAnim.SetOnUpdateFunc([this](float CompType) { return CompType >= 1.0f; });

		bDeactivateOnClickingAgain = false;
		bDeactivateOnPressingEnter = true;


	}

	UIInputBoxActor::UIInputBoxActor(GameScene& scene, Actor* parentActor, const std::string& name, std::function<void(const std::string&)> inputFunc, std::function<std::string()> valueGetter, const Transform& transform) :
		UIInputBoxActor(scene, parentActor, name, transform)
	{
		SetOnInputFunc(inputFunc, valueGetter);
	}

	UIInputBoxActor::UIInputBoxActor(GameScene& scene, Actor* parentActor, const std::string& name, std::function<void(float)> inputFunc, std::function<float()> valueGetter, bool fixNumberStr, const Transform& transform) :
		UIInputBoxActor(scene, parentActor, name, transform)
	{
		SetOnInputFunc(inputFunc, valueGetter, fixNumberStr);
	}

	UIInputBoxActor::UIInputBoxActor(UIInputBoxActor&& inputBox) :
		UIActivableButtonActor(std::move(inputBox)),
		ValueGetter(inputBox.ValueGetter),
		ContentTextComp(nullptr),
		RetrieveContentEachFrame(inputBox.RetrieveContentEachFrame),
		CaretComponent(nullptr),
		CaretPosition(inputBox.CaretPosition),
		CaretDirtyFlag(GetTransform()->AddDirtyFlag()),
		CaretAnim(inputBox.CaretAnim)
	{
		CaretAnim.SetOnUpdateFunc([](float CompType) { return CompType >= 1.0f;  });
	}

	/*UIInputBoxActor::UIInputBoxActor(UIInputBoxActor&& inputBox):
		UIActivableButtonActor(std::move(inputBox)),
		ValueGetter(inputBox.ValueGetter),
		ContentTextComp(nullptr)
	{
	}*/

	void UIInputBoxActor::OnStart()
	{
	}

	ScrollingTextComponent* UIInputBoxActor::GetContentTextComp()
	{
		return ContentTextComp;
	}

	std::string UIInputBoxActor::GetContent()
	{
		if (!ContentTextComp)
			return std::string();

		if (ValueGetter)
			ValueGetter();

		return ContentTextComp->GetContent();
	}

	void UIInputBoxActor::PutString(const std::string& str)
	{
		if (ContentTextComp)
		{
			ContentTextComp->SetContent(str);
			CaretPosition = (ContentTextComp->GetContent().empty()) ? (0) : (ContentTextComp->GetContent().length());
		}
		if (OnDeactivationFunc)
			OnDeactivationFunc();
	}

	void UIInputBoxActor::SetOnInputFunc(std::function<void(const std::string&)> inputFunc, std::function<std::string()> valueGetter)
	{
		if (!ContentTextComp)
			return;

		SetOnDeactivationFunc([=]() { inputFunc(ContentTextComp->GetContent()); if (ValueGetter) ValueGetter();/*valuegetter*/ });

		if (valueGetter)
		{
			auto textConstantSizeCast = dynamic_cast<TextConstantSizeComponent*>(ContentTextComp);
			ValueGetter = [=]() {
				ContentTextComp->SetContent(valueGetter());
				if (textConstantSizeCast)
					textConstantSizeCast->Unstretch();
			};

			ValueGetter();
		}
		else
			ValueGetter = nullptr;
	}

	void UIInputBoxActor::SetOnInputFunc(std::function<void(float)> inputFunc, std::function<float()> valueGetter, bool fixNumberStr)
	{
		std::function<std::string()> valueGetterStr = nullptr;
		if (valueGetter) valueGetterStr = [=]() { return ToStringPrecision(valueGetter()); };
		if (fixNumberStr && valueGetter)
		{
			valueGetterStr = [=]() {
				std::string str = valueGetterStr();
				str = str.erase(str.find_last_not_of('0') + 1, std::string::npos);
				if (str.back() == '.')
					str = str.substr(0, str.length() - 1);

				return str;
			};
		}

		std::function<void(const std::string&)> inputFuncStr;
		if (inputFunc) inputFuncStr = [=](const std::string& content)
		{
			if (content.empty()) return 0.0f;
			try { float val = std::stof(content); inputFunc(val); }
			catch (std::exception& exception) { std::cout << "ERROR: Std exception: " << exception.what() << '\n'; }
		};


		SetOnInputFunc(inputFuncStr, valueGetterStr);
	}

	void UIInputBoxActor::SetRetrieveContentEachFrame(bool retrieveContent)
	{
		RetrieveContentEachFrame = retrieveContent;
	}

	void UIInputBoxActor::SetActive(bool active)
	{
		UIActivableButtonActor::SetActive(active);

		if (active)
		{
			if (!(GetStateBits() & ButtonStateFlags::Active) && ValueGetter)
				ValueGetter();

			if (ContentTextComp)
				SetCaretPosAndUpdateModel(ContentTextComp->GetContent().length());
		}
	}

	void UIInputBoxActor::UpdateValue()
	{
		if (ValueGetter)
			ValueGetter();
	}

	void UIInputBoxActor::Update(float deltaTime)
	{
		UIActivableButtonActor::Update(deltaTime);

		// Caret anim

		if (!(GetStateBits() & ButtonStateFlags::Active))
			CaretComponent->SetHide(true);
		else
		{
			if (GetRoot()->GetTransform().GetDirtyFlag(CaretDirtyFlag))
				UpdateCaretModel(false);
			if (CaretAnim.UpdateT(deltaTime))
			{
				CaretAnim.Reset();
				CaretComponent->SetHide(!CaretComponent->GetHide());
			}
		}

		if (RetrieveContentEachFrame && ValueGetter && !(GetStateBits() & ButtonStateFlags::Active))
			ValueGetter();
	}

	void UIInputBoxActor::HandleEvent(const Event& ev)
	{
		UIActivableButtonActor::HandleEvent(ev);

		if (!ContentTextComp || !(GetStateBits() & ButtonStateFlags::Active))
			return;

		std::string content = ContentTextComp->GetContent();

		if (ev.GetType() == EventType::CharacterEntered)
		{
			const CharEnteredEvent& charEv = dynamic_cast<const CharEnteredEvent&>(ev);
			ContentTextComp->SetContent(content.insert(CaretPosition, charEv.GetUTF8()));
			SetCaretPosAndUpdateModel(++CaretPosition);
		}
		else if (ev.GetType() == EventType::KeyPressed || ev.GetType() == EventType::KeyRepeated)
		{
			const KeyEvent& keyEv = dynamic_cast<const KeyEvent&>(ev);
			std::cout << "Pressed " << (int)keyEv.GetKeyCode() << '\n';
			Key key = keyEv.GetKeyCode();

			if (~keyEv.GetModifierBits() & KeyModifierFlags::NumLock)
			{
				switch (key)
				{
				case Key::Keypad1: key = Key::End; break;
				case Key::Keypad4: key = Key::LeftArrow; break;
				case Key::Keypad6: key = Key::RightArrow; break;
				case Key::Keypad7: key = Key::Home; break;
				}
			}


			if (key == Key::Backspace && CaretPosition != 0)
			{
				ContentTextComp->SetContent((GameHandle->GetInputRetriever().IsKeyPressed(Key::LeftControl)) ? (content.erase(0, CaretPosition)) : (content.erase(CaretPosition - 1, 1))); //erase the last letter
				SetCaretPosAndUpdateModel((GameHandle->GetInputRetriever().IsKeyPressed(Key::LeftControl)) ? (0) : (CaretPosition - 1));
			}
			else if (key == Key::Escape)
			{
				ValueGetter();
				OnDeactivation();
			}
			else if (key == Key::Delete && !content.empty() && CaretPosition != content.length())
			{
				ContentTextComp->SetContent((GameHandle->GetInputRetriever().IsKeyPressed(Key::LeftControl)) ? (content.erase(CaretPosition)) : (content.erase(CaretPosition, 1))); //erase the last letter

				/*
				* Note: do not change the following line. By passing the unchanged CaretPosition, we still update the model.
				* Even though CaretPosition doesn't change (as the nb of letters before the caret is the same), the actual caret model's position does since letters might have different widths.
				*/

				SetCaretPosAndUpdateModel((GameHandle->GetInputRetriever().IsKeyPressed(Key::LeftControl)) ? (GetContentTextComp()->GetContent().length()) : (CaretPosition));
			}
			else if ((key == Key::LeftArrow && keyEv.GetModifierBits() & KeyModifierFlags::Control) || (key == Key::Home) || (key == Key::Keypad7 && ~keyEv.GetModifierBits() & KeyModifierFlags::NumLock))
			{
				SetCaretPosAndUpdateModel(0);
			}
			else if (CaretPosition != 0 && key == Key::LeftArrow)
			{
				SetCaretPosAndUpdateModel(CaretPosition - 1);
			}
			else if ((keyEv.GetModifierBits() & KeyModifierFlags::Control && key == Key::RightArrow) || (key == Key::End))
			{
				SetCaretPosAndUpdateModel(GetContentTextComp()->GetContent().length());
			}
			else if (CaretPosition != GetContentTextComp()->GetContent().length() && key == Key::RightArrow)
			{
				SetCaretPosAndUpdateModel(CaretPosition + 1);
			}
			else if (key == Key::V && keyEv.GetModifierBits() & KeyModifierFlags::Control)
			{
				auto clipboardContent = glfwGetClipboardString(Scene.GetUIData()->GetWindow());
				ContentTextComp->SetContent(content.insert(CaretPosition, clipboardContent));
				SetCaretPosAndUpdateModel(CaretPosition + strlen(clipboardContent));
			}
		}
		else if (ev.GetType() == EventType::CanvasViewChanged)
			UpdateCaretModel(false);
	}

	void UIInputBoxActor::OnClick()		
	{
		UIActivableButtonActor::OnClick();
		if (!(GetStateBits() & ButtonStateFlags::Active) && ValueGetter)
			ValueGetter();

		if (!ContentTextComp)
			return;

		// Position the caret
		Vec2f mousePos = static_cast<Vec2f>(Scene.GetUIData()->GetWindowData().GetMousePositionNDC());
		mousePos = Vec2f(textUtil::OwnedToSpaceTransform(UISpace::Canvas, *GetTransform(), GetCanvasPtr(), &Scene.GetUIData()->GetWindowData()).GetInverse().GetMatrix() * Vec4f(mousePos, 0.0f, 1.0f));

		//Transform contentTextT = GetContentTextComp()->GetTransformCorrectedForSize(false);
		Transform contentTextT = GetContentTextComp()->GetTransform();
		auto textBB = textUtil::ComputeBBox(GetContentTextComp()->GetContent(), contentTextT, *GetContentTextComp()->GetFontVariation(), GetContentTextComp()->GetAlignment());
		float currPos = -textBB.Size.x;
		auto contentStr = ContentTextComp->GetContent();
		auto ch = contentStr.begin();

		for (; ch != contentStr.end(); ch++)
		{
			float advance = ContentTextComp->GetFontVariation()->GetCharacter(*ch).Advance * contentTextT.GetScale().x * 2.0f;
			if (currPos + advance / 2.0f > mousePos.x)  break; // round (advance / 2.0)
			currPos += advance;
		}

		SetCaretPosAndUpdateModel(ch - contentStr.begin());
	}

	void UIInputBoxActor::OnHover()
	{
		UIActivableButtonActor::OnHover();

		GameHandle->SetCursorIcon(DefaultCursorIcon::TextInput);
	}

	void UIInputBoxActor::OnUnhover()
	{
		UIActivableButtonActor::OnUnhover();

		GameHandle->SetCursorIcon(DefaultCursorIcon::Regular);
	}

	void UIInputBoxActor::OnDeactivation()
	{
		UIActivableButtonActor::OnDeactivation();
		if (ValueGetter)
			ValueGetter();
	}
	void UIInputBoxActor::UpdateCaretModel(bool refreshAnim)
	{
		if (!CaretComponent)
			return;

		//auto wholeTextBB = textUtil::ComputeBBox(GetContentTextComp()->GetContent().substr(0, CaretPosition), GetContentTextComp()->GetTransformCorrectedForSize(false), *GetContentTextComp()->GetFontVariation(), GetContentTextComp()->GetAlignment());
		//auto notIncludedText = textUtil::ComputeBBox(GetContentTextComp()->GetContent().substr(CaretPosition), GetContentTextComp()->GetTransformCorrectedForSize(false), *GetContentTextComp()->GetFontVariation(), GetContentTextComp()->GetAlignment());
		auto wholeTextBB = textUtil::ComputeBBox(GetContentTextComp()->GetContent().substr(0, CaretPosition), GetContentTextComp()->GetTransform(), *GetContentTextComp()->GetFontVariation(), GetContentTextComp()->GetAlignment());
		auto notIncludedText = textUtil::ComputeBBox(GetContentTextComp()->GetContent().substr(CaretPosition), GetContentTextComp()->GetTransform(), *GetContentTextComp()->GetFontVariation(), GetContentTextComp()->GetAlignment());

		const float caretWidthPx = 5.0f;
		float caretWidthNDC = (caretWidthPx / static_cast<float>(Scene.GetUIData()->GetWindowData().GetWindowSize().x) / textUtil::ComputeScale(UISpace::Canvas, *GetTransform(), GetCanvasPtr(), &Scene.GetUIData()->GetWindowData()).x);
		CaretComponent->GetTransform().SetScale(Vec2f(caretWidthNDC / 2.0f, 1.0f));
		CaretComponent->GetTransform().SetPosition(Vec2f(wholeTextBB.Position.x + (wholeTextBB.Size.x - notIncludedText.Size.x) + CaretComponent->GetTransform().GetScale2D().x, 0.0f));
		
		std::cout << "====Caret====\n";
		std::cout << "$$$ Whole text pos: " << wholeTextBB.Position << "\n";
		std::cout << "$$$ Whole text size: " << wholeTextBB.Size << "\n";
		std::cout << "$$$ Whole text size: " << wholeTextBB.Size << "\n";
		std::cout << "\n";


		if (refreshAnim)
		{
			CaretAnim.Reset();
			CaretComponent->SetHide(false);
		}
	}
	void UIInputBoxActor::SetCaretPosAndUpdateModel(unsigned int changeCaretPos)
	{
		CaretPosition = changeCaretPos;
		UpdateCaretModel(true);
	}
}