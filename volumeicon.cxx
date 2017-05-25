#define UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define WM_VOLUMECHANGE     (WM_USER + 12)
#define WM_ENDPOINTCHANGE   (WM_USER + 13)

#pragma comment(lib, "Gdi32.lib")

#include <windows.h>
#include <shellapi.h>
#include <atlbase.h>
#include <atlsync.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <algorithm>

struct VOLUME_INFO
{
    UINT nStep;
    UINT cSteps;
    BOOL bMuted;
};

class VolumeMonitor : IMMNotificationClient, IAudioEndpointVolumeCallback
{
private:
    HWND                            m_hWnd;
    BOOL                            m_bRegisteredForEndpointNotifications;
    BOOL                            m_bRegisteredForVolumeNotifications;
    CComPtr<IMMDeviceEnumerator>    m_spEnumerator;
    CComPtr<IMMDevice>              m_spAudioEndpoint;
    CComPtr<IAudioEndpointVolume>   m_spVolumeControl;
    CCriticalSection                m_csEndpoint;
    long                            m_cRef;

    ~VolumeMonitor() {}

    HRESULT AttachToDefaultEndpoint()
    {
        m_csEndpoint.Enter();

        HRESULT hr = m_spEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &m_spAudioEndpoint);

        if (SUCCEEDED(hr))
        {
            hr = m_spAudioEndpoint->Activate(__uuidof(m_spVolumeControl), CLSCTX_INPROC_SERVER, NULL, (void**)&m_spVolumeControl);

            if (SUCCEEDED(hr))
            {
                hr = m_spVolumeControl->RegisterControlChangeNotify(this);
                m_bRegisteredForVolumeNotifications = SUCCEEDED(hr);
            }
        }

        m_csEndpoint.Leave();
        return hr;
    }

    void DetachFromEndpoint()
    {
        m_csEndpoint.Enter();

        if (m_spVolumeControl != NULL)
        {
            if (m_bRegisteredForVolumeNotifications)
            {
                m_spVolumeControl->UnregisterControlChangeNotify(this);
                m_bRegisteredForVolumeNotifications = FALSE;
            }

            m_spVolumeControl.Release();
        }

        if (m_spAudioEndpoint != NULL)
            m_spAudioEndpoint.Release();

        m_csEndpoint.Leave();
    }

    IFACEMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
    {
        if (flow == eRender)
        {
            if (m_hWnd != NULL)
                PostMessage(m_hWnd, WM_ENDPOINTCHANGE, 0, 0);
        }

        return S_OK;
    }

    IFACEMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify)
    {
        if (m_hWnd != NULL)
            PostMessage(m_hWnd, WM_VOLUMECHANGE, 0, 0);

        return S_OK;
    }

    IFACEMETHODIMP QueryInterface(const IID& iid, void** ppUnk)
    {
        if ((iid == __uuidof(IUnknown)) || (iid == __uuidof(IMMNotificationClient)))
        {
            *ppUnk = static_cast<IMMNotificationClient*>(this);
        }
        else if (iid == __uuidof(IAudioEndpointVolumeCallback))
        {
            *ppUnk = static_cast<IAudioEndpointVolumeCallback*>(this);
        }
        else
        {
            *ppUnk = NULL;
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) { return S_OK; }
    IFACEMETHODIMP OnDeviceAdded(LPCWSTR pwstrDeviceId) { return S_OK; }
    IFACEMETHODIMP OnDeviceRemoved(LPCWSTR pwstrDeviceId) { return S_OK; }
    IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) { return S_OK; }
    IFACEMETHODIMP OnDeviceQueryRemove() { return S_OK; }
    IFACEMETHODIMP OnDeviceQueryRemoveFailed() { return S_OK; }
    IFACEMETHODIMP OnDeviceRemovePending() { return S_OK; }

public:
    VolumeMonitor() :
        m_hWnd(NULL),
        m_bRegisteredForEndpointNotifications(FALSE),
        m_bRegisteredForVolumeNotifications(FALSE),
        m_cRef(1)
    {}

    HRESULT Initialize()
    {
        HRESULT hr = m_spEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));

        if (SUCCEEDED(hr))
        {
            hr = m_spEnumerator->RegisterEndpointNotificationCallback(this);

            if (SUCCEEDED(hr))
                hr = AttachToDefaultEndpoint();
        }

        return hr;
    }

    void Dispose()
    {
        DetachFromEndpoint();

        if (m_bRegisteredForEndpointNotifications)
        {
            m_spEnumerator->UnregisterEndpointNotificationCallback(this);
            m_bRegisteredForEndpointNotifications = FALSE;
        }
    }

    void SetWindow(HWND hWnd)
    {
        m_hWnd = hWnd;
    }

    HWND GetWindow()
    {
        return m_hWnd;
    }

    HRESULT GetLevelInfo(VOLUME_INFO* pInfo)
    {
        HRESULT hr = E_FAIL;

        m_csEndpoint.Enter();

        if (m_spVolumeControl != NULL)
        {
            hr = m_spVolumeControl->GetMute(&pInfo->bMuted);

            if (SUCCEEDED(hr))
                hr = m_spVolumeControl->GetVolumeStepInfo(&pInfo->nStep, &pInfo->cSteps);
        }

        m_csEndpoint.Leave();
        return hr;
    }

    void ChangeEndpoint()
    {
        DetachFromEndpoint();
        AttachToDefaultEndpoint();
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        long lRef = InterlockedDecrement(&m_cRef);

        if (lRef == 0)
            delete this;

        return lRef;
    }
};

static const char numbers[][6 * 10 - 1] = {
    "0000     0  0000  0000  0  0  0000  0     0000  0000  0000",
    "0  0     0     0     0  0  0  0     0        0  0  0  0  0",
    "0  0     0     0     0  0  0  0     0        0  0  0  0  0",
    "0  0     0     0     0  0  0  0     0        0  0  0  0  0",
    "0  0     0  0000  0000  0000  0000  0000     0  0000  0000",
    "0  0     0  0        0     0     0  0  0     0  0  0     0",
    "0  0     0  0        0     0     0  0  0     0  0  0     0",
    "0  0     0  0        0     0     0  0  0     0  0  0     0",
    "0000     0  0000  0000     0  0000  0000     0  0000     0"
};

static VolumeMonitor *vol = NULL;
static HICON icons[102] = {};
static DWORD fore = 0xFF000000;
static DWORD back = 0x00000000;

void draw_number(DWORD *buffer, int x0, int n)
{
    const int d = 16;
    const int y0 = 3;
    const int sx0 = n * 6;

    DWORD *p = &buffer[y0 * d + x0];

    for (int y = 0; y < 9; y++)
    {
        for (int x = 0; x < 4; x++)
            p[x] = (numbers[y][sx0 + x] == '0') ? fore : back;

        p += d;
    }
}

void InitializeIcons()
{
    const int d = 16;

    BITMAPV5HEADER bi = {};
    bi.bV5Size        = sizeof(BITMAPV5HEADER);
    bi.bV5Width       = d;
    bi.bV5Height      = -d;
    bi.bV5Planes      = 1;
    bi.bV5BitCount    = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask     = 0x00FF0000;
    bi.bV5GreenMask   = 0x0000FF00;
    bi.bV5BlueMask    = 0x000000FF;
    bi.bV5AlphaMask   = 0xFF000000;

    DWORD *buffer = 0;
    HDC hdc = GetDC(NULL);
    HBITMAP hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&buffer, NULL, (DWORD)0);
    HBITMAP hMask = CreateBitmap(d, d, 1, 1, NULL);

    ReleaseDC(NULL, hdc);

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = hMask;
    ii.hbmColor = hBitmap;

    for (int i = 0; i < 102; i++)
    {
        for (int j = 0; j < d*d; j++)
            buffer[j] = back;

        if (i == 101)
        {
            // TODO: mute
        }
        else
        {
            draw_number(buffer, -1 + (4 + 1) * 2, i % 10);

            if (i >= 10)
                draw_number(buffer, -1 + (4 + 1) * 1, (i / 10) % 10);

            if (i >= 100)
                draw_number(buffer, -1 + (4 + 1) * 0, (i / 100) % 10);
        }

        icons[i] = CreateIconIndirect(&ii);
    }

    DeleteObject(hMask);
    DeleteObject(hBitmap);
}

int GetVolumeLevel()
{
    VOLUME_INFO info = {0};
    vol->GetLevelInfo(&info);

    if (info.bMuted)
        return 0;

    return std::max<int>(0, std::min<int>(100, 100 * info.nStep / (info.cSteps - 1)));
}

void UpdateNotificationIcon()
{
    int level = GetVolumeLevel();

    NOTIFYICONDATA notif = { sizeof(notif) };
    notif.hWnd = vol->GetWindow();
    notif.uID = 1;
    notif.uFlags = NIF_ICON | NIF_TIP;
    notif.uVersion = NOTIFYICON_VERSION_4;
    notif.hIcon = icons[level];
    swprintf(notif.szTip, 64, L"Volumen: %d%%", level);

    Shell_NotifyIcon(NIM_MODIFY, &notif);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_VOLUMECHANGE:
        {
            UpdateNotificationIcon();
            return 0;
        }

        case WM_ENDPOINTCHANGE:
        {
            vol->ChangeEndpoint();
            UpdateNotificationIcon();
            return 0;
        }

        case WM_ERASEBKGND:
        {
            return 1;
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

int wmain(int argc, wchar_t **argv)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (SUCCEEDED(hr))
    {
        vol = new (std::nothrow) VolumeMonitor();

        if (SUCCEEDED(vol->Initialize()))
        {
            const wchar_t g_szWindowClass[] = L"volumeicon";
            HINSTANCE hInstance = GetModuleHandle(NULL);

            WNDCLASSEX wcex     = { sizeof(wcex) };
            wcex.style          = 0;
            wcex.lpfnWndProc    = WndProc;
            wcex.hInstance      = hInstance;
            wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
            wcex.hbrBackground  = NULL;
            wcex.lpszClassName  = g_szWindowClass;

            RegisterClassEx(&wcex);

            HWND hWnd = CreateWindowEx(WS_EX_NOACTIVATE, g_szWindowClass, NULL, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

            if (hWnd)
            {
                InitializeIcons();

                int level = GetVolumeLevel();

                NOTIFYICONDATA notif = { sizeof(notif) };
                notif.hWnd = hWnd;
                notif.uID = 1;
                notif.uFlags = NIF_ICON | NIF_TIP;
                notif.uVersion = NOTIFYICON_VERSION_4;
                notif.hIcon = icons[level];
                swprintf(notif.szTip, 64, L"Volumen: %d%%", level);

                Shell_NotifyIcon(NIM_ADD, &notif);
                vol->SetWindow(hWnd);

                MSG msg;

                while (GetMessage(&msg, NULL, 0, 0))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                notif.uFlags = 0;
                Shell_NotifyIcon(NIM_DELETE, &notif);
            }

            vol->Dispose();
            vol->Release();
        }

        CoUninitialize();
    }

    return 0;
}

int main()
{
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLine(), &argc);

    return wmain(argc, argv);
}
