#include <scene/UIInputBoxActor.h>
#include <input/Event.h>
#include <scene/TextComponent.h>
#include <scene/ModelComponent.h>
#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	UIInputBoxActor::UIInputBoxActor(GameScene& scene, Actor* parentActor, const std::string& name) :
		UIActivableButtonActor(scene, parentActor, name),
		ValueGetter(nullptr),
		ContentTextComp(nullptr),
		RetrieveContentEachFrame(false),
		CaretComponent(nullptr)
	{
		//if (name == "VecBox0")
			//ContentTextComp = &CreateComponent<ScrollingTextComponent>("ButtonText", Transform(Vec2f(0.0f), Vec2f(1.0f)), "0", "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
		//else
			ContentTextComp = &CreateButtonText("0");

		return;
		CaretComponent = &CreateComponent<ModelComponent>("CaretQuad", Transform(Vec2f(0.0f), Vec2f(0.05f, 1.0f)));

		auto caretMaterial = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_E_Caret_Material");
		if (!caretMaterial) 
		{
			caretMaterial = MakeShared<Material>("GEE_E_Caret_Material");
			caretMaterial->SetColor(Vec4f(1.0f));
			GameHandle->GetRenderEngineHandle()->AddMaterial(caretMaterial);
		}
		CaretComponent->AddMeshInst(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		CaretComponent->OverrideInstancesMaterial(caretMaterial);

	}

	UIInputBoxActor::UIInputBoxActor(GameScene& scene, Actor* parentActor, const std::string& name, std::function<void(const std::string&)> inputFunc, std::function<std::string()> valueGetter) :
		UIInputBoxActor(scene, parentActor, name)
	{
		SetOnInputFunc(inputFunc, valueGetter);
	}

	UIInputBoxActor::UIInputBoxActor(GameScene& scene, Actor* parentActor, const std::string& name, std::function<void(float)> inputFunc, std::function<float()> valueGetter, bool fixNumberStr) :
		UIInputBoxActor(scene, parentActor, name)
	{
		SetOnInputFunc(inputFunc, valueGetter, fixNumberStr);
	}

	UIInputBoxActor::UIInputBoxActor(UIInputBoxActor&& inputBox) :
		UIActivableButtonActor(std::move(inputBox)),
		ValueGetter(inputBox.ValueGetter),
		ContentTextComp(nullptr),
		RetrieveContentEachFrame(inputBox.RetrieveContentEachFrame),
		CaretComponent(nullptr),
		CaretPosition(inputBox.CaretPosition)
	{
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
			ContentTextComp->SetContent(str);
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
		std::function<std::string()> valueGetterStr = [=]() { return ToStringPrecision(valueGetter()); };
		if (fixNumberStr)
		{
			valueGetterStr = [=]() {
				std::string str = valueGetterStr();
				str = str.erase(str.find_last_not_of('0') + 1, std::string::npos);
				if (str.back() == '.')
					str = str.substr(0, str.length() - 1);

				return str;
			};
		}

		SetOnInputFunc([=](const std::string& content) {
			if (content.empty()) return 0.0f;
			try { float val = std::stof(content); inputFunc(val); }
			catch (std::exception& exception) { std::cout << "ERROR: Std exception: " << exception.what() << '\n'; }
			},
			valueGetterStr);
	}

	void UIInputBoxActor::SetRetrieveContentEachFrame(bool retrieveContent)
	{
		RetrieveContentEachFrame = retrieveContent;
	}

	void UIInputBoxActor::Update(float deltaTime)
	{
		UIActivableButtonActor::Update(deltaTime);



		if (RetrieveContentEachFrame && ValueGetter && GetState() != EditorButtonState::Activated)
			ValueGetter();
	}

	void UIInputBoxActor::HandleEvent(const Event& ev)
	{
		UIActivableButtonActor::HandleEvent(ev);

		if (!ContentTextComp || State != EditorButtonState::Activated)
			return;

		const std::string& content = ContentTextComp->GetContent();

		if (ev.GetType() == EventType::CharacterEntered)
		{
			const CharEnteredEvent& charEv = dynamic_cast<const CharEnteredEvent&>(ev);
			ContentTextComp->SetContent(content + charEv.GetUTF8());
		}
		else if (ev.GetType() == EventType::KeyPressed || ev.GetType() == EventType::KeyRepeated)
		{
			const KeyEvent& keyEv = dynamic_cast<const KeyEvent&>(ev);
			if (keyEv.GetKeyCode() == Key::Backspace && !content.empty())
				ContentTextComp->SetContent((GameHandle->GetInputRetriever().IsKeyPressed(Key::LeftControl)) ? (std::string()) : (content.substr(0, content.length() - 1))); //erase the last letter
		}
	}

	void UIInputBoxActor::OnClick()
	{
		UIActivableButtonActor::OnClick();
		if (ValueGetter)
			ValueGetter();
	}

	void UIInputBoxActor::OnDeactivation()
	{
		UIActivableButtonActor::OnDeactivation();
		if (ValueGetter)
			ValueGetter();
	}
}