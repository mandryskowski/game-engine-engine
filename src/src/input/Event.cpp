#include <input/Event.h>

namespace GEE
{
	Event::Event(Actor* eventRoot) :
		EventRoot(eventRoot)
	{
	}

	CursorMoveEvent::CursorMoveEvent(Vec2f newPositionPx, Vec2u windowSizePx, Actor* eventRoot) :
		CursorMoveEvent(newPositionPx / static_cast<Vec2f>(windowSizePx) * 2.0f - 1.0f, eventRoot)
	{

	}

	CursorMoveEvent::CursorMoveEvent(Vec2f newPositionNDC, Actor* eventRoot) :
		Event(eventRoot),
		NewPositionNDC(newPositionNDC)
	{
	}

	Vec2f CursorMoveEvent::GetNewPositionNDC() const
	{
		return NewPositionNDC;
	}

	MouseButtonEvent::MouseButtonEvent(MouseButton button, int modifierBits, bool released, Actor* eventRoot) :
		Button(button),
		ModifierBits(modifierBits),
		bReleased(released)
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

	MouseScrollEvent::MouseScrollEvent(Vec2f offset, Actor* eventRoot) :
		Offset(offset)
	{
	}

	Vec2f MouseScrollEvent::GetOffset() const
	{
		return Offset;
	}

	KeyEvent::KeyEvent(Key keyCode, int modifierBits, KeyAction action, Actor* eventRoot) :
		Event(eventRoot),
		KeyCode(keyCode),
		Action(action),
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

	CharEnteredEvent::CharEnteredEvent(unsigned int unicode, Actor* eventRoot) :
		Event(eventRoot),
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
