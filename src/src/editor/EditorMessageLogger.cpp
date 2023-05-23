#include <editor/EditorMessageLogger.h>
#include <game/GameScene.h>
#include <scene/TextComponent.h>
#include <scene/ModelComponent.h>
#include <utility/Log.h>

#include "EditorManager.h"
#include "rendering/RenderEngineManager.h"
#include "scene/UIWindowActor.h"
#include "UI/UIActor.h"

namespace GEE
{
	namespace Editor
	{

		EditorMessageLogger::EditorMessageLogger(EditorManager& editorHandle, GameScene& editorScene) :
			EditorHandle(editorHandle),
			EditorScene(editorScene), 
			IconsMaterial(nullptr),
			MessageSlots(4, nullptr),
			QueuedMessagesText(nullptr)
		{
			IconsMaterial = EditorHandle.GetGameHandle()->GetRenderEngineHandle()->FindMaterial<AtlasMaterial>("GEE_E_Icons");
			
			auto& moreMessagesActor = EditorScene.CreateActorAtRoot<UIActorDefault>("GEE_E_More_Messages_Actor", Transform(Vec2f(0.8f, -0.9f + static_cast<float>(MessageSlots.size()) * 0.2f - 0.1f), Vec2f(0.1f, 0.01f)));
			auto& moreMessagesText = moreMessagesActor.CreateComponent<TextComponent>("GEE_E_More_Messages_Text", Transform(), "0 more...", "", Alignment2D::Bottom());
			moreMessagesText.SetMaxSize(Vec2f(1.0f));

			moreMessagesText.Unstretch();
			QueuedMessagesText = &moreMessagesText;
			QueuedMessagesText->SetHide(true);
		}
		void EditorMessageLogger::Log(const std::string& message, MessageType type)
		{
			GEE_LOG(message);
			if (MessageSlots.back())
			{
				QueuedMessages.push(std::pair<std::string, MessageType>(message, type));
				UpdateQueuedMessagesText();
				return;
			}

			auto& logWindow = EditorScene.CreateActorAtRoot<UIWindowActor>("GEE_E_LogWindow", Transform(Vec2f(0.8f, -0.9f), Vec2f(0.2f, 0.1f)));
			logWindow.CreateCanvasBackgroundModel(Vec3f(0.3f, 0.0f, 0.25f));
			logWindow.SetViewScale(Vec2f(2.0f, 1.0f));
			logWindow.KillDragButton();
			logWindow.KillResizeBars();
			logWindow.KillScrollBars();

			AddMessageToSlot(logWindow);

			Interpolation interp(0.0f, 2.0f, InterpolationType::Linear);
			interp.SetOnUpdateFunc([&](double T) { if (T == 1.0) KillMessage(logWindow); return T == 1.0; });
			EditorHandle.GetGameHandle()->AddInterpolation(interp);



			auto closeButton = logWindow.GetActor("GEE_E_Close_Button");
			closeButton->GetTransform()->SetPosition(Vec2f(1.0f) - static_cast<Vec2f>(closeButton->GetTransform()->GetScale()));

			auto& content = logWindow.CreateChild<UIActorDefault>("GEE_E_LogWindowContent");
			auto& messageText = content.CreateComponent<TextComponent>("GEE_E_LogText", Transform(Vec2f(-0.8f, 0.0f), Vec2f(1.6f, 2.0f)), message, "", Alignment2D::LeftCenter());// .Unstretch();
			messageText.SetMaxSize(Vec2f(1.0f));
			messageText.Unstretch(UISpace::Local);
	

			auto& iconModel = content.CreateComponent<ModelComponent>("GEE_E_LogIcon", Transform(Vec2f(-1.4f, 0.0f), Vec2f(0.5f)));
			iconModel.AddMeshInst(EditorHandle.GetGameHandle()->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Quad));
			iconModel.OverrideInstancesMaterialInstances(MakeShared<MaterialInstance>(IconsMaterial, IconsMaterial->GetTextureIDInterpolatorTemplate(5.0f + static_cast<float>(type))));
		}
		void EditorMessageLogger::AddMessageToSlot(UIWindowActor& message)
		{
			for (int i = 0; i < static_cast<int>(MessageSlots.size()); i++)
			{
				if (!MessageSlots[i])
				{
					MessageSlots[i] = &message;
					message.GetTransform()->SetPosition(Vec2f(0.8f, 0.2f * static_cast<float>(i) - 0.9f));
					break;
				}
			}
		}
		void EditorMessageLogger::KillMessage(UIWindowActor& message)
		{
			std::vector<UIWindowActor*>::iterator slot;
			for (slot = MessageSlots.begin(); slot != MessageSlots.end(); slot++)
			{
				if (&message == *slot)
				{
					(*slot) = nullptr;
					message.MarkAsKilled();
					break;
				}
			}

			if (slot != MessageSlots.end())
			{
				for (; slot != MessageSlots.end() - 1; slot++)
				{
					*slot = *(slot + 1);
					if (*slot)
						(*slot)->GetTransform()->Move(Vec2f(0.0f, -0.2f));
				}

				MessageSlots.back() = nullptr;
			}

			if (!QueuedMessages.empty())
			{
				Log(QueuedMessages.front().first, QueuedMessages.front().second);
				QueuedMessages.pop();
				UpdateQueuedMessagesText();
			}
		}
		void EditorMessageLogger::UpdateQueuedMessagesText()
		{
			if (!QueuedMessagesText)
				return;

			if (QueuedMessages.empty())
			{
				QueuedMessagesText->SetHide(true);
				return;
			}

			QueuedMessagesText->SetHide(false);
			QueuedMessagesText->SetContent(ToStringPrecision(QueuedMessages.size(), 0) + " more...");
		}
	}
}
