#include <windows.h>
#include <iostream>
#include <unordered_map>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "ntdll.lib")
HHOOK hMouseHook;
POINT MousePos;
CURSORINFO CursorInfo{ };

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_opt_(MaximumLength, Length) PWCH Buffer;
} UNICODE_STRING, * PUNICODE_STRING;
typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor; // PSECURITY_DESCRIPTOR;
    PVOID SecurityQualityOfService; // PSECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

#define THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE 0x40
extern "C" NTSTATUS NTAPI NtCreateThreadEx(
    OUT PHANDLE hThread,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE ProcessHandle,
    IN PVOID StartRoutine,
    IN PVOID Argument,
    IN ULONG CreateFlags,
    IN SIZE_T ZeroBits,
    IN SIZE_T StackSize,
    IN SIZE_T MaximumStackSize,
    IN PVOID AttributeList);

int GetBaseDPI()
{
    HDC screen = GetDC(NULL);
    int dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(NULL, screen);
    return dpi;
}


LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
        CursorInfo.cbSize = sizeof(CursorInfo);
        GetCursorInfo(&CursorInfo);
        float ScaleFactor = float(GetDpiForWindow(GetConsoleWindow())) / GetBaseDPI();
        CursorInfo.ptScreenPos.x *= ScaleFactor;
        CursorInfo.ptScreenPos.y *= ScaleFactor;
        ScreenToClient(GetConsoleWindow(), &CursorInfo.ptScreenPos);
        MousePos = CursorInfo.ptScreenPos;
        printf("Cursor Position: %d, %d\n", CursorInfo.ptScreenPos.x, CursorInfo.ptScreenPos.y);

        
        
    }

    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}
bool IsHookInstalled = false;
bool IsHookStillValid() {
    if (hMouseHook == NULL) return false;
	if (!IsHookInstalled) return false;
    __try {
        if (!CallNextHookEx(hMouseHook, 0, 0, 0)) {
          
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    return true;
}
    
void InstallHooks()
{

   

    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
    if (hMouseHook == NULL)
    {
        std::cerr << "Failed to install mouse hook!" << std::endl;
    }
    else
    {
        std::cout << "Mouse hook installed successfully!" << std::endl;
    }
	IsHookInstalled = true;
}

void UninstallHooks()
{

    if (hMouseHook != NULL)
    {
        UnhookWindowsHookEx(hMouseHook);
        std::cout << "Mouse hook uninstalled!" << std::endl;
    }
}
void InputIntegrityThread()
{
    while (true)
    {
		if (!IsHookStillValid())
		{
			std::cout << "Mouse hook is not valid, reinstalling..." << std::endl;
			UninstallHooks();
			InstallHooks();
            // You could also add a ban here if this is within a DRM!
		}
		Sleep(1000); 
    }
}
int main()
{
    InstallHooks();

    MSG msg;
    HANDLE hThread = NULL;
    NtCreateThreadEx(&hThread,
        MAXIMUM_ALLOWED,
        NULL,
        GetCurrentProcess(),
        InputIntegrityThread,
        NULL,
        THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE,
        0, 0, 0, NULL);

    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                break;
        }
    }

    UninstallHooks();

    return 0;
}