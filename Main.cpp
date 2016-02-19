//---------------------------------------------------------------------------//
//
// Main.cpp
//  プラグインのメイン処理
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

#include <windows.h>
#include <strsafe.h>

#include "Plugin.hpp"
#include "MessageDef.hpp"
#include "Utility.hpp"

#include "Settings.hpp"
#include "SettingWnd.hpp"

#include "Main.hpp"

#ifndef HOTKEYF_WIN
  #define HOTKEYF_WIN 0x10
#endif

//---------------------------------------------------------------------------//
//
// グローバル変数
//
//---------------------------------------------------------------------------//

HINSTANCE g_hInst { nullptr };

//---------------------------------------------------------------------------//

Settings*   settings { nullptr };
SettingWnd* wnd      { nullptr };

//---------------------------------------------------------------------------//

// コマンドID
enum CMD : INT32
{
    CMD_SWITCH, CMD_SETTINGS, CMD_CUONT
};

//---------------------------------------------------------------------------//

// プラグインの名前
LPCTSTR PLUGIN_NAME { TEXT("HotkeyCpp") };

// コマンドの数
DWORD COMMAND_COUNT { CMD_CUONT };

//---------------------------------------------------------------------------//

// コマンドの情報
PLUGIN_COMMAND_INFO g_cmd_info[] =
{
    {
        TEXT("Switch HotkeyCpp on/off"),                       // コマンド名（英名）
        TEXT("HotkeyCpp の ON/OFF"),                           // コマンド説明（日本語）                                                 
        CMD_SWITCH,                                            // コマンドID
        0,                                                     // Attr（未使用）
        -1,                                                    // ResTd(未使用）
        DISPMENU(dmSystemMenu | dmHotKeyMenu | dmMenuChecked), // DispMenu
        0,                                                     // TimerInterval[msec] 0で使用しない                                                   
        0                                                      // TimerCounter（未使用）                                                 
    },
    {
        TEXT("HotkeyCpp Settings"),            // コマンド名（英名）
        TEXT("HotkeyCpp 設定"),                // コマンド説明（日本語）
        CMD_SETTINGS,                          // コマンドID
        0,                                     // Attr（未使用）
        -1,                                    // ResTd(未使用）
        DISPMENU(dmSystemMenu | dmHotKeyMenu), // DispMenu
        0,                                     // TimerInterval[msec] 0で使用しない
        0                                      // TimerCounter（未使用）
    },                                         
};

//---------------------------------------------------------------------------//

// プラグインの情報
PLUGIN_INFO g_info =
{
    0,                   // プラグインI/F要求バージョン
    (LPTSTR)PLUGIN_NAME, // プラグインの名前（任意の文字が使用可能）
    nullptr,             // プラグインのファイル名（相対パス）
    ptAlwaysLoad,        // プラグインのタイプ
    0,                   // バージョン
    0,                   // バージョン
    COMMAND_COUNT,       // コマンド個数
    &g_cmd_info[0],      // コマンド
    0,                   // ロードにかかった時間（msec）
};

//---------------------------------------------------------------------------//

// TTBEvent_Init() の内部実装
BOOL WINAPI Init()
{
    // 設定オブジェクトの生成
    if ( nullptr == settings )
    {
        settings = new Settings;
    }

    // ウィンドウの生成
    if ( nullptr == wnd )
    {
        wnd = new SettingWnd;
    }

    WriteLog(elInfo, TEXT("%s: %s"), PLUGIN_NAME, TEXT("Successfully initialized"));
    return TRUE;
}

//---------------------------------------------------------------------------//

// TTBEvent_Unload() の内部実装
void WINAPI Unload()
{
    // ウィンドウの破棄
    if ( wnd )
    {
        delete wnd; wnd = nullptr;
    }

    // 設定オブジェクトの破棄
    if ( settings )
    {
        delete settings; settings = nullptr;
    }

    WriteLog(elInfo, TEXT("%s: %s"), PLUGIN_NAME, TEXT("Successfully uninitialized"));
}

//---------------------------------------------------------------------------//

// TTBEvent_Execute() の内部実装
BOOL WINAPI Execute(INT32 CmdId, HWND)
{
    if ( nullptr == settings || nullptr == wnd ) { return FALSE; }

    switch ( CmdId )
    {
        case CMD_SWITCH:
        {
            // 状態を切り替え
            settings->enable ^= true;

            // ホットキーを登録
            if ( settings->enable )
            {
                RegisterAllHotkeys(wnd->handle());
            }
            else
            {
                UnregisterAllHotkeys(wnd->handle());
            }

            // チェック状態を本体に通知
            TTBPlugin_SetMenuProperty
            (
                g_hPlugin, CMD_SWITCH, DISPMENU_CHECKED,
                settings->enable ? dmMenuChecked : dmUnchecked
            );

            return TRUE;
        }
        case CMD_SETTINGS:
        {
            // 設定ウィンドウを表示
            if ( wnd )
            {
                wnd->ToCenter();
                wnd->Show();
                ::SetForegroundWindow(wnd->handle());
            }
            return TRUE;
        }
        default:
        {
            return FALSE;
        }
    }
}

//---------------------------------------------------------------------------//

// TTBEvent_WindowsHook() の内部実装
void WINAPI Hook(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(Msg);    // ERASE ME
    UNREFERENCED_PARAMETER(wParam); // ERASE ME
    UNREFERENCED_PARAMETER(lParam); // ERASE ME
}

//---------------------------------------------------------------------------//

// ホットキーを登録する
void RegisterMyHotkey(HWND hwnd, INT32 id, const command& cmd)
{
    UINT fsModifiers { 0 };
    UINT vk          { 0 };

    // 修飾キー
    if ( HIBYTE(cmd.key) & HOTKEYF_ALT )
    {
        fsModifiers |= MOD_ALT;
    }
    if ( HIBYTE(cmd.key) & HOTKEYF_CONTROL )
    {
        fsModifiers |= MOD_CONTROL;
    }
    if ( HIBYTE(cmd.key) & HOTKEYF_SHIFT )
    {
        fsModifiers |= MOD_SHIFT;
    }
    if ( HIBYTE(cmd.key) & HOTKEYF_WIN )
    {
        fsModifiers |= MOD_WIN;
    }

    // キー
    vk = LOBYTE(cmd.key);

    // Windows に 登録
    ::RegisterHotKey(hwnd, id, fsModifiers, vk);
}

// ホットキーの登録を解除する
void UnregisterMyHotkey(HWND hwnd, INT32 id)
{
    ::UnregisterHotKey(hwnd, id);
}

//---------------------------------------------------------------------------//

// すべてのホットキーを登録する
void RegisterAllHotkeys(HWND hwnd)
{
    if ( nullptr == settings ) { return; }

    const auto& commands = settings->commands;

    INT32 id = 0;
    for ( auto&& cmd: commands )
    {
        RegisterMyHotkey(hwnd, id, cmd);
        ++id;
    }
}

// すべてのホットキーの登録を解除する
void UnregisterAllHotkeys(HWND hwnd)
{
    if ( nullptr == settings ) { return; }

    const auto& commands = settings->commands;

    INT32 id = 0;
    for ( auto&& cmd: commands )
    {
        cmd.key;

        UnregisterMyHotkey(hwnd, id);
        ++id;
    }
}

//---------------------------------------------------------------------------//
// CRT を使わないため new/delete を自前で実装
//---------------------------------------------------------------------------//

#if defined(_NODEFLIB)

void* __cdecl operator new(size_t size)
{
    return ::HeapAlloc(::GetProcessHeap(), 0, size);
}

void __cdecl operator delete(void* p)
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void __cdecl operator delete(void* p, size_t) // C++14
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void* __cdecl operator new[](size_t size)
{
    return ::HeapAlloc(::GetProcessHeap(), 0, size);
}

void __cdecl operator delete[](void* p)
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void __cdecl operator delete[](void* p, size_t) // C++14
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

// プログラムサイズを小さくするためにCRTを除外
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#pragma comment(linker, "/entry:DllMain")

#endif

//---------------------------------------------------------------------------//
//
// DLL エントリポイント
//
//---------------------------------------------------------------------------//

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{
    if ( fdwReason == DLL_PROCESS_ATTACH ) { g_hInst = hInstance; }
    return TRUE;
}

//---------------------------------------------------------------------------//

// Main.cpp