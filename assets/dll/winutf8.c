/* winutf8.c - UTF-8 wrapper functions for Win32 API
 *
 * Reconstructed from usage in csvexport.c for GCC/MinGW compilation.
 * In MultiByte (non-Unicode) builds, these map directly to the ANSI versions.
 * Original Copyright (c) CompuPhase
 */

#define WINVER 0x0500
#include <windows.h>
#include <string.h>
#include "winutf8.h"

DWORD GetModuleFileNameU(HINSTANCE hModule, LPSTR lpFilename, DWORD nSize)
{
  return GetModuleFileNameA((HMODULE)hModule, lpFilename, nSize);
}

DWORD GetPrivateProfileStringU(LPCSTR lpAppName, LPCSTR lpKeyName,
                               LPCSTR lpDefault, LPSTR lpReturnedString,
                               DWORD nSize, LPCSTR lpFileName)
{
  return GetPrivateProfileStringA(lpAppName, lpKeyName, lpDefault,
                                  lpReturnedString, nSize, lpFileName);
}

BOOL WritePrivateProfileStringU(LPCSTR lpAppName, LPCSTR lpKeyName,
                                LPCSTR lpString, LPCSTR lpFileName)
{
  return WritePrivateProfileStringA(lpAppName, lpKeyName, lpString, lpFileName);
}

INT_PTR DialogBoxU(HINSTANCE hInstance, LPCSTR lpTemplateName,
                   HWND hWndParent, DLGPROC lpDialogFunc)
{
  return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, 0);
}

INT_PTR DialogBoxParamU(HINSTANCE hInstance, LPCSTR lpTemplateName,
                        HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
  return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

HFONT CreateFontU(int cHeight, int cWidth, int cEscapement, int cOrientation,
                  int cWeight, DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut,
                  DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision,
                  DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName)
{
  return CreateFontA(cHeight, cWidth, cEscapement, cOrientation, cWeight,
                     bItalic, bUnderline, bStrikeOut, iCharSet,
                     iOutPrecision, iClipPrecision, iQuality,
                     iPitchAndFamily, pszFaceName);
}

int MessageBoxU(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
  return MessageBoxA(hWnd, lpText, lpCaption, uType);
}

BOOL SetDlgItemTextU(HWND hDlg, int nIDDlgItem, LPCSTR lpString)
{
  return SetDlgItemTextA(hDlg, nIDDlgItem, lpString);
}

UINT GetDlgItemTextU(HWND hDlg, int nIDDlgItem, LPSTR lpString, int cchMax)
{
  return GetDlgItemTextA(hDlg, nIDDlgItem, lpString, cchMax);
}

int Combobox_AddStringU(HWND hwndCtl, LPCSTR lpsz)
{
  return (int)SendMessageA(hwndCtl, CB_ADDSTRING, 0, (LPARAM)lpsz);
}

int Combobox_FindStringU(HWND hwndCtl, int indexStart, LPCSTR lpszFind, BOOL exact)
{
  if (exact)
    return (int)SendMessageA(hwndCtl, CB_FINDSTRINGEXACT, (WPARAM)indexStart, (LPARAM)lpszFind);
  else
    return (int)SendMessageA(hwndCtl, CB_FINDSTRING, (WPARAM)indexStart, (LPARAM)lpszFind);
}

int Combobox_GetLBTextU(HWND hwndCtl, int index, LPSTR lpszBuffer, int cchMax)
{
  int len = (int)SendMessageA(hwndCtl, CB_GETLBTEXTLEN, (WPARAM)index, 0);
  if (len == CB_ERR || len >= cchMax)
    return CB_ERR;
  return (int)SendMessageA(hwndCtl, CB_GETLBTEXT, (WPARAM)index, (LPARAM)lpszBuffer);
}
