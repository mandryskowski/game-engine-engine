#include <scene/CameraComponent.h>
#include <rendering/RenderInfo.h>
#include <rendering/Material.h>

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIInputBoxActor.h>

namespace GEE
{
	CameraComponent::CameraComponent(Actor& actor, Component* parentComp, std::string name, const Mat4f& projectionMatrix) :
		Component(actor, parentComp, name, Transform(Vec3f(0.0f), Vec3f(0.0f), Vec3f(1.0f))),
		RotationEuler(0.0f),
		Projection(projectionMatrix)
	{
	}

	CameraComponent::CameraComponent(CameraComponent&& comp) :
		Component(std::move(comp)),
		RotationEuler(comp.RotationEuler),
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

	RenderInfo CameraComponent::GetRenderInfo(RenderToolboxCollection& renderCollection)
	{
		return RenderInfo(renderCollection, GetViewMat(), GetProjectionMat(), Mat4f(1.0f), GetTransform().GetWorldTransform().GetPos());
	}

	void CameraComponent::Update(float deltaTime)
	{
		Component::Update(deltaTime);
	}

	MaterialInstance CameraComponent::LoadDebugMatInst(EditorIconState state)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_CameraComponent", "EditorAssets/cameracomponent_icon.png");
		return Component::LoadDebugMatInst(state);
	}

	void CameraComponent::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Component::GetEditorDescription(descBuilder);

		UIInputBoxActor& fovInputBox = descBuilder.AddField("Field of view").CreateChild<UIInputBoxActor>("FovButton");
		fovInputBox.SetOnInputFunc([this](float fov) { Projection = glm::perspective(glm::radians(fov), 1.0f, 0.01f, 100.0f); }, []() { return 0.0f; });

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