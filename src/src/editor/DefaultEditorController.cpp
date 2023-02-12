#include "DefaultEditorController.h"
#include <game/GameScene.h>
#include <scene/CameraComponent.h>
#include <editor/EditorActions.h>

namespace GEE
{
	namespace Editor
	{
		DefaultEditorController::DefaultEditorController(GameScene& scene, Actor* parentActor, const std::string& name):
			Controller(scene, parentActor, name),
			State(ControllerState::Default),
			AxisModeBits(AxisBits::All)
		{
		}
		void DefaultEditorController::HandleEvent(const Event& ev)
		{
			if (ev.GetType() == EventType::KeyReleased)
			{
				auto keyEv = dynamic_cast<const KeyEvent*>(&ev);
				bool shiftPressed = keyEv->GetModifierBits() & KeyModifierFlags::Shift;

				auto possessed = dynamic_cast<EditorManager*>(GameHandle)->GetActions().GetSelectedComponent();

				switch (keyEv->GetKeyCode())
				{
					case Key::G: State = ControllerState::Grab; break;
					case Key::R: State = ControllerState::Rotate; break;
					case Key::S: State = ControllerState::Resize; break;

					case Key::X: AxisModeBits = (shiftPressed) ? (AxisBits::All - AxisBits::X) : (AxisBits::X); break;
					case Key::Y: AxisModeBits = (shiftPressed) ? (AxisBits::All - AxisBits::Y) : (AxisBits::Y); break;
					case Key::Z: AxisModeBits = (shiftPressed) ? (AxisBits::All - AxisBits::Z) : (AxisBits::Z); break;

					case Key::Escape:
						possessed->SetTransform(TransformBeforeAction);
						State = ControllerState::Default;
						AxisModeBits = AxisBits::All; break;
				}
			}
			else if (ev.GetType() == EventType::MousePressed)
			{
				auto mouseEv = dynamic_cast<const MouseButtonEvent*>(&ev);

				if (mouseEv->GetButton() == MouseButton::Left)
					State = ControllerState::Default;
			}
			
		}
		void DefaultEditorController::OnMouseMovement(const Vec2f& previousPosPx, const Vec2f& currentPosPx, const Vec2u& windowSize)
		{
			Vec2f translationVec((currentPosPx - previousPosPx) * 0.01f);
			Vec3f rightVector = glm::cross(GameHandle->GetMainScene()->GetActiveCamera()->GetTransform().GetWorldTransform().GetFrontVec(), Vec3f(0.0f, 1.0f, 0.0f));
			Vec3f upVector = glm::cross(glm::normalize(rightVector), GameHandle->GetMainScene()->GetActiveCamera()->GetTransform().GetWorldTransform().GetFrontVec());

			auto possessed = dynamic_cast<EditorManager*>(GameHandle)->GetActions().GetSelectedComponent();

			if (!(AxisModeBits & AxisBits::X))
				rightVector.x = upVector.x = 0.0f;
			if (!(AxisModeBits & AxisBits::Y))
				rightVector.y = upVector.y = 0.0f;
			if (!(AxisModeBits & AxisBits::Z))
				rightVector.z = upVector.z = 0.0f;

			if (rightVector != Vec3f(0.0f)) rightVector = glm::normalize(rightVector);
			if (upVector != Vec3f(0.0f)) upVector = glm::normalize(upVector);
			
			if (!possessed)
				return;

			switch (State)
			{
				case ControllerState::Grab:
				{
					auto v = (rightVector * translationVec.x + upVector * translationVec.y) * 0.4f;
					std::cout << "% Translation: " << v << '\n';
					possessed->GetTransform().Move(v);
					break;
				}
				case ControllerState::Resize:
				{
					auto vec = (rightVector * translationVec.x + upVector * translationVec.y) * 0.4f;
					std::cout << "% Scale: " << vec + Vec3f(1.0f) << '\n';
					possessed->GetTransform().ApplyScale((vec + Vec3f(1.0f)));
					break;
				}
				case ControllerState::Rotate:
				{
					auto vect = (rightVector * translationVec.x + upVector * translationVec.y) * 0.4f;
					possessed->GetTransform().Rotate(vect * 100.0f);
					break;
				}
			}
		}
		void DefaultEditorController::Reset(const Transform& t)
		{
			State = ControllerState::Default;
			AxisModeBits = AxisBits::All;
			TransformBeforeAction = t;
		}
	}

}