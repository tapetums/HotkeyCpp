#pragma once

//---------------------------------------------------------------------------//
//
// SettingWnd.hpp
//  設定ウィンドウ
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

#include <windows.h>
#include <windowsx.h>

#include "resource.h"

#include "include/UWnd.hpp"
#include "include/CtrlWnd.hpp"
#include "include/Font.hpp"

#ifndef HOTKEYF_WIN
  #define HOTKEYF_WIN 0x10
#endif

//---------------------------------------------------------------------------//
// 前方宣言
//---------------------------------------------------------------------------//

INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);

//---------------------------------------------------------------------------//
// クラス
//---------------------------------------------------------------------------//

class SettingWnd : public tapetums::UWnd
{
    using super = tapetums::UWnd;

private:
    tapetums::Font    font;
    tapetums::ListWnd list_cmd;
    tapetums::BtnWnd  btn_add;
    tapetums::BtnWnd  btn_edit;
    tapetums::BtnWnd  btn_del;

public:
    struct CTRL
    {
        enum : INT16 { LIST_COMMAND, BTN_ADD, BTN_EDIT, BTN_DEL, };
    };

public:
    SettingWnd()
    {
        font.create(16, TEXT("Meiryo UI"), FW_REGULAR);

        const auto hwnd = Create
        (
            PLUGIN_NAME, WS_CAPTION | WS_SYSMENU, 0, nullptr, nullptr
        );
        Resize(480, 320);
        SetFont(font);

        auto style = WS_VSCROLL | WS_HSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL;
        auto styleEx = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP;
        list_cmd.Create(style, styleEx, hwnd, CTRL::LIST_COMMAND);
        list_cmd.Bounds(4, 4, 472, 280);
        list_cmd.SetFont(font);
        list_cmd.InsertColumn(TEXT("keys"),    140, 0);
        list_cmd.InsertColumn(TEXT("command"), 180, 1);
        list_cmd.InsertColumn(TEXT("ID"),       34, 2);
        list_cmd.InsertColumn(TEXT("param"),   100, 3);

        btn_add. Create(BS_PUSHBUTTON, hwnd, CTRL::BTN_ADD);
        btn_edit.Create(BS_PUSHBUTTON, hwnd, CTRL::BTN_EDIT);
        btn_del. Create(BS_PUSHBUTTON, hwnd, CTRL::BTN_DEL);

        btn_add. Bounds(360 - 60 + 24, 320 - 36, 120, 24);
        btn_edit.Bounds(240 - 60 +  0, 320 - 36, 120, 24);
        btn_del. Bounds(120 - 60 - 24, 320 - 36, 120, 24);

        btn_add. SetFont(font);
        btn_edit.SetFont(font);
        btn_del. SetFont(font);

        btn_add. SetText(TEXT("追加"));
        btn_edit.SetText(TEXT("編集"));
        btn_del. SetText(TEXT("削除"));

        MakeCommandList();
    }

public:
    LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp) override
    {
        if ( uMsg == WM_CREATE )
        {
            return OnCreate(hwnd);
        }
        else if ( uMsg == WM_CLOSE )
        {
            Hide(); return 0;
        }
        else if ( uMsg == WM_COMMAND )
        {
            return OnCommand(hwnd, LOWORD(wp));
        }
        else
        {
            return ::DefWindowProc(hwnd, uMsg, wp, lp);
        }
    }

private:
    LRESULT OnCreate(HWND)
    {
        return 0;
    }

    LRESULT OnCommand(HWND hwnd, INT16 wID)
    {
        if ( wID == CTRL::BTN_ADD )
        {
        }
        else if ( wID == CTRL::BTN_EDIT )
        {
            ::DialogBoxParam
            (
                g_hInst, MAKEINTRESOURCE(IDD_DIALOG_EDIT),
                hwnd, DialogProc, 0
            );
        }
        else if ( wID == CTRL::BTN_DEL )
        {
            DeleteItem(list_cmd.SelectedIndex());
        }
        return 0;
    }

    void MakeCommandList()
    {
        TCHAR buf [MAX_PATH];

        const auto infos = TTBPlugin_GetAllPluginInfo();
        if ( nullptr == infos )
        {
            return;
        }

        INT32 index = 0;
        const auto& commands = settings::get().commands;
        for ( auto&& cmd: commands )
        {
            GetHotkeyString(cmd.key, buf, MAX_PATH);
            list_cmd.InsertItem(buf, index);

            for ( size_t i = 0; ; ++i )
            {
                auto info = infos[i];
                if ( nullptr == info )
                {
                    break;
                }
                if ( 0 == lstrcmp(cmd.filename, info->Filename) )
                {
                    list_cmd.SetItem(info->Name, index, 1);
                    break;
                }
            }

            ::wsprintf(buf,TEXT("%u"), cmd.id);
            list_cmd.SetItem(buf, index, 2);

            list_cmd.SetItem(cmd.param, index, 3);

            ++index;
        }

        TTBPlugin_FreePluginInfoArray(infos);
    }

    void GetHotkeyString(INT32 key, TCHAR* buf, size_t buf_size)
    {
        TCHAR mod_txt[MAX_PATH];
        mod_txt[0] = '\0';

        const auto mod = HIBYTE(key);
        if ( mod & HOTKEYF_CONTROL )
        {
            ::StringCchCat(mod_txt, 64, TEXT("Ctrl + "));
        }
        if ( mod & HOTKEYF_SHIFT )
        {
            ::StringCchCat(mod_txt, 64, TEXT("Shift + "));
        }
        if ( mod & HOTKEYF_ALT )
        {
            ::StringCchCat(mod_txt, 64, TEXT("Alt + "));
        }
        if ( mod & HOTKEYF_WIN )
        {
            ::StringCchCat(mod_txt, 64, TEXT("Win + "));
        }

        const auto vk  = LOBYTE(key);
        const auto sc = ::MapVirtualKey(vk, MAPVK_VK_TO_VSC);

        TCHAR vk_txt[MAX_PATH];
        ::GetKeyNameText((sc << 16), vk_txt, MAX_PATH);

        ::StringCchCat(mod_txt, MAX_PATH, vk_txt);
        ::StringCchCopy(buf, buf_size, mod_txt); 
    }

    void DeleteItem(INT32 index)
    {
        if ( index < 0 ) { return; }
        ::MessageBox(nullptr, TEXT("BTN_DEL"), TEXT(""), MB_OK);
    }
};

//---------------------------------------------------------------------------//
// コールバック関数
//---------------------------------------------------------------------------//

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM, LPARAM)
{
    switch ( uMsg )
    {
        case WM_CLOSE:
        {
            ::EndDialog(hwnd, IDOK);
            return TRUE;
        }
        default:
        {
            return FALSE;
        }
    }
}

//---------------------------------------------------------------------------//

// SettingWnd.hpp