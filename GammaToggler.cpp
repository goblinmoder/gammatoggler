#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <wcsplugin.h>
#include <icm.h>
#include <iostream>
#include <list>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "mscms.lib")

using namespace std;

const wstring profileName = L"Bright.icc";
wstring profilePath;
bool gamma = false;
list<DISPLAY_DEVICEW> monitors;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

wstring ExePath()
{
    TCHAR buffer[MAX_PATH] = {0};
    GetModuleFileName(NULL, buffer, MAX_PATH);
    wstring::size_type pos = wstring(buffer).find_last_of(L"\\/");
    return wstring(buffer).substr(0, pos);
}

void ApplyProfile(LPCWSTR monitorDeviceID, PCWSTR profilePath)
{
    wstring profileSource = ExePath() + L"\\" + profileName;
    InstallColorProfileW(NULL, profileSource.c_str());
    WcsSetUsePerUserProfiles(monitorDeviceID, CLASS_MONITOR, TRUE);
    WcsAssociateColorProfileWithDevice(
            WCS_PROFILE_MANAGEMENT_SCOPE_CURRENT_USER,
            profilePath,
            monitorDeviceID);
    WcsSetDefaultColorProfile(
            WCS_PROFILE_MANAGEMENT_SCOPE_CURRENT_USER,
            monitorDeviceID,
            CPT_ICC,
            CPST_NONE,
            0,
            profilePath);
}

void RemoveProfile(LPCWSTR monitorDeviceID, LPCWSTR profilePath)
{
    WcsDisassociateColorProfileFromDevice(
            WCS_PROFILE_MANAGEMENT_SCOPE_CURRENT_USER,
            profilePath,
            monitorDeviceID);
}

list<DISPLAY_DEVICEW> getMonitors()
{
    list<DISPLAY_DEVICEW> monitors;
    DWORD adapterIndex = 0;
    DISPLAY_DEVICEW adapter;

    while (true)
    {
        ZeroMemory(&adapter, sizeof(adapter));
        adapter.cb = sizeof(adapter);

        if (!EnumDisplayDevicesW(NULL, adapterIndex, &adapter, 0))
            break;

        DWORD monitorIndex = 0;
        DISPLAY_DEVICEW monitor;

        while (true)
        {
            ZeroMemory(&monitor, sizeof(monitor));
            monitor.cb = sizeof(monitor);

            if (!EnumDisplayDevicesW(adapter.DeviceName,
                                     monitorIndex,
                                     &monitor,
                                     0))
                break;

            if (monitor.StateFlags & DISPLAY_DEVICE_ACTIVE)
            {
                monitors.push_front(monitor);
            }

            monitorIndex++;
        }
        adapterIndex++;
    }
    return monitors;
}

wstring getProfilePath(wstring profileName)
{
    WCHAR colorDir[MAX_PATH];
    DWORD size = MAX_PATH;
    GetColorDirectoryW(NULL, colorDir, &size);
    return wstring(colorDir) + L"\\" + profileName;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Gamma Toggler Window";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                   // Optional window styles.
        CLASS_NAME,          // Window class
        L"Gamma Toggler",    // Window text
        WS_OVERLAPPEDWINDOW, // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 250,

        NULL,      // Parent window
        NULL,      // Menu
        hInstance, // Instance handle
        NULL       // Additional application data
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    profilePath = getProfilePath(profileName);
    monitors = getMonitors();

    RegisterHotKey(hwnd, 1, MOD_ALT | MOD_NOREPEAT, 0x47);

    // Run the message loop.

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_HOTKEY:
    {
        if (gamma)
        {
            for (DISPLAY_DEVICEW monitor : monitors)
            {
                RemoveProfile(monitor.DeviceID, profilePath.c_str());
            }
        }
        else
        {
            for (DISPLAY_DEVICEW monitor : monitors)
            {
                ApplyProfile(monitor.DeviceID, profilePath.c_str());
            }
        }
        gamma = !gamma;
    }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
    }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
