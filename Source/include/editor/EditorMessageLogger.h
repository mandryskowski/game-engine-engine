#pragma once
#include <scene/UIWindowActor.h>
#include <editor/EditorManager.h>

namespace GEE
{
	class EditorMessageLogger
	{
	public:
		enum class MessageType
		{
			Error,
			Warning, 
			Information,
			Success,
		};
		EditorMessageLogger(EditorManager&, GameScene& editorScene);

		void Log(const std::string& message, MessageType type = MessageType::Information);
	private:
		void AddMessageToSlot(UIWindowActor&);
		void KillMessage(UIWindowActor&);
		void UpdateQueuedMessagesText();
		EditorManager& EditorHandle;
		GameScene& EditorScene;
		AtlasMaterial* IconsMaterial;

		std::vector<UIWindowActor*> MessageSlots;
		std::queue<std::pair<std::string, MessageType>> QueuedMessages;
		TextComponent* QueuedMessagesText;
	};
}