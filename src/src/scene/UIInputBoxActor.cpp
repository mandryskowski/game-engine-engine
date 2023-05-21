#include <scene/UIInputBoxActor.h>
#include <game/GameScene.h>
#include <input/Event.h>
#include <scene/TextComponent.h>
#include <scene/ModelComponent.h>
#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	UIInputBoxActor::UIInputBoxActor(GameScene& scene, Actor* parentActor, const String& name, const Transform& transform) :
		UIActivableButtonActor(scene, parentActor, name, nullptr, nullptr, transform),
		ValueGetter(nullptr),
		ContentTextComp(nullptr),
		RetrieveContentEachFrame(false),
		CaretComponent(nullptr),
		CaretPosition(0),
		CaretDirtyFlag(GetTransform()->AddWorldDirtyFlag()),
		CaretAnim(0.0f, 0.5f),
		TextSelectionRange(-1),
		CaretNDCWidth(0.0f)
	{
		CreateButtonText("0");

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

		CaretAnim.SetOnUpdateFunc([this](double T) { return T >= 1.0; });

		bDeactivateOnClickingAgain = false;
		bDeactivateOnPressingEnter = true;
	}

	UIInputBoxActor::UIInputBoxActor(GameScene& scene, Actor* parentActor, const String& name, std::function<void(const String&)> inputFunc, std::function<String()> valueGetter, const Transform& transform) :
		UIInputBoxActor(scene, parentActor, name, transform)
	{
		SetOnInputFunc(inputFunc, valueGetter);
	}

	UIInputBoxActor::UIInputBoxActor(GameScene& scene, Actor* parentActor, const String& name, std::function<void(float)> inputFunc, std::function<float()> valueGetter, bool fixNumberStr, const Transform& transform) :
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
		CaretDirtyFlag(GetTransform()->AddWorldDirtyFlag()),
		CaretAnim(inputBox.CaretAnim),
		CaretNDCWidth(inputBox.CaretNDCWidth)
	{
		CaretAnim.SetOnUpdateFunc([](double T) { return T >= 1.0;  });
	}

	/*UIInputBoxActor::UIInputBoxActor(UIInputBoxActor&& inputBox):
		UIActivableButtonActor(std::move(inputBox)),
		ValueGetter(inputBox.ValueGetter),
		ContentTextComp(nullptr)
	{
	}*/

	void UIInputBoxActor::OnStart()
	{
		if (CaretNDCWidth == 0.0f)
			SetCaretWidthPx(5);
	}

	ScrollingTextComponent* UIInputBoxActor::GetContentTextComp()
	{
		return ContentTextComp;
	}

	String UIInputBoxActor::GetContent()
	{
		if (!ContentTextComp)
			return String();

		if (ValueGetter)
			ValueGetter();

		return ContentTextComp->GetContent();
	}

	void UIInputBoxActor::PutString(const String& str)
	{
		if (ContentTextComp)
		{
			ContentTextComp->SetContent(str);
			CaretPosition = ContentTextComp->GetContent().empty() ? 0U : static_cast<unsigned int>(ContentTextComp->GetContent().length());
		}
		if (OnDeactivationFunc)
			OnDeactivationFunc();
	}

	void UIInputBoxActor::SetOnInputFunc(std::function<void(const String&)> inputFunc, std::function<String()> valueGetter)
	{
		if (!ContentTextComp)
			return;

		if (inputFunc)
			SetOnDeactivationFunc([=]() { inputFunc(ContentTextComp->GetContent()); if (ValueGetter) ValueGetter(); });
		else
			SetOnDeactivationFunc([=]() { if (ValueGetter) ValueGetter(); });

		auto textConstantSizeCast = dynamic_cast<TextComponent*>(ContentTextComp);
		if (valueGetter)
		{
			ValueGetter = [=]() {
				ContentTextComp->SetContent(valueGetter());
				if (textConstantSizeCast)
					textConstantSizeCast->Unstretch();
			};

			ValueGetter();
		}
		else
		{
			ValueGetter = nullptr;
			if (textConstantSizeCast)
				textConstantSizeCast->Unstretch();
		}
	}

	void UIInputBoxActor::SetOnInputFunc(std::function<void(float)> inputFunc, std::function<float()> valueGetter, bool fixNumberStr)
	{
		std::function<String()> valueGetterStr = nullptr;
		if (valueGetter) valueGetterStr = [=]() { return ToStringPrecision(valueGetter()); };
		if (fixNumberStr && valueGetter)
		{
			valueGetterStr = [=]() {
				String str = valueGetterStr();
				str = str.erase(str.find_last_not_of('0') + 1, String::npos);
				if (str.back() == '.')
					str = str.substr(0, str.length() - 1);

				return str;
			};
		}

		std::function<void(const String&)> inputFuncStr;
		if (inputFunc) inputFuncStr = [=](const String& content)
		{
			if (content.empty()) return;
			try { auto val = std::stof(content); inputFunc(val); }
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
				SetCaretPosAndUpdateModel(static_cast<unsigned>(ContentTextComp->GetContent().length()));
		}
	}

	void UIInputBoxActor::SetCaretWidthPx(unsigned int pixelWidth)
	{
		auto windowData = Scene.GetUIData()->GetWindowData();
		CaretNDCWidth = (static_cast<float>(pixelWidth) / static_cast<float>(windowData.GetWindowSize().x) / TextUtil::ComputeScale(UISpace::Canvas, *GetTransform(), GetCanvasPtr(), &windowData).x);
	}

	void UIInputBoxActor::UpdateValue()
	{
		if (ValueGetter)
			ValueGetter();
	}

	void UIInputBoxActor::Update(Time dt)
	{
		UIActivableButtonActor::Update(dt);

		if (IsBeingKilled())
			return;

		UpdateCaretModel(false);
		// Caret anim

		if (!(GetStateBits() & ButtonStateFlags::Active))
			CaretComponent->SetHide(true);
		else
		{
			if (GetRoot()->GetTransform().GetWorldDirtyFlag(CaretDirtyFlag))
				UpdateCaretModel(false);
			if (CaretAnim.UpdateT(dt))
			{
				CaretAnim.Reset();

				// If caret outside input box, always hide. Otherwise, flip the flag.
				CaretComponent->SetHide(!CaretComponent->GetHide() || !IsCaretInsideInputBox());
			}
		}

		if (RetrieveContentEachFrame && ValueGetter && !(GetStateBits() & ButtonStateFlags::Active))
			ValueGetter();
	}

	void UIInputBoxActor::HandleEvent(const Event& ev)
	{
		UIActivableButtonActor::HandleEvent(ev);

		auto inputRetriever = Scene.GetUIData()->GetInputRetriever();

		if (!ContentTextComp || !(GetStateBits() & ButtonStateFlags::Active))
			return;

		String content = ContentTextComp->GetContent();

		NormalizeCaretPosition();

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

			switch (key)
			{

			case Key::Backspace:
				if (CaretPosition != 0)
				{
					ContentTextComp->SetContent((inputRetriever.IsKeyPressed(Key::LeftControl)) ? (content.erase(0, CaretPosition)) : (content.erase(CaretPosition - 1, 1))); //erase the last letter
					SetCaretPosAndUpdateModel((inputRetriever.IsKeyPressed(Key::LeftControl)) ? (0) : (CaretPosition - 1));
				}
				break;

			case Key::Escape:
				ValueGetter();
				OnDeactivation();
				break;

			case Key::Delete:
				if (!content.empty() && CaretPosition != content.length())
				{
					ContentTextComp->SetContent((inputRetriever.IsKeyPressed(Key::LeftControl)) ? (content.erase(CaretPosition)) : (content.erase(CaretPosition, 1))); //erase the last letter

					/*
					* Note: do not change the following line. By passing the unchanged CaretPosition, we still update the model.
					* Even though CaretPosition doesn't change (as the nb of letters before the caret is the same), the actual caret model's position does since letters might have different widths.
					*/

					SetCaretPosAndUpdateModel((inputRetriever.IsKeyPressed(Key::LeftControl)) ? (static_cast<unsigned>(GetContentTextComp()->GetContent().length())) : (CaretPosition));
				}
				break;

			case Key::LeftArrow:
				if (keyEv.GetModifierBits() & KeyModifierFlags::Control)
				{
					case Key::Home:
					SetCaretPosAndUpdateModel(0);
					break;
				}
				else if (CaretPosition != 0)
				{
					SetCaretPosAndUpdateModel(CaretPosition - 1);
				}
				break;

			case Key::Keypad7:
				if ((~keyEv.GetModifierBits() & KeyModifierFlags::NumLock))
					SetCaretPosAndUpdateModel(0);
				break;

			case Key::RightArrow:
				if (keyEv.GetModifierBits() & KeyModifierFlags::Control && key == Key::RightArrow)
				{
					case Key::End:
					SetCaretPosAndUpdateModel(static_cast<unsigned>(GetContentTextComp()->GetContent().length()));
					break;
				}
				else if (CaretPosition != GetContentTextComp()->GetContent().length())
				{
					SetCaretPosAndUpdateModel(CaretPosition + 1);
				}
				break;

			case Key::V:
				if (keyEv.GetModifierBits() & KeyModifierFlags::Control)
				{
					auto clipboardContent = glfwGetClipboardString(Scene.GetUIData()->GetWindow());
					ContentTextComp->SetContent(content.insert(CaretPosition, clipboardContent));
					SetCaretPosAndUpdateModel(CaretPosition + static_cast<unsigned>(strlen(clipboardContent)));
				}
				break;
			};
		}
		else if (ev.GetType() == EventType::CanvasViewChanged)
		{
			SetCaretWidthPx(5);
			UpdateCaretModel(false);
		}
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
		auto windowData = Scene.GetUIData()->GetWindowData();
		mousePos = Vec2f(TextUtil::OwnedToSpaceTransform(UISpace::Canvas, *GetTransform(), GetCanvasPtr(), &windowData).GetInverse().GetMatrix() * Vec4f(mousePos, 0.0f, 1.0f));

		Transform contentTextT = GetContentTextComp()->GetCorrectedTransform(false);
		auto textBB = TextUtil::ComputeBBox(GetContentTextComp()->GetContent(), contentTextT, *GetContentTextComp()->GetFontVariation(), GetContentTextComp()->GetAlignment());
		float currPos = textBB.Position.x - textBB.Size.x;
		auto contentStr = ContentTextComp->GetContent();
		auto ch = contentStr.begin();

		for (; ch != contentStr.end(); ch++)
		{
			float advance = ContentTextComp->GetFontVariation()->GetCharacter(*ch).Advance * contentTextT.GetScale().x * 2.0f;
			if (currPos + advance / 2.0f > mousePos.x)  break; // round (advance / 2.0)
			currPos += advance;
		}

		SetCaretPosAndUpdateModel(static_cast<unsigned int>(ch - contentStr.begin()));
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
		if (!GetContent().empty())
		{
			CaretPosition = 0;
			ContentTextComp->ScrollToLetter(0);
		}
		if (ValueGetter)
			ValueGetter();
	}

	void UIInputBoxActor::CreateButtonText(const String& content)
	{
		ContentTextComp = &CreateComponent<ScrollingTextComponent>("ButtonText", Transform(Vec2f(0.0f), Vec2f(1.0f)), content, "", Alignment2D::Center());
		ContentTextComp->Unstretch();
	}

	void UIInputBoxActor::UpdateCaretModel(bool refreshAnim)
	{
		if (!CaretComponent)
			return;

		NormalizeCaretPosition();

		auto text = GetContentTextComp();
		auto leftBB = TextUtil::ComputeBBox(text->GetContent().substr(0, CaretPosition), text->GetCorrectedTransform(false), *text->GetFontVariation(), text->GetAlignment());
		auto rightBB = TextUtil::ComputeBBox(text->GetContent().substr(CaretPosition), text->GetCorrectedTransform(false), *text->GetFontVariation(), text->GetAlignment());


		if (text->GetAlignment().GetHorizontal() == Alignment::Left)
			rightBB.Position.x += leftBB.Size.x * 2.0f;
		else if (text->GetAlignment().GetHorizontal() == Alignment::Center)
		{
			leftBB.Position.x -= rightBB.Size.x;
			rightBB.Position.x += leftBB.Size.x;
		}

		CaretComponent->GetTransform().SetScale(Vec2f(CaretNDCWidth / 2.0f, 1.0f));
		CaretComponent->GetTransform().SetPosition(Vec2f(leftBB.Position.x + (leftBB.Size.x) + CaretComponent->GetTransform().GetScale2D().x, 0.0f));

		/*ModelComponent* wholeTextQuad = GetRoot()->GetComponent<ModelComponent>("wholeTextQuad"), * notIncludedQuad = GetRoot()->GetComponent<ModelComponent>("notIncludedQuad");
		if (!wholeTextQuad || !notIncludedQuad)
		{
			wholeTextQuad = &CreateComponent<ModelComponent>("wholeTextQuad");
			wholeTextQuad->AddMeshInst(MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad), MakeShared<Material>("testmaerial", Vec4f(1.0f, 0.3f, 0.3f, 0.5f))));
			notIncludedQuad = &CreateComponent<ModelComponent>("notIncludedQuad");
			notIncludedQuad->AddMeshInst(MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad), MakeShared<Material>("asdsad", Vec4f(0.3f, 0.3f, 1.0f, 0.5f))));
		}

		wholeTextQuad->SetTransform(Transform(leftBB.Position, leftBB.Size));
		notIncludedQuad->SetTransform(Transform(rightBB.Position, rightBB.Size));*/

		if (!IsCaretInsideInputBox())
		{
			CaretComponent->SetHide(true);
			
			// If we are scrolling left, scroll to next letter after caret to make it more readable.
			bool caretOnLeftSide = CaretComponent->GetTransform().GetPos2D().x < 0.0f;
			ContentTextComp->ScrollToLetter((caretOnLeftSide) ? (CaretPosition + 1) : (CaretPosition));
		}
		else if (refreshAnim)
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
	bool UIInputBoxActor::IsCaretInsideInputBox()
	{
		// 1.0 is the right side of the input box in the caret's local space
		return glm::abs(CaretComponent->GetTransform().GetPos2D().x) <= 1.0f + CaretNDCWidth;
	}
	void UIInputBoxActor::NormalizeCaretPosition()
	{
		CaretPosition = glm::clamp(CaretPosition, static_cast<unsigned int>(0), static_cast<unsigned int>(ContentTextComp->GetContent().length()));
	}
}