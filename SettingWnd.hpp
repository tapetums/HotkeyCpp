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
#include <shobjidl.h>
#include <wrl.h>

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

PLUGIN_INFO* GetInfoByFilename(PLUGIN_INFO* infos[], LPCTSTR Filename);
INT32        GetCommandIndex(const PLUGIN_INFO* info, INT32 CmdID);

void GetHotkeyString(INT32 key, TCHAR* buf, size_t buf_size);
void GetCommandString(PLUGIN_INFO* infos[], LPCTSTR Filename, INT32 CmdID, TCHAR* buf, size_t buf_size);
void CheckVKeyItem(HWND hwnd, INT16 mod);
void ShowVKeyName(HWND hItem, INT16 vk);

void SetVKey        (command* pcmd, INT32 delta);
void SetCommandID   (command* pcmd, INT32 delta);
void ShowVKeyName   (HWND hwnd, command* pcmd);
void ShowCommandName(HWND hwnd, command* pcmd);
bool OpenFileDialog (TCHAR* buf, size_t buf_size);

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
        else if ( uMsg == WM_NOTIFY )
        {
            return OnNotify(hwnd, LOWORD(wp), LPNMHDR(lp));
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
            AddItem(hwnd);
        }
        else if ( wID == CTRL::BTN_EDIT )
        {
            EditItem(hwnd, list_cmd.SelectedIndex());
        }
        else if ( wID == CTRL::BTN_DEL )
        {
            DeleteItem(list_cmd.SelectedIndex());
        }
        return 0;
    }

    LRESULT OnNotify(HWND hwnd, INT32, NMHDR* pNMHdr)
    {
        if ( pNMHdr->idFrom == CTRL::LIST_COMMAND && pNMHdr->code == NM_DBLCLK )
        {
            const auto index = list_cmd.SelectedIndex();
            if ( index < 0 )
            {
                AddItem(hwnd);
            }
            else
            {
                EditItem(hwnd, index);
            }
        }

        return 0;
    }

private:
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

            GetCommandString(infos, cmd.filename, cmd.id, buf, MAX_PATH);
            list_cmd.SetItem(buf, index, 1);

            ::StringCchPrintf(buf, MAX_PATH, TEXT("%u"), cmd.id);
            list_cmd.SetItem(buf, index, 2);

            list_cmd.SetItem(cmd.param, index, 3);

            ++index;
        }

        TTBPlugin_FreePluginInfoArray(infos);
    }

    void ClearCommandList()
    {
        list_cmd.DeleteAllItems();
    }

    void AddItem(HWND hwnd)
    {
        INT_PTR ret;
        command cmd;

        while ( true )
        {
            ret = ::DialogBoxParam
            (
                g_hInst, MAKEINTRESOURCE(IDD_DIALOG_EDIT),
                hwnd, DialogProc, LPARAM(&cmd)
            );
            if ( ret != IDOK )
            {
                return;
            }

            auto&& commands = settings::get().commands;

            bool already_reged = false;
            for ( const auto& cmd_reged: commands )
            {
                if ( cmd_reged.key == cmd.key )
                {
                    already_reged = true; break;
                }
            }
            if ( already_reged )
            {
                ret = ::MessageBox
                (
                    nullptr, TEXT("同じキーが登録されています"),
                    PLUGIN_NAME, MB_RETRYCANCEL
                );
                if ( ret == IDRETRY )
                {
                    continue;
                }
                else
                {
                    return;
                }
            }

            commands.push_back(std::move(cmd));
            ClearCommandList();
            MakeCommandList();

            return;
        }
    }

    void EditItem(HWND hwnd, INT32 index)
    {
        if ( index < 0 ) { return; }

        auto&& commands = settings::get().commands;

        INT32    i    { 0 };
        command* pcmd { nullptr };
        for ( auto&& cmd: commands )
        {
            if ( i == index )
            {
                pcmd = &cmd; break;
            }
            ++i;
        }
        if ( nullptr == pcmd )
        {
            return;
        }

        auto cmd = *pcmd;
        const auto ret = ::DialogBoxParam
        (
            g_hInst, MAKEINTRESOURCE(IDD_DIALOG_EDIT),
            hwnd, DialogProc, LPARAM(&cmd)
        );
        if ( ret == IDOK )
        {
            *pcmd = cmd;
        }

        ClearCommandList();
        MakeCommandList();
    }

    void DeleteItem(INT32 index)
    {
        if ( index < 0 ) { return; }

        auto&& commands = settings::get().commands;

        INT32 i = 0;
        for ( auto it = commands.begin(); it != commands.end(); ++it, ++i )
        {
            if ( i == index )
            {
                commands.erase(it); break;
            }
        }

        ClearCommandList();
        MakeCommandList();
    }
};

//---------------------------------------------------------------------------//
// ユーティリティ関数
//---------------------------------------------------------------------------//

PLUGIN_INFO* GetInfoByFilename(PLUGIN_INFO* infos[], LPCTSTR Filename)
{
    PLUGIN_INFO* info = nullptr;

    for ( size_t i = 0; ; ++i )
    {
        info = infos[i];
        if ( nullptr == info )
        {
            break;
        }
        if ( 0 == lstrcmp(Filename, info->Filename) )
        {
            break;
        }
    }

    return info;
}

//---------------------------------------------------------------------------//

INT32 GetCommandIndex(const PLUGIN_INFO* info, INT32 CmdID)
{
    const auto count = info->CommandCount;
    for ( DWORD idx = 0; idx < count; ++idx )
    {
        if ( CmdID == info->Commands[idx].CommandID )
        {
            return idx;
        }
    }
    return -1;
}

//---------------------------------------------------------------------------//

void GetHotkeyString(INT32 key, TCHAR* buf, size_t buf_size)
{
    TCHAR mod_txt[MAX_PATH];
    TCHAR vk_txt[MAX_PATH];
    mod_txt[0] = '\0';
    vk_txt[0] = '\0';

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
    ::GetKeyNameText((sc << 16), vk_txt, MAX_PATH);
    ::StringCchCat(mod_txt, MAX_PATH, vk_txt);

    if ( mod_txt[0] == '\0' )
    {
        ::StringCchCopy(buf, buf_size, TEXT("なし"));
    }
    else
    {
        ::StringCchCopy(buf, buf_size, mod_txt);
    }
}

//---------------------------------------------------------------------------//

void GetCommandString
(
    PLUGIN_INFO* infos[],
    LPCTSTR      Filename,
    INT32        CmdID,
    TCHAR*       buf,
    size_t       buf_size
)
{
    const auto info = GetInfoByFilename(infos, Filename);
    if ( nullptr == info )
    {
        ::StringCchCopy(buf, buf_size, Filename);
        return;
    }

    const auto cmd_idx = GetCommandIndex(info, CmdID);
    if ( cmd_idx < 0 )
    {
        ::StringCchPrintf
        (
            buf, buf_size, TEXT("%s - ?"), info->Name
        );
    }
    else
    {
        ::StringCchPrintf
        (
            buf, buf_size, TEXT("%s - %s"),
            info->Name, info->Commands[cmd_idx].Caption
        );
    }
}

//---------------------------------------------------------------------------//

void CheckVKeyItem(HWND hwnd, INT16 mod)
{
    HWND hItem;

    if ( mod & HOTKEYF_CONTROL )
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK1);
        ::SendMessage(hItem, BM_SETCHECK, BST_CHECKED, 0);
    }
    if ( mod & HOTKEYF_SHIFT )
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK2);
        ::SendMessage(hItem, BM_SETCHECK, BST_CHECKED, 0);
    }
    if ( mod & HOTKEYF_ALT )
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK3);
        ::SendMessage(hItem, BM_SETCHECK, BST_CHECKED, 0);
    }
    if ( mod & HOTKEYF_WIN )
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK4);
        ::SendMessage(hItem, BM_SETCHECK, BST_CHECKED, 0);
    }
}

//---------------------------------------------------------------------------//

void ShowVKeyName(HWND hItem, INT16 vk)
{
    TCHAR vk_txt[MAX_PATH];
    vk_txt[0] = '\0';

    const auto sc = ::MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    ::GetKeyNameText((sc << 16), vk_txt, MAX_PATH);

    if ( vk_txt[0] == '\0' )
    {
        ::SetWindowText(hItem, TEXT("(なし)"));
    }
    else
    {
        ::SetWindowText(hItem, vk_txt);
    }
}

//---------------------------------------------------------------------------//

void SetVKey(command* pcmd, INT32 delta)
{
    auto vk = LOBYTE(pcmd->key);
    if ( delta > 0 )
    {
        ++vk;
        if ( vk > 0xFF )
        {
            vk = 0x00;
        }
    }
    else if ( delta < 0 )
    {
        --vk;
        if ( vk < 0x00 )
        {
            vk = 0xFF;
        }
    }
    pcmd->key &= 0xFF00;
    pcmd->key |= vk;
}

//---------------------------------------------------------------------------//

void SetCommandID(command* pcmd, INT32 delta)
{
    if ( delta > 0 )
    {
        ++pcmd->id;
        if ( pcmd->id > 255 )
        {
            pcmd->id = 0;
        }
    }
    else if ( delta < 0 )
    {
        --pcmd->id;
        if ( pcmd->id < 0 )
        {
            pcmd->id = 255;
        }
    }
}

//---------------------------------------------------------------------------//

void ShowVKeyName(HWND hwnd, command* pcmd)
{
    HWND hItem;
    TCHAR buf [MAX_PATH];

    hItem = ::GetDlgItem(hwnd, IDC_EDIT1);
    ShowVKeyName(hItem, LOBYTE(pcmd->key));

    ::StringCchPrintf(buf, MAX_PATH, TEXT("0x%02X"), LOBYTE(pcmd->key));
    hItem = ::GetDlgItem(hwnd, IDC_EDIT6);
    ::SetWindowText(hItem, buf);
}

//---------------------------------------------------------------------------//

void ShowCommandName(HWND hwnd, command* pcmd)
{
    TCHAR buf [MAX_PATH];

    const auto& infos = TTBPlugin_GetAllPluginInfo();
    GetCommandString(infos, pcmd->filename, pcmd->id, buf, MAX_PATH);

    const auto hItem = ::GetDlgItem(hwnd, IDC_EDIT5);
    ::SetWindowText(hItem, buf);

    TTBPlugin_FreePluginInfoArray(infos);
}

bool OpenFileDialog(TCHAR* buf, size_t buf_size)
{
    using namespace Microsoft::WRL;

    HRESULT hr;

    // 念のため
    hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if ( FAILED(hr) )
    {
        return false;
    }

    ComPtr<IFileDialog> fd;
    hr = ::CoCreateInstance
    (
        CLSID_FileOpenDialog, 
        nullptr, 
        CLSCTX_INPROC_SERVER, 
        IID_PPV_ARGS(&fd)
    );
    if ( FAILED(hr) )
    {
        return false;
    }

    hr = fd->Show(nullptr);
    if ( FAILED(hr) )
    {
        return false;
    }

    ComPtr<IShellItem> item;
    hr = fd->GetResult(&item);
    if ( FAILED(hr) )
    {
        return false;
    }

    LPWSTR pszFilePath;
    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    ::StringCchCopy(buf, buf_size, pszFilePath);
    ::CoTaskMemFree(pszFilePath);

    return true;
}

//---------------------------------------------------------------------------//
// コールバック関数
//---------------------------------------------------------------------------//

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
    static command* pcmd;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            pcmd = (command*)lp;
            HWND hItem;
            TCHAR buf [MAX_PATH];

            ShowVKeyName(hwnd, pcmd);

            CheckVKeyItem(hwnd, HIBYTE(pcmd->key));

            hItem = ::GetDlgItem(hwnd, IDC_COMBO1);
            ::SetWindowText(hItem, pcmd->filename);

            auto infos = TTBPlugin_GetAllPluginInfo();
            for ( size_t index = 0; ; ++index )
            {
                const auto info = infos[index];
                if ( nullptr == info ) { break; }

                SendMessage(hItem , CB_ADDSTRING , 0, (LPARAM)info->Filename);
            }
            TTBPlugin_FreePluginInfoArray(infos);

            hItem = ::GetDlgItem(hwnd, IDC_EDIT3);
            ::SetWindowText(hItem, pcmd->param);

            ::StringCchPrintf(buf, MAX_PATH, TEXT("%i"), pcmd->id);
            hItem = ::GetDlgItem(hwnd, IDC_EDIT4);
            ::SetWindowText(hItem, buf);

            hItem = ::GetDlgItem(hwnd, IDC_SPIN2);
            ::SendMessage
            (
                hItem, UDM_SETRANGE, 0, MAKELPARAM(255, 0)
            );
            ::SendMessage
            (
                hItem, UDM_SETBUDDY, (WPARAM)::GetDlgItem(hwnd, IDC_EDIT4), 0
            );

            ShowCommandName(hwnd, pcmd);

            ::SetFocus(::GetDlgItem(hwnd, IDC_EDIT1));

            return TRUE;
        }
        case WM_COMMAND:
        {
            const auto wID = LOWORD(wp);

            if ( wID == IDOK )
            {
                ::EndDialog(hwnd, IDOK);
            }
            else if ( wID == IDC_FILE )
            {
                OpenFileDialog(pcmd->filename, MAX_PATH);

                const auto hItem = ::GetDlgItem(hwnd, IDC_COMBO1);
                ::SetWindowText(hItem, pcmd->filename);

                ShowCommandName(hwnd, pcmd);
            }
            else if ( wID == IDC_CMD )
            {
                ::MessageBox(nullptr, TEXT("DUMMY"), TEXT("IDC_CMD"), MB_OK);
            }
            else if ( wID == IDC_COMBO1 )
            {
                const auto hItem = ::GetDlgItem(hwnd, IDC_COMBO1);

                const auto code = HIWORD(wp);
                if ( code == CBN_SELCHANGE )
                {
                    const auto index = ::SendMessage(hItem, CB_GETCURSEL, 0, 0);
                    if ( index >= 0 )
                    {
                        ::SendMessage
                        (
                            hItem, CB_GETLBTEXT, index, (LPARAM)pcmd->filename
                        );
                        ShowCommandName(hwnd, pcmd);
                    }
                }
                else if ( code == CBN_EDITCHANGE )
                {
                    ::GetWindowText(hItem, pcmd->filename, MAX_PATH);
                    ShowCommandName(hwnd, pcmd);
                }
            }
            else if ( wID == IDC_EDIT3 )
            {
                const auto code = HIWORD(wp);
                if ( code != EN_CHANGE ) { return TRUE; }

                const auto hItem = ::GetDlgItem(hwnd, IDC_EDIT3);
                ::GetWindowText(hItem, pcmd->param, MAX_PATH);
            }
            else if ( wID == IDC_CHECK1 )
            {
                const auto checked = ::SendMessage
                (
                    ::GetDlgItem(hwnd, IDC_CHECK1), BM_GETCHECK, 0, 0
                );
                pcmd->key ^= (HOTKEYF_CONTROL << 8);
            }
            else if ( wID == IDC_CHECK2 )
            {
                const auto checked = ::SendMessage
                (
                    ::GetDlgItem(hwnd, IDC_CHECK1), BM_GETCHECK, 0, 0
                );
                pcmd->key ^= (HOTKEYF_SHIFT << 8);
            }
            else if ( wID == IDC_CHECK3 )
            {
                const auto checked = ::SendMessage
                (
                    ::GetDlgItem(hwnd, IDC_CHECK1), BM_GETCHECK, 0, 0
                );
                pcmd->key ^= (HOTKEYF_ALT << 8);
            }
            else if ( wID == IDC_CHECK4 )
            {
                const auto checked = ::SendMessage
                (
                    ::GetDlgItem(hwnd, IDC_CHECK1), BM_GETCHECK, 0, 0
                );
                pcmd->key ^= (HOTKEYF_WIN << 8);
            }

            return TRUE;
        }
        case WM_NOTIFY:
        {
            const auto pNMhdr = LPNMHDR(lp);
            if ( pNMhdr->code != UDN_DELTAPOS ) { return TRUE; }

            const auto pNMud = LPNMUPDOWN(lp);
            if ( pNMud->hdr.hwndFrom == ::GetDlgItem(hwnd, IDC_SPIN1) )
            {
                SetVKey(pcmd, pNMud->iDelta);
                ShowVKeyName(hwnd, pcmd);
            }
            else if ( pNMud->hdr.hwndFrom == ::GetDlgItem(hwnd, IDC_SPIN2) )
            {
                SetCommandID(pcmd, pNMud->iDelta);
                ShowCommandName(hwnd, pcmd);
            }

            return TRUE;
        }
        case WM_CLOSE:
        {
            ::EndDialog(hwnd, IDCANCEL);
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