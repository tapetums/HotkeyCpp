#pragma once

//---------------------------------------------------------------------------//
//
// SettingWnd.hpp
//  設定ウィンドウ
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

#include <windows.h>

#include "include/UWnd.hpp"
#include "include/CtrlWnd.hpp"
#include "include/Font.hpp"

#include "Plugin.hpp"

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
    SettingWnd();

public:
    LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp) override;

private:
    LRESULT CALLBACK OnDestory(HWND hwnd);
    LRESULT CALLBACK OnNotify (HWND hwnd, INT32 wID, NMHDR* pNMHdr);
    LRESULT CALLBACK OnCommand(HWND hwnd, INT16 wID);
    LRESULT CALLBACK OnHotkey (HWND hwnd, INT32 idHotKey, WORD fsModifiers, WORD vk);

private:
    void MakeCommandList();
    void ClearCommandList();

    void AddItem   (HWND hwnd);
    void EditItem  (HWND hwnd, INT32 index);
    void DeleteItem(HWND hwnd, INT32 index);
};

//---------------------------------------------------------------------------//

// SettingWnd.hpp