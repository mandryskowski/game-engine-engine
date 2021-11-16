#include <editor/EditorActions.h>
#include <UI/UIListActor.h>
#include <scene/UIButtonActor.h>
#include <UI/UICanvasActor.h>
#include <scene/TextComponent.h>




#include <scene/ModelComponent.h>
#include <editor/EditorManager.h>
namespace GEE
{
	PopupDescription::PopupDescription(SystemWindow& window, UIAutomaticListActor* actor, UICanvasActor* canvas):
		Window(window),
		DescriptionRoot(actor),
		PopupCanvas(canvas),
		bViewComputed(false),
		LastComputedView(Transform()),
		PopupOptionSizePx(Vec2i(240, 30))
	{
		SharedPtr<Material> secondaryMat;
		if ((secondaryMat = actor->GetGameHandle()->GetRenderEngineHandle()->FindMaterial("GEE_E_Popup_Secondary")) == nullptr)
		{
			secondaryMat = MakeShared<Material>("GEE_E_Popup_Secondary");
			secondaryMat->SetColor(Vec3f(0.5f, 0.3f, 0.5f));
			secondaryMat->SetRenderShaderName("Forward_NoLight");
			actor->GetGameHandle()->GetRenderEngineHandle()->AddMaterial(secondaryMat);
		}
		PopupCanvas->CreateCanvasBackgroundModel(Vec3f(0.5f, 0.3f, 0.5f));
	}
	void PopupDescription::AddOption(const std::string& buttonContent, std::function<void()> buttonFunc, const IconData& optionalIconData)
	{
		auto& button = DescriptionRoot->CreateChild<UIButtonActor>("Button_" + buttonContent, buttonContent, 
		[window = &Window, buttonFunc]()
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			if (buttonFunc)
				buttonFunc();
		}, Transform(Vec2f(0.0f), Vec2f(0.9f)));

		if (optionalIconData.IsValid())
		{
			auto& iconModel = button.CreateComponent<ModelComponent>("GEE_E_Button_Icon", Transform(Vec2f(-0.7f, 0.0f), Vec2f(0.1f, 0.8f)));
			iconModel.AddMeshInst(MeshInstance(DescriptionRoot->GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), optionalIconData.GetMatInstance()));
		}

		DescriptionRoot->Refresh();
		PopupCanvas->AutoClampView(true, true);
		//button.GetRoot()->GetComponent<TextConstantSizeComponent>("ButtonText")->Unstretch(UISpace::Window);
	}
	PopupDescription PopupDescription::AddSubmenu(const std::string& buttonContent, std::function<void(PopupDescription)> prepareSubmenu, const IconData& optionalIconData)
	{
		int optionIndex = DescriptionRoot->GetListElementCount();

		auto createSubPopup = [gameHandle = DescriptionRoot->GetGameHandle(), optionIndex, *this, prepareSubmenu, window = &Window]() mutable
		{
			Vec2f submenuPos(1.0f, 1.0f);
			auto t = PopupCanvas->FromCanvasSpace(PopupCanvas->GetViewT().GetInverse() * PopupCanvas->ToCanvasSpace(DescriptionRoot->GetListElement(optionIndex).GetActorRef().GetTransform()->GetWorldTransform()));
			std::cout << "^!^ " << DescriptionRoot->GetListElement(optionIndex).GetActorRef().GetTransform()->GetWorldTransform().GetPos2D() << "\n";
			std::cout << "^^ " << t.GetPos2D() << "\n";
			submenuPos = t.GetPos2D() + t.GetScale2D();
			//submenuPos.y -= (optionIndex / (float)DescriptionRoot->GetListElementCount()) * 2.0f;
			//submenuPos.y += PopupOptionSizePx.y / 2.0f;
			//submenuPos = Vec2f(0.0f);
			//submenuPos = Vec2f(1.0f) / submenuPos;
			PopupDescription desc = dynamic_cast<Editor::EditorManager*>(gameHandle)->CreatePopupMenu(submenuPos, *window);
			prepareSubmenu(desc);
			desc.RefreshPopup();
		};
		auto& button = DescriptionRoot->CreateChild<UIButtonActor>("Button_" + buttonContent, buttonContent,
			[window = &Window, createSubPopup]() mutable
			{
				createSubPopup();
			}, Transform(Vec2f(0.0f), Vec2f(0.9f)));

		auto& visualCue = button.GetRoot()->CreateComponent<ModelComponent>("GEE_E_Submenu_Triangle", Transform(Vec2f(0.7f, 0.0f), Vec2f(0.1f, 0.8f)));
		visualCue.AddMeshInst(visualCue.GetScene().GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		//visualCue.OverrideInstancesMaterial(visualCue.GetScene().GetGameHandle()->GetRenderEngineHandle()->FindMaterial("GEE_E_Popup_Secondary").get());
		visualCue.OverrideInstancesMaterial(&IconData(*DescriptionRoot->GetGameHandle()->GetRenderEngineHandle(), "EditorAssets/more.png").GetMatInstance()->GetMaterialRef());
			
		if (optionalIconData.IsValid())
		{
			auto& iconModel = button.CreateComponent<ModelComponent>("GEE_E_Button_Icon", Transform(Vec2f(-0.7f, 0.0f), Vec2f(0.1f, 0.8f)));
			iconModel.AddMeshInst(MeshInstance(DescriptionRoot->GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD), optionalIconData.GetMatInstance()));
		}

		DescriptionRoot->Refresh();
		PopupCanvas->AutoClampView(true, true);
		return *this;
	}
	void PopupDescription::RefreshPopup()
	{
		ComputeView();
		glfwSetWindowSize(&Window, PopupOptionSizePx.x, static_cast<int>(static_cast<float>(PopupOptionSizePx.y) * DescriptionRoot->GetBoundingBoxIncludingChildren().Size.y));
	}
	void PopupDescription::ComputeView()
	{
		
	}
	IconData::IconData(RenderEngineManager& renderHandle, const std::string& texFilepath, Vec2i iconAtlasSize, float iconAtlasID):
		IconMaterial(nullptr),
		IconAtlasID(iconAtlasID)
	{
		if ((IconMaterial = renderHandle.FindMaterial("TEX:" + texFilepath)) == nullptr)
		{
			if (iconAtlasSize != Vec2i(0))
				IconMaterial = MakeShared<AtlasMaterial>("TEX:" + texFilepath, iconAtlasSize);
			else
				IconMaterial = MakeShared<Material>("TEX:" + texFilepath);

			IconMaterial->AddTexture(Texture::Loader<>::FromFile2D(texFilepath, Texture::Format::RGBA(), true, Texture::MinFilter::Bilinear()));
			IconMaterial->SetRenderShaderName("Forward_NoLight");
			renderHandle.AddMaterial(IconMaterial);
		}
	}
	bool IconData::IsValid() const
	{
		return IconMaterial != nullptr;
	}
	SharedPtr<MaterialInstance> IconData::GetMatInstance() const
	{
		GEE_CORE_ASSERT(IconMaterial);
		if (IconAtlasID >= 0.0f)
			if (auto materialCast = dynamic_cast<AtlasMaterial*>(IconMaterial.get()))
				return MakeShared<MaterialInstance>(*materialCast, materialCast->GetTextureIDInterpolatorTemplate(IconAtlasID));

		return MakeShared<MaterialInstance>(*IconMaterial);
	}
}