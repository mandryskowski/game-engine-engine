#include <scene/CameraComponent.h>
#include <game/GameScene.h>
#include <rendering/RenderInfo.h>
#include <rendering/Material.h>

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIInputBoxActor.h>

namespace GEE
{
	CameraComponent::CameraComponent(Actor& actor, Component* parentComp, std::string name, const Mat4f& projectionMatrix) :
		Component(actor, parentComp, name, Transform(Vec3f(0.0f), Vec3f(0.0f), Vec3f(1.0f))),
		Projection(projectionMatrix)
	{
	}

	CameraComponent::CameraComponent(CameraComponent&& comp) :
		Component(std::move(comp)),
		Projection(comp.Projection)
	{
	}

	Mat4f CameraComponent::GetViewMat()
	{
		return GetTransform().GetWorldTransform().GetViewMatrix();
	}

	Mat4f CameraComponent::GetProjectionMat()
	{
		return Projection;
	}

	SceneMatrixInfo CameraComponent::GetRenderInfo(RenderingContextID contextID, RenderToolboxCollection& renderCollection)
	{
		return SceneMatrixInfo(contextID, renderCollection, *Scene.GetRenderData(), GetViewMat(), GetProjectionMat(), GetTransform().GetWorldTransform().GetPos());
	}

	void CameraComponent::Update(float deltaTime)
	{
		Component::Update(deltaTime);
	}

	MaterialInstance CameraComponent::GetDebugMatInst(ButtonMaterialType type)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_CameraComponent", "Assets/Editor/cameracomponent_icon.png");
		return Component::GetDebugMatInst(type);
	}

	void CameraComponent::GetEditorDescription(ComponentDescriptionBuilder descBuilder)
	{
		Component::GetEditorDescription(descBuilder);

		UIInputBoxActor& fovInputBox = descBuilder.AddField("Field of view").CreateChild<UIInputBoxActor>("FovButton");
		fovInputBox.SetOnInputFunc([this, descBuilder](float fov) mutable { Vec2f viewportSize = static_cast<Vec2f>(GameHandle->GetGameSettings()->Video.Resolution); Projection = glm::perspective(glm::radians(fov), viewportSize.x / viewportSize.y, 0.01f, 100.0f); }, []() { return 0.0f; });

		UIInputBoxActor& renderDistBox = descBuilder.AddField("Render distance").CreateChild<UIInputBoxActor>("RenderDistanceButton");
		renderDistBox.SetOnInputFunc([this, descBuilder](float dist) mutable { Vec2f viewportSize = static_cast<Vec2f>(GameHandle->GetGameSettings()->Video.Resolution); Projection = glm::perspective(glm::radians(90.0f), viewportSize.x / viewportSize.y, 0.01f, dist); }, []() { return 100.0f; });

		descBuilder.AddField("Active").GetTemplates().TickBox([this](bool val) {
			if (val) {
				Scene.BindActiveCamera(this);  GameHandle->BindAudioListenerTransformPtr(&GetTransform());
			}
			else {
				Scene.BindActiveCamera(nullptr); GameHandle->BindAudioListenerTransformPtr(nullptr);
			}
			}, [this]() {return this == Scene.GetActiveCamera(); });
	}

	CameraComponent::~CameraComponent()
	{
		if (Scene.GetActiveCamera() == this)
		{
			Scene.BindActiveCamera(nullptr);
			GameHandle->UnbindAudioListenerTransformPtr(nullptr);	//TODO: Fix: deleting editor's camera will unbind listenertransformptr for the whole game.
		}
	}

}