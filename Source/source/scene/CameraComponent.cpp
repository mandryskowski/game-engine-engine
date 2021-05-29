#include <scene/CameraComponent.h>
#include <rendering/RenderInfo.h>
#include <rendering/Material.h>

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIInputBoxActor.h>

namespace GEE
{
	CameraComponent::CameraComponent(Actor& actor, Component* parentComp, std::string name, const glm::mat4& projectionMatrix) :
		Component(actor, parentComp, name, Transform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f))),
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

	glm::mat4 CameraComponent::GetProjectionMat()
	{
		return Projection;
	}

	RenderInfo CameraComponent::GetRenderInfo(RenderToolboxCollection& renderCollection)
	{
		return RenderInfo(renderCollection, GetTransform().GetWorldTransform().GetViewMatrix(), GetProjectionMat(), glm::mat4(1.0f), GetTransform().GetWorldTransform().PositionRef);
	}

	void CameraComponent::RotateWithMouse(glm::vec2 mouseOffset)
	{
		float sensitivity = 0.15f;
		RotationEuler.x -= mouseOffset.y * sensitivity;
		RotationEuler.y -= mouseOffset.x * sensitivity;

		RotationEuler.x = glm::clamp(RotationEuler.x, -89.9f, 89.9f);
		RotationEuler.y = fmod(RotationEuler.y, 360.0f);

		ComponentTransform.SetRotation(RotationEuler);

		/*
		std::vector <CollisionComponent*> collisionChildren;
		GetAllComponents<CollisionComponent, BBox>(&collisionChildren);

		glm::vec3 minSize(0.1f);
		for (unsigned int i = 0; i < collisionChildren.size(); i++)
		{
			glm::vec3 childSize = collisionChildren[i]->GetTransform()->ScaleRef;

			if (i == 0)
			{
				minSize = childSize;
				continue;
			}

			if (length2(childSize) < length2(minSize))
				minSize = childSize;
		}


		std::vector <glm::vec3> bounceNormals;
		CollisionEng->CheckForCollision(collisionChildren, &bounceNormals);
		std::vector <glm::vec3> bNormals;

		while (CollisionEng->CheckForCollision(collisionChildren, &bNormals))
		{
			if ((bNormals.empty()) || (bNormals != bounceNormals))	//jesli wektor normalnych zmieni sie w trakcie przemieszczania, to nasz komponent utknal - nie da sie go przemiescic w zaden realistyczny sposob
				break;
			bounceNormals = bNormals;

			for (unsigned int i = 0; i < bounceNormals.size(); i++)
			{
				glm::mat3 inverseRotMat = ComponentTransform.GetParentTransform()->GetWorldTransform().GetRotationMatrix(-1.0f);
				bounceNormals[i] = inverseRotMat * bounceNormals[i];
				ComponentTransform.Move(bounceNormals[i] * minSize * glm::vec3(0.01f)); //mnozymy normalna przez czesc rozmiaru obiektu kolizji, aby odsunac go od sciany
			}
		}
		*/
	}

	void CameraComponent::Update(float deltaTime)
	{
		Component::Update(deltaTime);
	}

	MaterialInstance CameraComponent::GetDebugMatInst(EditorIconState state)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_CameraComponent", "EditorAssets/cameracomponent_icon.png");
		return Component::GetDebugMatInst(state);
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