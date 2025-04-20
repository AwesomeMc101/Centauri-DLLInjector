#define _CRT_SECURE_NO_WARNINGS
#include "interface.hpp"

// Forward declare message handler from imgui_impl_win32.cpp

// Global Variables
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
// Main code

void WriteProcessList(std::vector<char*>& pL, std::vector<DWORD>& pids, char* sQ) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        //omggggggg
        MessageBoxA(NULL, "Unable to scan processes.", "error 678", MB_OK);
        return;
    }


    PROCESSENTRY32 pE;
    pE.dwSize = sizeof(pE);
    if (Process32First(hSnap, &pE)) {
        do {
            char* NX = (char*)malloc(sizeof(char) * (lstrlenW(pE.szExeFile) + 1));
            std::transform(pE.szExeFile, pE.szExeFile + lstrlenW(pE.szExeFile), NX, [](WCHAR x) {
                return (x > 255 ? '?' : x);
                });
            NX[lstrlenW(pE.szExeFile)] = '\0';

            if (strstr(NX, sQ) == nullptr) { //query functionality 
                continue;
            }

            pL.push_back(NX);
            pids.push_back(pE.th32ProcessID);
        } while (Process32Next(hSnap, &pE));
    }
    CloseHandle(hSnap);
}

int initui()
{
    SetConsoleTitleA("DLL Injector Debug Console");
    std::cout << "DLL Injector - By AwesomeMc101" << "\n";
    HWND cons = GetConsoleWindow();
    CloseWindow(cons);

    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("Centauri"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("CENTAURI DLLInjector"), WS_OVERLAPPEDWINDOW, 100, 100, 1200, 600, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;
    //ShowWindow(hwnd, SW_HIDE);
    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    ImFont* fskinny_T = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuil.ttf", 32.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    ImFont* fskinny_N = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuisl.ttf", 20.0f, NULL, io.Fonts->GetGlyphRangesDefault());

    char* filePath = (char*)"quote unquote dll file path lol";
    int proc_selected = 0;

    //deprecated
    int type_selected = 0;

    //process data
    std::vector<char*> ProcessList;
    std::vector<DWORD> pids;

    //settings
    bool doSuspend = true;
    bool findInternalAddr = true;
    bool doSyscall = true;

    char searchQuery[32] = "";

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f)); // place the next window in the top left corner (0,0)
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("!", 0, window_flags);

        ImGui::PushFont(fskinny_T);
        ImGui::SeparatorText("DLL Injector");
        ImGui::PopFont();

        ImGui::PushFont(fskinny_N);

        if (ImGui::Button("Select DLL File...")) {
            IFileOpenDialog* pFileOpen;
            HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
            if (hr < 0) {
                MessageBoxA(0, "FAILED!", "err", MB_OK);
                abort();
            }
            hr = pFileOpen->Show(0);
            if (hr < 0) {
                MessageBoxA(0, "bro tryna make my job harder by not selecting shi", "err", MB_OK);
            }
            else {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                //convert wchar to char cuz windows smdddd
                size_t len = lstrlenW(pszFilePath); //how it feels to type in strlen with a W and get a function
                filePath = (char*)malloc(sizeof(char) * (len+1));
                std::transform(pszFilePath, (pszFilePath + lstrlenW(pszFilePath)), filePath, [](WCHAR Y) {
                    return ((Y > 255) ? ' ' : (Y)); //yeah, this is called safe casting pal.
                    });
                filePath[len] = '\0';
            }
        }

        char* textLol = (char*)malloc(sizeof(char) * (strlen("Selected File: ") + strlen(filePath) + 1));
        strcpy(textLol, "Selected File: ");
        strcpy(textLol + strlen("Selected File: "), filePath);
        textLol[strlen("Selected File: ") + strlen(filePath)] = '\0';
        ImGui::Text(textLol);


        ImGui::InputText("Query..", searchQuery, 32);
        WriteProcessList(ProcessList, pids, searchQuery);
        ImGui::ListBox("Active Processes", &proc_selected, ProcessList.data(), ProcessList.size());
        char* textLol2 = nullptr;
        if (proc_selected < ProcessList.size()) {
            textLol2 = (char*)malloc(sizeof(char) * (strlen("Selected Process: ") + strlen(ProcessList[proc_selected])));
            strcpy(textLol2, "Selected Process: ");
            strcpy(textLol2 + strlen("Selected Process: "), ProcessList[proc_selected]);
            ImGui::Text(textLol2);
            ImGui::Text(std::to_string(pids[proc_selected]).c_str());
        }
        else {
            ImGui::Text("Select a valid process...");
        }

        if (ImGui::Button("Inject")) {
            InjectResults X;
            switch (type_selected) {
            case 0:
                X = baseInjector(filePath, pids[proc_selected], doSuspend, findInternalAddr, doSyscall);
                break;
            }
           
            switch (X) {
            case SUCCESS:
                MessageBoxA(0, "Injected DLL successfully.", "Success!", MB_OK);
                break;
            case FAIL_INJECT:
                MessageBoxA(0, "Failed to write into process memory.", "Fail!", MB_OK);
                break;
            case FAIL_NOMEM:
                MessageBoxA(0, "Failed to allocate process memory.", "Fail!", MB_OK);
                break;
            case FAIL_THREAD:
                MessageBoxA(0, "Failed to start thread or catch handle.", "Fail!", MB_OK);
                break;
            case FAIL_NOPROC:
                MessageBoxA(0, "Failed to find process.", "Fail!", MB_OK);
                break;
            case FAIL_PREFADDRESS:
                MessageBoxA(0, "DLL didn't load at ImageBase.", "Fail!", MB_OK);
                break;
            }
        }

        if (ImGui::CollapsingHeader("Security Settings")) {
            ImGui::Checkbox("Suspend Process During Injection", &doSuspend);
            ImGui::Checkbox("Find Internal LoadLibA (& Internal Nt Functions if Syscall)", &findInternalAddr);
            ImGui::Checkbox("Syscall Alloc/Thread (Nt)", &doSyscall);
        }

        ImGui::PopFont();
        ImGui::End();
        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync

        //free 
        free(textLol);
        if (textLol2 != nullptr) {
            free(textLol2);
        }
        while (ProcessList.size()) {
            free(ProcessList.back());
            ProcessList.pop_back();
        }
        pids.clear();
    }


    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain 
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_DROPFILES:
        break;
    default:
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
