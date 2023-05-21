#pragma once
#include <utility/Utility.h>
#include <glfw/glfw3.h>
#include <queue>
#include <map>

namespace GEE
{
	class GameManager;

	enum class MouseButton
	{
		Left = GLFW_MOUSE_BUTTON_LEFT,
		Right = GLFW_MOUSE_BUTTON_RIGHT,
		Middle = GLFW_MOUSE_BUTTON_MIDDLE,

		Mouse1 = GLFW_MOUSE_BUTTON_1,
		Mouse2 = GLFW_MOUSE_BUTTON_2,
		Mouse3 = GLFW_MOUSE_BUTTON_3,
		Mouse4 = GLFW_MOUSE_BUTTON_4,
		Mouse5 = GLFW_MOUSE_BUTTON_5,
		Mouse6 = GLFW_MOUSE_BUTTON_6,
		Mouse7 = GLFW_MOUSE_BUTTON_7,
		Mouse8 = GLFW_MOUSE_BUTTON_8,

		Last = Mouse8
	};

	enum class Key
	{
		Unknown = GLFW_KEY_UNKNOWN,
		Space = GLFW_KEY_SPACE,
		Apostrophe = GLFW_KEY_APOSTROPHE, /* ' */
		Comma = GLFW_KEY_COMMA, /* , */
		Minus = GLFW_KEY_MINUS, /* - */
		Period = GLFW_KEY_PERIOD, /* . */
		Slash = GLFW_KEY_SLASH, /* / */
		Zero = GLFW_KEY_0,
		One = GLFW_KEY_1,
		Two = GLFW_KEY_2,
		Three = GLFW_KEY_3,
		Four = GLFW_KEY_4,
		Five = GLFW_KEY_5,
		Six = GLFW_KEY_6,
		Seven = GLFW_KEY_7,
		Eight = GLFW_KEY_8,
		Nine = GLFW_KEY_9,
		Semicolon = GLFW_KEY_SEMICOLON, /* ; */
		Equal = GLFW_KEY_EQUAL, /* = */
		A = GLFW_KEY_A,
		B = GLFW_KEY_B,
		C = GLFW_KEY_C,
		D = GLFW_KEY_D,
		E = GLFW_KEY_E,
		F = GLFW_KEY_F,
		G = GLFW_KEY_G,
		H = GLFW_KEY_H,
		I = GLFW_KEY_I,
		J = GLFW_KEY_J,
		K = GLFW_KEY_K,
		L = GLFW_KEY_L,
		M = GLFW_KEY_M,
		N = GLFW_KEY_N,
		O = GLFW_KEY_O,
		P = GLFW_KEY_P,
		Q = GLFW_KEY_Q,
		R = GLFW_KEY_R,
		S = GLFW_KEY_S,
		CompType = GLFW_KEY_T,
		U = GLFW_KEY_U,
		V = GLFW_KEY_V,
		W = GLFW_KEY_W,
		X = GLFW_KEY_X,
		Y = GLFW_KEY_Y,
		Z = GLFW_KEY_Z,
		LeftBracket = GLFW_KEY_LEFT_BRACKET, /* [ */
		Backslash = GLFW_KEY_BACKSLASH, /* \ */
		RightBracket = GLFW_KEY_RIGHT_BRACKET, /* ] */
		GraveAccent = GLFW_KEY_GRAVE_ACCENT, /* ` */
		World1 = GLFW_KEY_WORLD_1, /* non-US #1 */
		World2 = GLFW_KEY_WORLD_2, /* non-US #2 */
		Escape = GLFW_KEY_ESCAPE,
		Enter = GLFW_KEY_ENTER,
		Tab = GLFW_KEY_TAB,
		Backspace = GLFW_KEY_BACKSPACE,
		Insert = GLFW_KEY_INSERT,
		Delete = GLFW_KEY_DELETE,
		RightArrow = GLFW_KEY_RIGHT,
		LeftArrow = GLFW_KEY_LEFT,
		DownArrow = GLFW_KEY_DOWN,
		UpArrow = GLFW_KEY_UP,
		PageUp = GLFW_KEY_PAGE_UP,
		PageDown = GLFW_KEY_PAGE_DOWN,
		Home = GLFW_KEY_HOME,
		End = GLFW_KEY_END,
		CapsLock = GLFW_KEY_CAPS_LOCK,
		ScrollLock = GLFW_KEY_SCROLL_LOCK,
		NumLock = GLFW_KEY_NUM_LOCK,
		PrintScreen = GLFW_KEY_PRINT_SCREEN,
		Pause = GLFW_KEY_PAUSE,
		F1 = GLFW_KEY_F1,
		F2 = GLFW_KEY_F2,
		F3 = GLFW_KEY_F3,
		F4 = GLFW_KEY_F4,
		F5 = GLFW_KEY_F5,
		F6 = GLFW_KEY_F6,
		F7 = GLFW_KEY_F7,
		F8 = GLFW_KEY_F8,
		F9 = GLFW_KEY_F9,
		F10 = GLFW_KEY_F10,
		F11 = GLFW_KEY_F11,
		F12 = GLFW_KEY_F12,
		F13 = GLFW_KEY_F13,
		F14 = GLFW_KEY_F14,
		F15 = GLFW_KEY_F15,
		F16 = GLFW_KEY_F16,
		F17 = GLFW_KEY_F17,
		F18 = GLFW_KEY_F18,
		F19 = GLFW_KEY_F19,
		F20 = GLFW_KEY_F20,
		F21 = GLFW_KEY_F21,
		F22 = GLFW_KEY_F22,
		F23 = GLFW_KEY_F23,
		F24 = GLFW_KEY_F24,
		F25 = GLFW_KEY_F25,
		Keypad0 = GLFW_KEY_KP_0,
		Keypad1 = GLFW_KEY_KP_1,
		Keypad2 = GLFW_KEY_KP_2,
		Keypad3 = GLFW_KEY_KP_3,
		Keypad4 = GLFW_KEY_KP_4,
		Keypad5 = GLFW_KEY_KP_5,
		Keypad6 = GLFW_KEY_KP_6,
		Keypad7 = GLFW_KEY_KP_7,
		Keypad8 = GLFW_KEY_KP_8,
		Keypad9 = GLFW_KEY_KP_9,
		KeypadDecimal = GLFW_KEY_KP_DECIMAL,
		KeypadDivide = GLFW_KEY_KP_DIVIDE,
		KeypadMultiply = GLFW_KEY_KP_MULTIPLY,
		KeypadSubtract= GLFW_KEY_KP_SUBTRACT,
		KeypadAdd = GLFW_KEY_KP_ADD,
		KeypadEnter= GLFW_KEY_KP_ENTER,
		KeypadEqual= GLFW_KEY_KP_EQUAL,
		LeftShift = GLFW_KEY_LEFT_SHIFT,
		LeftControl = GLFW_KEY_LEFT_CONTROL,
		LeftAlt = GLFW_KEY_LEFT_ALT,
		LeftSuper = GLFW_KEY_LEFT_SUPER,
		RightShift = GLFW_KEY_RIGHT_SHIFT,
		RightControl = GLFW_KEY_RIGHT_CONTROL,
		RightAlt = GLFW_KEY_RIGHT_ALT,

		RightSuper = GLFW_KEY_RIGHT_SUPER,
		Menu = GLFW_KEY_MENU,
		Last = Menu
	};

	enum KeyModifierFlags
	{
		Shift = GLFW_MOD_SHIFT,
		Control = GLFW_MOD_CONTROL,
		Alt = GLFW_MOD_ALT,
		Super = GLFW_MOD_SUPER,
		CapsLock = GLFW_MOD_CAPS_LOCK,
		NumLock = GLFW_MOD_NUM_LOCK
	};

	enum class EventType
	{
		WindowClosed,
		WindowResized,
		KeyPressed,
		KeyRepeated,
		KeyReleased,
		CharacterEntered,
		MouseMoved,
		MouseScrolled,
		MousePressed,
		MouseReleased,
		FocusSwitched,

		CanvasViewChanged
	};


	class Event
	{
	public:
		/**
		 * @param type - an enum indicating the type of this Event. Should match the class of this Event.
		 * @param eventRoot - an optional Actor that will be the first to receive this Event, if it is in the active Scene. If it's not, or if eventRoot is nullptr, active scene's root will be used.
		*/
		Event(Actor* eventRoot = nullptr);
		virtual EventType GetType() const = 0;
		Actor* GetEventRoot() { return EventRoot; }
		const Actor* GetEventRoot() const { return EventRoot; }

		virtual ~Event() {}
	private:
		
		// The actor which will be used as the first recipient of this Event, and which will pass this Event to its children. Can be used for optimisation.
		Actor* EventRoot;
	};



	class CursorMoveEvent : public Event
	{
	public:
		CursorMoveEvent(Vec2f newPositionPx, Vec2u windowSizePx, Actor* eventRoot = nullptr);
		CursorMoveEvent(Vec2f newPositionNDC, Actor* eventRoot = nullptr);
		Vec2f GetNewPositionNDC() const;
		EventType GetType() const override { return EventType::MouseMoved; }

	private:
		Vec2f NewPositionNDC;
	};

	class MouseButtonEvent : public Event
	{
	public:
		MouseButtonEvent(MouseButton, int modifierBits, bool released = false, Actor* eventRoot = nullptr);
		MouseButton GetButton() const;
		int GetModifierBits() const;
		EventType GetType() const override { return bReleased ? EventType::MouseReleased : EventType::MousePressed; }

	private:
		const MouseButton Button;
		const int ModifierBits;
		const bool bReleased; // If true, then mouse button was released. Otherwise, it was pressed.
	};

	class MouseScrollEvent : public Event
	{
	public:
		MouseScrollEvent(Vec2f offset, Actor* eventRoot = nullptr);
		Vec2f GetOffset() const;
		EventType GetType() const override { return EventType::MouseScrolled; }

	private:
		Vec2f Offset;
	};

	class KeyEvent : public Event
	{
	public:
		enum KeyAction
		{
			Pressed,
			Released,
			Repeated
		};

		KeyEvent(Key, int modifierFlags, KeyAction action, Actor* eventRoot = nullptr);
		Key GetKeyCode() const;
		int GetModifierBits() const;
		EventType GetType() const override
		{
			switch (Action)
			{
			case Pressed: return EventType::KeyPressed;
			case Released: return EventType::KeyReleased;
			case Repeated: return EventType::KeyRepeated;
			}
		}

	private:
		Key KeyCode;
		KeyAction Action;
		int ModifierBits;
	};

	class CharEnteredEvent : public Event
	{
	public:
		CharEnteredEvent(unsigned int unicode, Actor* eventRoot = nullptr);
		std::string GetUTF8() const;
		EventType GetType() const override { return EventType::CharacterEntered; }

	private:
		unsigned int Unicode;
	};

	class WindowClosedEvent : public Event
	{
	public:
		using Event::Event;
		EventType GetType() const override { return EventType::WindowClosed; }
	};

	class WindowResizedEvent : public Event
	{
	public:
		using Event::Event;
		EventType GetType() const override { return EventType::WindowResized; }
	};

	class FocusSwitchedEvent : public Event
	{
	public:
		using Event::Event;
		EventType GetType() const override { return EventType::FocusSwitched; }
	};

	class CanvasViewChangedEvent : public Event
	{
	public:
		CanvasViewChangedEvent(Actor* eventRoot = nullptr, bool clampViewToElements = true) : ClampViewToElements(clampViewToElements), Event(eventRoot) {};
		bool ClampsViewToElements() const { return ClampViewToElements; }
		EventType GetType() const override { return EventType::CanvasViewChanged; }

	private:
		bool ClampViewToElements;
	};

	/**
	 * @brief General class for holding all events in a Game.
	*/
	class EventHolder
	{
	public:
		SharedPtr<Event> PollEvent(GameScene&);
		template <typename EventClass>
		void PushEvent(GameScene&, const EventClass& _event);
		template <typename EventClass>
		void PushEventInActiveScene(GameManager&, const EventClass& _event);

	private:
		std::map<GameScene*, std::queue<SharedPtr<Event>>> Events;
	};

	/**
	 * @brief Class for pushing Events by clients of the engine.
	*/
	class EventPusher
	{
	public:
		template <typename EventClass>
		void PushEvent(const EventClass& _event)
		{
			_EventHolder.PushEvent<EventClass>(Scene, _event);
		}
	private:
		EventPusher(GameScene& scene, EventHolder& evHolder) : Scene(scene), _EventHolder(evHolder) {}

		friend class Game;

		GameScene& Scene;
		EventHolder& _EventHolder;
	};

	template<typename EventClass>
	inline void EventHolder::PushEvent(GameScene& scene, const EventClass& _event)
	{
		Events[&scene].push(MakeShared<EventClass>(_event));
	}
	template<typename EventClass>
	inline void EventHolder::PushEventInActiveScene(GameManager& gameHandle, const EventClass& _event)
	{
		GEE_CORE_ASSERT(gameHandle.GetActiveScene());
		PushEvent<EventClass>(*gameHandle.GetActiveScene(), _event);
	}
}