#pragma once
#if WITH_IMGUI

bool InitImGui(HWND hWnd);
void UpdateImGui();
LRESULT WINAPI ImGuiWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif