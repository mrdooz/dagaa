#pragma once
#if WITH_IMGUI
#include <contrib/imgui/imgui_internal.h>

bool InitImGui(HWND hWnd);
void UpdateImGui();
LRESULT WINAPI ImGuiWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif