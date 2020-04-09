/*
 * list-immersive-colors - Lists Windows immersive color values.
 *
 * Copyright © 2020 Pete Batard <pete@akeo.ie>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Ignore deprecated warnings for GetVersionEx()
#pragma warning(disable: 4996)
#pragma warning(disable: 28159)

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "msapi_utf8.h"

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#ifndef APP_VERSION
#define APP_VERSION_STR "[DEV]"
#else
#define APP_VERSION_STR STRINGIFY(APP_VERSION)
#endif

#define MAX_LIBRARY_HANDLES 32

#define PF_TYPE(api, ret, proc, args)       typedef ret (api *proc##_t)args
#define PF_DECL(proc)                       static proc##_t pf##proc = NULL
#define PF_TYPE_DECL(api, ret, proc, args)  PF_TYPE(api, ret, proc, args); PF_DECL(proc)
#define PF_INIT_ID(proc, name, id)          if (pf##proc == NULL) pf##proc =    \
	(proc##_t) GetProcAddress(GetLibraryHandle(#name), MAKEINTRESOURCEA(id))
#define PF_INIT_ID_OR_OUT(proc, name, id)   do {PF_INIT_ID(proc, name, id);     \
	if (pf##proc == NULL) {fprintf(stderr, "Unable to locate %s() in %s.dll\n", \
	#proc, #name); goto out;} } while(0)

PF_TYPE_DECL(WINAPI, DWORD, GetImmersiveUserColorSetPreference, (BOOL, BOOL));
PF_TYPE_DECL(WINAPI, DWORD, GetImmersiveColorTypeFromName, (LPCWSTR));
PF_TYPE_DECL(WINAPI, DWORD, GetImmersiveColorFromColorSetEx, (DWORD, DWORD, BOOL, DWORD));
PF_TYPE_DECL(WINAPI, LPWSTR*, GetImmersiveColorNamedTypeByIndex, (DWORD));

enum WindowsVersion {
	WINDOWS_UNDEFINED = -1,
	WINDOWS_UNSUPPORTED = 0,
	WINDOWS_XP = 0x51,
	WINDOWS_2003 = 0x52,	// Also XP_64
	WINDOWS_VISTA = 0x60,	// Also 2008
	WINDOWS_7 = 0x61,		// Also 2008_R2
	WINDOWS_8 = 0x62,		// Also 2012
	WINDOWS_8_1 = 0x63,		// Also 2012_R2
	WINDOWS_10_PREVIEW1 = 0x64,
	WINDOWS_10 = 0xA0,
	WINDOWS_MAX
};

HMODULE  OpenedLibrariesHandle[MAX_LIBRARY_HANDLES];
uint16_t OpenedLibrariesHandleSize = 0;
int  nWindowsVersion = WINDOWS_UNDEFINED, nWindowsBuildNumber = -1;

static __inline HMODULE GetLibraryHandle(char* szLibraryName) {
	HMODULE h = NULL;
	if ((h = GetModuleHandleA(szLibraryName)) == NULL) {
		assert(OpenedLibrariesHandleSize < MAX_LIBRARY_HANDLES);
		if (OpenedLibrariesHandleSize >= MAX_LIBRARY_HANDLES) {
			fprintf(stderr, "Error: MAX_LIBRARY_HANDLES is too small\n");
		} else {
			h = LoadLibraryA(szLibraryName);
			if (h != NULL)
				OpenedLibrariesHandle[OpenedLibrariesHandleSize++] = h;
		}
	}
	return h;
}

static __inline char* appname(const char* path)
{
	static char appname[128];
	_splitpath_s(path, NULL, 0, NULL, 0, appname, sizeof(appname), NULL, 0);
	return appname;
}

// From smartmontools os_win32.cpp
static void GetWindowsVersion(void)
{
	OSVERSIONINFOEXA vi, vi2;
	unsigned major, minor;
	ULONGLONG major_equal, minor_equal;
	BOOL ws;

	nWindowsVersion = WINDOWS_UNDEFINED;

	memset(&vi, 0, sizeof(vi));
	vi.dwOSVersionInfoSize = sizeof(vi);
	if (!GetVersionExA((OSVERSIONINFOA*)&vi)) {
		memset(&vi, 0, sizeof(vi));
		vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
		if (!GetVersionExA((OSVERSIONINFOA*)&vi))
			return;
	}

	if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {

		if (vi.dwMajorVersion > 6 || (vi.dwMajorVersion == 6 && vi.dwMinorVersion >= 2)) {
			// Starting with Windows 8.1 Preview, GetVersionEx() does no longer report the actual OS version
			// See: http://msdn.microsoft.com/en-us/library/windows/desktop/dn302074.aspx
			// And starting with Windows 10 Preview 2, Windows enforces the use of the application/supportedOS
			// manifest in order for VerSetConditionMask() to report the ACTUAL OS major and minor...

			major_equal = VerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL);
			for (major = vi.dwMajorVersion; major <= 9; major++) {
				memset(&vi2, 0, sizeof(vi2));
				vi2.dwOSVersionInfoSize = sizeof(vi2); vi2.dwMajorVersion = major;
				if (!VerifyVersionInfoA(&vi2, VER_MAJORVERSION, major_equal))
					continue;
				if (vi.dwMajorVersion < major) {
					vi.dwMajorVersion = major; vi.dwMinorVersion = 0;
				}

				minor_equal = VerSetConditionMask(0, VER_MINORVERSION, VER_EQUAL);
				for (minor = vi.dwMinorVersion; minor <= 9; minor++) {
					memset(&vi2, 0, sizeof(vi2)); vi2.dwOSVersionInfoSize = sizeof(vi2);
					vi2.dwMinorVersion = minor;
					if (!VerifyVersionInfoA(&vi2, VER_MINORVERSION, minor_equal))
						continue;
					vi.dwMinorVersion = minor;
					break;
				}

				break;
			}
		}

		if (vi.dwMajorVersion <= 0xf && vi.dwMinorVersion <= 0xf) {
			ws = (vi.wProductType <= VER_NT_WORKSTATION);
			nWindowsVersion = vi.dwMajorVersion << 4 | vi.dwMinorVersion;
			if (nWindowsVersion < 0x51)
				nWindowsVersion = WINDOWS_UNSUPPORTED;
		}
	}

	nWindowsBuildNumber = vi.dwBuildNumber;
}

int main_utf8(int argc, char** argv)
{
	int i;
	uint8_t r, g, b;
	wchar_t name[128], **named_type;
	DWORD type;
	COLORREF color;

	fprintf(stderr, "%s %s © 2020 Pete Batard <pete@akeo.ie>\n\n",
		appname(argv[0]), APP_VERSION_STR);

	GetWindowsVersion();

	if (nWindowsVersion < WINDOWS_10) {
		fprintf(stderr, "This application requires Windows 10 or later.\n");
		goto out;
	}

	PF_INIT_ID_OR_OUT(GetImmersiveColorFromColorSetEx, UxTheme, 95);
	PF_INIT_ID_OR_OUT(GetImmersiveColorTypeFromName, UxTheme, 96);
	PF_INIT_ID_OR_OUT(GetImmersiveUserColorSetPreference, UxTheme, 98);
	PF_INIT_ID_OR_OUT(GetImmersiveColorNamedTypeByIndex, UxTheme, 100);

	printf("[Sample] Visual sample of the colour (provided your console supports 24-bit colour output).\n");
	printf("[Colour] RGB value of the colour.\n");
	printf("[Indx]   Value passed to GetImmersiveColorNamedTypeByIndex() to retrieve [Name].\n");
	printf("[Name]   Value passed to GetImmersiveColorTypeFromName() to retreive [Type]\n");
	printf("[Type]   Value passed to GetImmersiveColorFromColorSetEx() to retreive [Colour].\n\n");

	printf("[Sample] [Colour] [Indx] [Type] [Name]\n");

	for (i = 0; ; i++) {
		named_type = pfGetImmersiveColorNamedTypeByIndex(i);
		if (named_type == NULL)
			break;
		wcscpy_s(name, ARRAYSIZE(name), L"Immersive");
		wcscat_s(name, ARRAYSIZE(name), *named_type);
		type = pfGetImmersiveColorTypeFromName(name);
		color = pfGetImmersiveColorFromColorSetEx(
			pfGetImmersiveUserColorSetPreference(FALSE, FALSE),
			type, FALSE, 0);
		r = (uint8_t)(color);
		g = (uint8_t)(color >> 8);
		b = (uint8_t)(color >> 16);
		printf("\x1b[48;2;%03d;%03d;%03dm        \x1b[0m "
			"0x%02x%02x%02x 0x%04x 0x%04x %S\n",
			r, g, b, r, g, b, i, type, name);
	}

out:
	while (OpenedLibrariesHandleSize > 0)
		FreeLibrary(OpenedLibrariesHandle[--OpenedLibrariesHandleSize]);
	return 0;
}

int wmain(int argc, wchar_t** argv16)
{
	SetConsoleOutputCP(CP_UTF8);
	char** argv = calloc(argc, sizeof(char*));
	for (int i = 0; i < argc; i++)
		argv[i] = wchar_to_utf8(argv16[i]);
	int r = main_utf8(argc, argv);
	for (int i = 0; i < argc; i++)
		free(argv[i]);
	free(argv);
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif
	return r;
}
