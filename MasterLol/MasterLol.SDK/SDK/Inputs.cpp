#include "stdafx.h"
#include "Inputs.h"
#include "D3DHooks.h"
#include "Config.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"

InputSys::InputSys()
	: m_hTargetWindow(nullptr), m_ulOldWndProc(0)
{
}

InputSys::~InputSys()
{
	if (m_ulOldWndProc)
		SetWindowLongPtr(m_hTargetWindow, GWLP_WNDPROC, m_ulOldWndProc);
	m_ulOldWndProc = 0;
}

void InputSys::Initialize()
{
	LPDIRECT3DDEVICE9 device = D3D::D3DHooks::Get().GetDevice();
	for (int i = 0; i < 20; ++i)
	{
		Sleep(100);
		device = D3D::D3DHooks::Get().GetDevice();
		if (device)
			break;
	}
	if(!device)
		throw std::runtime_error("[InputSys] Cannot find Device.");

	D3DDEVICE_CREATION_PARAMETERS params;
	if (FAILED(device->GetCreationParameters(&params)))
		throw std::runtime_error("[InputSys] GetCreationParameters failed.");

	m_hTargetWindow = params.hFocusWindow;
	m_ulOldWndProc = SetWindowLongPtr(m_hTargetWindow, GWLP_WNDPROC, (LONG_PTR)WndProc);

	if (!m_ulOldWndProc)
		throw std::runtime_error("[InputSys] SetWindowLongPtr failed.");

	std::cout << "[InputSys] OK \n";
}

void InputSys::Shutdown()
{
	if (m_ulOldWndProc)
		SetWindowLongPtr(m_hTargetWindow, GWLP_WNDPROC, m_ulOldWndProc);
	m_ulOldWndProc = 0;
}

extern LRESULT ImGui_ImplDX9_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT __stdcall InputSys::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Get().ProcessMessage(msg, wParam, lParam);
	if (g_Unload)
	{
		Sleep(50);
		return true;
	}

	if ( ImGui_ImplDX9_WndProcHandler( hWnd, msg, wParam, lParam ) )
		return true;

	return CallWindowProcW( ( WNDPROC )Get().m_ulOldWndProc, hWnd, msg, wParam, lParam );
}


bool InputSys::ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
	case WM_XBUTTONUP:
		return ProcessMouseMessage(uMsg, wParam, lParam);
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		return ProcessKeybdMessage(uMsg, wParam, lParam);
	default:
		return false;
	}
}

bool InputSys::ProcessMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto key = VK_LBUTTON;
	auto state = KeyState::None;
	switch (uMsg) {
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		state = uMsg == WM_MBUTTONUP ? KeyState::Up : KeyState::Down;
		key = VK_MBUTTON;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		state = uMsg == WM_RBUTTONUP ? KeyState::Up : KeyState::Down;
		key = VK_RBUTTON;
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		state = uMsg == WM_LBUTTONUP ? KeyState::Up : KeyState::Down;
		key = VK_LBUTTON;
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		state = uMsg == WM_XBUTTONUP ? KeyState::Up : KeyState::Down;
		key = (HIWORD(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
		break;
	default:
		return false;
	}

	if (state == KeyState::Up && m_iKeyMap[key] == KeyState::Down)
		m_iKeyMap[key] = KeyState::Pressed;
	else
		m_iKeyMap[key] = state;
	return true;
}

bool InputSys::ProcessKeybdMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto key = wParam;
	auto state = KeyState::None;

	switch (uMsg) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		state = KeyState::Down;
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		state = KeyState::Up;
		break;
	default:
		return false;
	}

	if (state == KeyState::Up && m_iKeyMap[int(key)] == KeyState::Down) {
		m_iKeyMap[int(key)] = KeyState::Pressed;

		auto& hotkey_callback = m_Hotkeys[key];

		if (hotkey_callback)
			hotkey_callback();

	}
	else {
		m_iKeyMap[int(key)] = state;
	}

	return true;
}
KeyState InputSys::GetKeyState(std::uint32_t vk)
{
	return m_iKeyMap[vk];
}
bool InputSys::IsKeyDown(std::uint32_t vk)
{
	return m_iKeyMap[vk] == KeyState::Down;
}
bool InputSys::WasKeyPressed(std::uint32_t vk)
{
	if (m_iKeyMap[vk] == KeyState::Pressed) {
		m_iKeyMap[vk] = KeyState::Up;
		return true;
	}
	return false;
}

void InputSys::RegisterHotkey(std::uint32_t vk, std::function<void(void)> f)
{
	m_Hotkeys[vk] = f;
}
void InputSys::RemoveHotkey(std::uint32_t vk)
{
	m_Hotkeys[vk] = nullptr;
}
