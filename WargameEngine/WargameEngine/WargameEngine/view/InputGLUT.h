#pragma once
#include "IInput.h"
#include <memory>

class IObject;

class CInputGLUT : public IInput
{
public:
	CInputGLUT();
	virtual void DoOnLMBDown(std::function<bool(int, int) > const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnLMBUp(std::function<bool(int, int) > const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnRMBDown(std::function<bool(int, int) > const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnRMBUp(std::function<bool(int, int) > const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnMouseWheelUp(std::function<bool() > const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnMouseWheelDown(std::function<bool() > const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnKeyDown(std::function<bool(int key, int modifiers) > const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnKeyUp(std::function<bool(int key, int modifiers) > const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnCharacter(std::function<bool(unsigned int character) > const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnMouseMove(std::function<bool(int, int) > const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnGamepadButtonStateChange(std::function<bool(int gamepadIndex, int buttonIndex, bool newState)> const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void DoOnGamepadAxisChange(std::function<bool(int gamepadIndex, int axisIndex, double horizontal, double vertical)> const& handler, int priority = 0, std::string const& tag = "") override;
	virtual void EnableCursor(bool enable = true) override;
	virtual int GetModifiers() const override;
	virtual int GetMouseX() const override;
	virtual int GetMouseY() const override;
	virtual void DeleteAllSignalsByTag(std::string const& tag) override;
	virtual VirtualKey KeycodeToVirtualKey(int key) const override;

	static void OnSpecialKeyPress(int key, int x, int y);
	static void OnSpecialKeyRelease(int key, int x, int y);
	static void OnMouse(int button, int state, int x, int y);
	static void OnKeyboard(unsigned char key, int x, int y);
	static void OnKeyboardUp(unsigned char key, int x, int y);
	static void OnPassiveMouseMove(int x, int y);
	static void OnMouseMove(int x, int y);

	static void OnJoystick(unsigned int buttonMask, int x, int y, int z);
private:
	struct sSignals;
	static std::unique_ptr<sSignals> m_signals;
	static bool m_cursorEnabled;
	static unsigned int m_joystickButtons;
	static int m_joystickAxes[3];
};
