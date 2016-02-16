#pragma once

//---------------------------------------------------------------------------//
//
// Main.hpp
//  TTB Plugin Template (C++11)
//
//---------------------------------------------------------------------------//

#include <windows.h>

#include "Settings.hpp"

//---------------------------------------------------------------------------//

extern HINSTANCE g_hInst;

//---------------------------------------------------------------------------//

void RegisterMyHotkey  (HWND hwnd, INT32 id, const command& cmd);
void UnregisterMyHotkey(HWND hwnd, INT32 id);

void RegisterAllHotkeys  (HWND hwnd);
void UnregisterAllHotkeys(HWND hwnd);

//---------------------------------------------------------------------------//

// Main.hpp