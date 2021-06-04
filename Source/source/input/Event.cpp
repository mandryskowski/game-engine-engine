#include <input/Event.h>

namespace GEE
{
	Event::Event(EventType type) :
		Type(type)
	{
	}

	EventType Event::GetType() const
	{
		return Type;
	}

	CursorMoveEvent::CursorMoveEvent(EventType type, Vec2u newPosition) :
		Event(type), NewPosition(newPosition)
	{
	}

	Vec2u CursorMoveEvent::GetNewPosition() const
	{
		return NewPosition;
	}

	MouseButtonEvent::MouseButtonEvent(EventType type, MouseButton button, int modifierBits) :
		Event(type), Button(button), ModifierBits(modifierBits)
	{
	}

	MouseButton MouseButtonEvent::GetButton() const
	{
		return Button;
	}

	int MouseButtonEvent::GetModifierBits() const
	{
		return ModifierBits;
	}

	MouseScrollEvent::MouseScrollEvent(EventType type, Vec2f offset) :
		Event(type),
		Offset(offset)
	{
	}

	Vec2f MouseScrollEvent::GetOffset() const
	{
		return Offset;
	}

	KeyEvent::KeyEvent(EventType type, Key keyCode, int modifierBits) :
		Event(type), KeyCode(keyCode), ModifierBits(modifierBits)
	{
	}

	Key KeyEvent::GetKeyCode() const
	{
		return KeyCode;
	}

	int KeyEvent::GetModifierBits() const
	{
		return ModifierBits;
	}

	CharEnteredEvent::CharEnteredEvent(EventType type, unsigned int unicode) :
		Event(type),
		Unicode(unicode)
	{
	}

	std::string CharEnteredEvent::GetUTF8() const
	{
		return std::string(1, static_cast<unsigned char>(Unicode));
	}

	std::shared_ptr<Event> EventHolder::PollEvent()
	{
		if (Events.empty())
			return nullptr;

		auto polledEvent = Events.front();
		Events.pop();

		return polledEvent;
	}
}