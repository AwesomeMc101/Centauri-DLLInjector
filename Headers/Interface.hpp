#include <d3d11.h>
#include <tchar.h>
#include "ImGUI/imgui.h"
#include "ImGUI/backends/imgui_impl_dx11.h"
#include "ImGUI/backends/imgui_impl_win32.h"

#include <Windows.h>
#include <ShObjIdl.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#include <TlHelp32.h>
#include <iomanip>
#include <Shlwapi.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <numeric>

#include "Injection.hpp"

int initui();


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Function Prototypes
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
