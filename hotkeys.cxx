#define UNICODE
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")

#include <windows.h>
#include <initguid.h>
#include <KnownFolders.h>
#include <shlobj.h>
#include <vector>
#include <string>

struct HotKey
{
	int id;
	UINT modifiers;
	UINT vk;
	std::wstring filename;
};

static wchar_t *hotkeys_directory = 0;
static std::vector<HotKey> hotkeys;

int map_key(const wchar_t *keyname, int len)
{
	struct KeyInfo {
		wchar_t *name;
		int code;
	};

	static KeyInfo map[] = {
		{L"VK_LBUTTON",             0x01}, {L"VK_RBUTTON",             0x02}, {L"VK_CANCEL",              0x03},
		{L"VK_MBUTTON",             0x04}, {L"VK_XBUTTON1",            0x05}, {L"VK_XBUTTON2",            0x06},
		{L"VK_BACK",                0x08}, {L"VK_TAB",                 0x09}, {L"VK_CLEAR",               0x0C},
		{L"VK_RETURN",              0x0D}, {L"VK_SHIFT",               0x10}, {L"VK_CONTROL",             0x11},
		{L"VK_MENU",                0x12}, {L"VK_PAUSE",               0x13}, {L"VK_CAPITAL",             0x14},
		{L"VK_KANA",                0x15}, {L"VK_HANGEUL",             0x15}, {L"VK_HANGUL",              0x15},
		{L"VK_JUNJA",               0x17}, {L"VK_FINAL",               0x18}, {L"VK_HANJA",               0x19},
		{L"VK_KANJI",               0x19}, {L"VK_ESCAPE",              0x1B}, {L"VK_CONVERT",             0x1C},
		{L"VK_NONCONVERT",          0x1D}, {L"VK_ACCEPT",              0x1E}, {L"VK_MODECHANGE",          0x1F},
		{L"VK_SPACE",               0x20}, {L"VK_PRIOR",               0x21}, {L"VK_NEXT",                0x22},
		{L"VK_END",                 0x23}, {L"VK_HOME",                0x24}, {L"VK_LEFT",                0x25},
		{L"VK_UP",                  0x26}, {L"VK_RIGHT",               0x27}, {L"VK_DOWN",                0x28},
		{L"VK_SELECT",              0x29}, {L"VK_PRINT",               0x2A}, {L"VK_EXECUTE",             0x2B},
		{L"VK_SNAPSHOT",            0x2C}, {L"VK_INSERT",              0x2D}, {L"VK_DELETE",              0x2E},
		{L"VK_HELP",                0x2F}, {L"VK_LWIN",                0x5B}, {L"VK_RWIN",                0x5C},
		{L"VK_APPS",                0x5D}, {L"VK_SLEEP",               0x5F}, {L"VK_NUMPAD0",             0x60},
		{L"VK_NUMPAD1",             0x61}, {L"VK_NUMPAD2",             0x62}, {L"VK_NUMPAD3",             0x63},
		{L"VK_NUMPAD4",             0x64}, {L"VK_NUMPAD5",             0x65}, {L"VK_NUMPAD6",             0x66},
		{L"VK_NUMPAD7",             0x67}, {L"VK_NUMPAD8",             0x68}, {L"VK_NUMPAD9",             0x69},
		{L"VK_MULTIPLY",            0x6A}, {L"VK_ADD",                 0x6B}, {L"VK_SEPARATOR",           0x6C},
		{L"VK_SUBTRACT",            0x6D}, {L"VK_DECIMAL",             0x6E}, {L"VK_DIVIDE",              0x6F},
		{L"VK_F1",                  0x70}, {L"VK_F2",                  0x71}, {L"VK_F3",                  0x72},
		{L"VK_F4",                  0x73}, {L"VK_F5",                  0x74}, {L"VK_F6",                  0x75},
		{L"VK_F7",                  0x76}, {L"VK_F8",                  0x77}, {L"VK_F9",                  0x78},
		{L"VK_F10",                 0x79}, {L"VK_F11",                 0x7A}, {L"VK_F12",                 0x7B},
		{L"VK_F13",                 0x7C}, {L"VK_F14",                 0x7D}, {L"VK_F15",                 0x7E},
		{L"VK_F16",                 0x7F}, {L"VK_F17",                 0x80}, {L"VK_F18",                 0x81},
		{L"VK_F19",                 0x82}, {L"VK_F20",                 0x83}, {L"VK_F21",                 0x84},
		{L"VK_F22",                 0x85}, {L"VK_F23",                 0x86}, {L"VK_F24",                 0x87},
		{L"VK_NUMLOCK",             0x90}, {L"VK_SCROLL",              0x91}, {L"VK_OEM_NEC_EQUAL",       0x92},
		{L"VK_OEM_FJ_JISHO",        0x92}, {L"VK_OEM_FJ_MASSHOU",      0x93}, {L"VK_OEM_FJ_TOUROKU",      0x94},
		{L"VK_OEM_FJ_LOYA",         0x95}, {L"VK_OEM_FJ_ROYA",         0x96}, {L"VK_LSHIFT",              0xA0},
		{L"VK_RSHIFT",              0xA1}, {L"VK_LCONTROL",            0xA2}, {L"VK_RCONTROL",            0xA3},
		{L"VK_LMENU",               0xA4}, {L"VK_RMENU",               0xA5}, {L"VK_BROWSER_BACK",        0xA6},
		{L"VK_BROWSER_FORWARD",     0xA7}, {L"VK_BROWSER_REFRESH",     0xA8}, {L"VK_BROWSER_STOP",        0xA9},
		{L"VK_BROWSER_SEARCH",      0xAA}, {L"VK_BROWSER_FAVORITES",   0xAB}, {L"VK_BROWSER_HOME",        0xAC},
		{L"VK_VOLUME_MUTE",         0xAD}, {L"VK_VOLUME_DOWN",         0xAE}, {L"VK_VOLUME_UP",           0xAF},
		{L"VK_MEDIA_NEXT_TRACK",    0xB0}, {L"VK_MEDIA_PREV_TRACK",    0xB1}, {L"VK_MEDIA_STOP",          0xB2},
		{L"VK_MEDIA_PLAY_PAUSE",    0xB3}, {L"VK_LAUNCH_MAIL",         0xB4}, {L"VK_LAUNCH_MEDIA_SELECT", 0xB5},
		{L"VK_LAUNCH_APP1",         0xB6}, {L"VK_LAUNCH_APP2",         0xB7}, {L"VK_OEM_1",               0xBA},
		{L"VK_OEM_PLUS",            0xBB}, {L"VK_OEM_COMMA",           0xBC}, {L"VK_OEM_MINUS",           0xBD},
		{L"VK_OEM_PERIOD",          0xBE}, {L"VK_OEM_2",               0xBF}, {L"VK_OEM_3",               0xC0},
		{L"VK_OEM_4",               0xDB}, {L"VK_OEM_5",               0xDC}, {L"VK_OEM_6",               0xDD},
		{L"VK_OEM_7",               0xDE}, {L"VK_OEM_8",               0xDF}, {L"VK_OEM_AX",              0xE1},
		{L"VK_OEM_102",             0xE2}, {L"VK_ICO_HELP",            0xE3}, {L"VK_ICO_00",              0xE4},
		{L"VK_PROCESSKEY",          0xE5}, {L"VK_ICO_CLEAR",           0xE6}, {L"VK_PACKET",              0xE7},
		{L"VK_OEM_RESET",           0xE9}, {L"VK_OEM_JUMP",            0xEA}, {L"VK_OEM_PA1",             0xEB},
		{L"VK_OEM_PA2",             0xEC}, {L"VK_OEM_PA3",             0xED}, {L"VK_OEM_WSCTRL",          0xEE},
		{L"VK_OEM_CUSEL",           0xEF}, {L"VK_OEM_ATTN",            0xF0}, {L"VK_OEM_FINISH",          0xF1},
		{L"VK_OEM_COPY",            0xF2}, {L"VK_OEM_AUTO",            0xF3}, {L"VK_OEM_ENLW",            0xF4},
		{L"VK_OEM_BACKTAB",         0xF5}, {L"VK_ATTN",                0xF6}, {L"VK_CRSEL",               0xF7},
		{L"VK_EXSEL",               0xF8}, {L"VK_EREOF",               0xF9}, {L"VK_PLAY",                0xFA},
		{L"VK_ZOOM",                0xFB}, {L"VK_NONAME",              0xFC}, {L"VK_PA1",                 0xFD},
		{L"VK_OEM_CLEAR",           0xFE}
	};

	if (len == 1)
	{
		wchar_t ch = towupper(keyname[0]);

		if (ch >= L'A' && ch <= L'Z' || ch >= L'0' && ch <= L'9')
			return (int)ch;
		else
			return 0;
	}
	else
	{
		for (int i = 0; i < sizeof(map) / sizeof(KeyInfo); i++)
		{
			const wchar_t *name = map[i].name + 3;
			int n = wcslen(name);

			if (n == len && _wcsnicmp(keyname, name, len) == 0)
				return map[i].code;
		}

		return 0;
	}
}

int initialize(int argc, wchar_t **argv)
{
	wchar_t *dir = 0;

	if (argc > 1)
	{
		DWORD attr = GetFileAttributes(argv[1]);

		if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
			dir = argv[1];
	}

	if (dir == 0)
	{
		wchar_t *home = 0;
		wchar_t subdir[] = L"\\hotkeys";

		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, 0, &home)))
		{
			dir = new wchar_t[wcslen(home) + sizeof(subdir) / sizeof(wchar_t) + 1];
			wcscpy(dir, home);
			wcscat(dir, subdir);
			CoTaskMemFree(home);
		}
	}

	hotkeys_directory = dir;

	return (dir != 0 ? 0 : 1);
}

bool parse_filename(const wchar_t *filename, UINT *modifiers, UINT *vk)
{
	int len = wcslen(filename);

	if (len <= 4 || _wcsnicmp(filename + len - 4, L".lnk", 4) != 0)
		return false;

	const wchar_t *end = filename + len - 4;
	const wchar_t *delim = wcschr(filename, L' ');

	if (delim != 0 && delim < end)
		end = delim;

	const wchar_t *s = filename;

	*modifiers = 0;
	*vk = 0;

	while (s < end && (delim = wcschr(s, L'+')) != 0 && delim < end)
	{
		len = delim - s;

		if (len == 3 && _wcsnicmp(s, L"win", 3) == 0)
		{
			if (*modifiers & MOD_WIN)
				return false;

			*modifiers |= MOD_WIN;
		}
		else if (len == 3 && _wcsnicmp(s, L"alt", 3) == 0)
		{
			if (*modifiers & MOD_ALT)
				return false;

			*modifiers |= MOD_ALT;
		}
		else if (len == 4 && _wcsnicmp(s, L"ctrl", 4) == 0)
		{
			if (*modifiers & MOD_CONTROL)
				return false;

			*modifiers |= MOD_CONTROL;
		}
		else if (len == 5 && _wcsnicmp(s, L"shift", 5) == 0)
		{
			if (*modifiers & MOD_SHIFT)
				return false;

			*modifiers |= MOD_SHIFT;
		}
		else
		{
			return false;
		}

		s = delim + 1;
	}

	if (s >= end)
		return false;

	len = end - s;
	*vk = map_key(s, len);

	return *vk != 0;
}

void reload_hotkeys()
{
	for (int i = 0; i < hotkeys.size(); i++)
		UnregisterHotKey(0, hotkeys[i].id);

	hotkeys.clear();

	std::wstring path = hotkeys_directory;
	path += + L"\\*.lnk";

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(path.c_str(), &ffd);

	if (hFind != INVALID_HANDLE_VALUE) do
	{
		HotKey hk = {0};

		if (parse_filename(ffd.cFileName, &hk.modifiers, &hk.vk))
		{
			hk.id = hotkeys.size() + 1;
			hk.filename = hotkeys_directory;
			hk.filename += L"\\";
			hk.filename += ffd.cFileName;

			if (RegisterHotKey(0, hk.id, hk.modifiers, hk.vk))
				hotkeys.push_back(hk);
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
}

int wmain(int argc, wchar_t **argv)
{
	if (initialize(argc, argv) != 0)
		return 1;

	RegisterHotKey(0, 0, MOD_WIN | MOD_ALT | MOD_CONTROL | MOD_SHIFT, (UINT)L'R');
	reload_hotkeys();

	MSG msg = {0};

	while (GetMessage(&msg, 0, 0, 0) != 0)
	{
		if (msg.message == WM_HOTKEY)
		{
			if (msg.wParam > 0 && msg.wParam <= hotkeys.size())
			{
				HotKey &hk = hotkeys[msg.wParam - 1];
				ShellExecute(0, L"open", hk.filename.c_str(), 0, 0, SW_SHOWNORMAL);
			}
			else if (msg.wParam == 0)
			{
				reload_hotkeys();
			}
		}
	}

	return 0;
}

int main()
{
	int argc = 0;
	wchar_t **argv = CommandLineToArgvW(GetCommandLine(), &argc);

	return wmain(argc, argv);
}
