#include <input/Event.h>

namespace GEE
{
	Event::Event(EventType type, Actor* eventRoot) :
		Type(type),
		EventRoot(eventRoot)
	{
	}

	EventType Event::GetType() const
	{
		return Type;
	}

	CursorMoveEvent::CursorMoveEvent(EventType type, Vec2f newPositionPx, Vec2u windowSizePx, Actor* eventRoot) :
		CursorMoveEvent(type, Vec2f(newPositionPx.x, windowSizePx.y - newPositionPx.y) / static_cast<Vec2f>(windowSizePx) * 2.0f - 1.0f, eventRoot)
	{

	}

	CursorMoveEvent::CursorMoveEvent(EventType type, Vec2f newPositionNDC, Actor* eventRoot) :
		Event(type, eventRoot),
		NewPositionNDC(newPositionNDC)
	{
	}

	Vec2f CursorMoveEvent::GetNewPositionNDC() const
	{
		return NewPositionNDC;
	}

	MouseButtonEvent::MouseButtonEvent(EventType type, MouseButton button, int modifierBits, Actor* eventRoot) :
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

	MouseScrollEvent::MouseScrollEvent(EventType type, Vec2f offset, Actor* eventRoot) :
		Event(type),
		Offset(offset)
	{
	}

	Vec2f MouseScrollEvent::GetOffset() const
	{
		return Offset;
	}

	KeyEvent::KeyEvent(EventType type, Key keyCode, int modifierBits, Actor* eventRoot) :
		Event(type, eventRoot),
		KeyCode(keyCode),
		ModifierBits(modifierBits)
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

	CharEnteredEvent::CharEnteredEvent(EventType type, unsigned int unicode, Actor* eventRoot) :
		Event(type, eventRoot),
		Unicode(unicode)
	{
	}

	std::string CharEnteredEvent::GetUTF8() const
	{
		return std::string(1, static_cast<unsigned char>(Unicode));
	}

	SharedPtr<Event> EventHolder::PollEvent(GameScene& scene)
	{
		if (Events[&scene].empty())
			return nullptr;

		auto polledEvent = Events[&scene].front();
		Events[&scene].pop();

		return polledEvent;
	}
}
