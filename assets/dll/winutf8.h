/* winutf8.h - UTF-8 wrapper functions for Win32 API
 *
 * Reconstructed from usage in csvexport.c for GCC/MinGW compilation.
 * In MultiByte (non-Unicode) builds, these map directly to the ANSI versions.
 * Original Copyright (c) CompuPhase
 */

#ifndef WINUTF8_H
#define WINUTF8_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

DWORD GetModuleFileNameU(HINSTANCE hModule, LPSTR lpFilename, DWORD nSize);
DWORD GetPrivateProfileStringU(LPCSTR lpAppName, LPCSTR lpKeyName,
                               LPCSTR lpDefault, LPSTR lpReturnedString,
                               DWORD nSize, LPCSTR lpFileName);
BOOL WritePrivateProfileStringU(LPCSTR lpAppName, LPCSTR lpKeyName,
                                LPCSTR lpString, LPCSTR lpFileName);

INT_PTR DialogBoxU(HINSTANCE hInstance, LPCSTR lpTemplateName,
                   HWND hWndParent, DLGPROC lpDialogFunc);
INT_PTR DialogBoxParamU(HINSTANCE hInstance, LPCSTR lpTemplateName,
                        HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);

HFONT CreateFontU(int cHeight, int cWidth, int cEscapement, int cOrientation,
                  int cWeight, DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut,
                  DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision,
                  DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName);

int MessageBoxU(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
BOOL SetDlgItemTextU(HWND hDlg, int nIDDlgItem, LPCSTR lpString);
UINT GetDlgItemTextU(HWND hDlg, int nIDDlgItem, LPSTR lpString, int cchMax);

int Combobox_AddStringU(HWND hwndCtl, LPCSTR lpsz);
int Combobox_FindStringU(HWND hwndCtl, int indexStart, LPCSTR lpszFind, BOOL exact);
int Combobox_GetLBTextU(HWND hwndCtl, int index, LPSTR lpszBuffer, int cchMax);

#ifdef __cplusplus
}
#endif

#endif /* WINUTF8_H */
