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

	CursorMoveEvent::CursorMoveEvent(EventType type, Vec2f newPositionPx, Vec2u windowSizePx) :
		CursorMoveEvent(type, Vec2f(newPositionPx.x, windowSizePx.y - newPositionPx.y) / static_cast<Vec2f>(windowSizePx) * 2.0f - 1.0f)
	{

	}

	CursorMoveEvent::CursorMoveEvent(EventType type, Vec2f newPositionNDC) :
		Event(type), NewPositionNDC(newPositionNDC)
	{
	}

	Vec2f CursorMoveEvent::GetNewPositionNDC() const
	{
		return NewPositionNDC;
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

	SharedPtr<Event> EventHolder::PollEvent(SystemWindow& window)
	{
		if (Events[&window].empty())
			return nullptr;

		auto polledEvent = Events[&window].front();
		Events[&window].pop();

		return polledEvent;
	}
}
