#pragma once
#include <queue>
#include <utility/Utility.h>

namespace GEE
{
	class AtlasMaterial;
	class UIWindowActor;
	class TextComponent;
}

namespace GEE
{
	namespace Editor
	{
		class EditorManager;

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
			SharedPtr<AtlasMaterial> IconsMaterial;

			std::vector<UIWindowActor*> MessageSlots;
			std::queue<std::pair<std::string, MessageType>> QueuedMessages;
			TextComponent* QueuedMessagesText;
		};
	}
}
