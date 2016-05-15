//---------------------------------------------------------------------------//
//
// SettingWnd.cpp
//  設定ウィンドウ
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

#include <windows.h>
#include <windowsx.h>
#include <shobjidl.h>
#include <wrl.h>

#include "resource.h"

#if !defined(_UNICODE) && !defined(UNICOED)
  #include "include/Transcode.hpp"
#endif

#include "Utility.hpp"
#include "Settings.hpp"
#include "Main.hpp"

#include "SettingWnd.hpp"

#ifndef HOTKEYF_WIN
  #define HOTKEYF_WIN 0x10
#endif

//---------------------------------------------------------------------------//
// グローバル変数
//---------------------------------------------------------------------------//

PLUGIN_INFO** infos { nullptr };

//---------------------------------------------------------------------------//
// 前方宣言
//---------------------------------------------------------------------------//

class SettingWnd;

INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

const command* const GetCommandByIndex(INT32 index);
PLUGIN_INFO* GetInfoByFilename(LPCTSTR Filename);
INT32 GetCommandIndex   (const PLUGIN_INFO* info, INT32 CmdID);
void  GetHotkeyString   (INT32 key, TCHAR* buf, size_t buf_size);
void  GetCommandString  (LPCTSTR Filename, INT32 CmdID, TCHAR* buf, size_t buf_size);
void  GetKeynameString  (INT16 vk, TCHAR* buf, size_t buf_size);
void  CheckVKeyItem     (HWND hwnd, INT16 mod);
void  ShowVKeyString    (HWND hItem, INT16 vk);
void  SetVKey           (command* pcmd, INT32 delta);
void  SetCommandID      (command* pcmd, INT32 delta);
void  ShowVKeyValue     (HWND hwnd, command* pcmd);
void  ShowCommandCaption(HWND hwnd, command* pcmd);
bool  OpenFileDialog    (TCHAR* buf, size_t buf_size);

//---------------------------------------------------------------------------//
// SettingWnd
//---------------------------------------------------------------------------//

SettingWnd::SettingWnd()
{
    Register(PLUGIN_NAME);

    // フォントを生成
    font.create(16, TEXT("Meiryo UI"), FW_REGULAR);

    // ウィンドウを生成
    const auto hwnd = Create
    (
        PLUGIN_NAME, WS_CAPTION | WS_SYSMENU, 0, nullptr, nullptr
    );
    Resize(480, 320);
    SetFont(font);

    // 子コントロールを生成
    auto style = WS_VSCROLL | WS_HSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL;
    auto styleEx = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP;
    list_cmd.Create(style, styleEx, hwnd, CTRL::LIST_COMMAND);
    list_cmd.Bounds(4, 4, 472, 280);
    list_cmd.SetFont(font);
    list_cmd.InsertColumn(TEXT("keys"),    140, 0);
    list_cmd.InsertColumn(TEXT("command"), 280, 1);
    list_cmd.InsertColumn(TEXT("ID"),       34, 2);
    list_cmd.InsertColumn(TEXT("param"),     0, 3);

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

    // ホットキーを更新
    if ( settings && settings->enable )
    {
        RegisterAllHotkeys(hwnd);
    }
}

//---------------------------------------------------------------------------//

LRESULT CALLBACK SettingWnd::WndProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
    if ( uMsg == WM_DESTROY )
    {
        return OnDestory(hwnd);
    }
    else if ( uMsg == WM_CLOSE )
    {
        Hide(); return 0;
    }
    else if ( uMsg == WM_SHOWWINDOW )
    {
        return OnShown(hwnd, BOOL(wp));
    }
    else if ( uMsg == WM_NOTIFY )
    {
        return OnNotify(hwnd, LOWORD(wp), LPNMHDR(lp));
    }
    else if ( uMsg == WM_COMMAND )
    {
        return OnCommand(hwnd, LOWORD(wp));
    }
    else if ( uMsg == WM_HOTKEY )
    {
        return OnHotkey(hwnd, INT32(wp), LOWORD(lp), HIWORD(lp));
    }
    else
    {
        return ::DefWindowProc(hwnd, uMsg, wp, lp);
    }
}

//---------------------------------------------------------------------------//

LRESULT CALLBACK SettingWnd::OnDestory(HWND hwnd)
{
    if ( infos ) { TTBPlugin_FreePluginInfoArray(infos); }
    infos = nullptr;

    // ホットキーの登録を解除
    UnregisterAllHotkeys(hwnd);
    return 0;
}

//---------------------------------------------------------------------------//

LRESULT CALLBACK SettingWnd::OnShown(HWND, BOOL bShown)
{
    if ( ! bShown ) { return 0; }

    if ( infos ) { TTBPlugin_FreePluginInfoArray(infos); }
    infos = TTBPlugin_GetAllPluginInfo(); // リークに注意

    ClearCommandList();
    MakeCommandList();

    return 0;
}

//---------------------------------------------------------------------------//

LRESULT CALLBACK SettingWnd::OnNotify(HWND hwnd, INT32, NMHDR* pNMHdr)
{
    if ( pNMHdr->idFrom == CTRL::LIST_COMMAND && pNMHdr->code == NM_DBLCLK )
    {
        // リストをダブルクリックしたとき
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

//---------------------------------------------------------------------------//

LRESULT CALLBACK SettingWnd::OnCommand(HWND hwnd, INT16 wID)
{
    if ( wID == CTRL::BTN_ADD ) // 追加
    {
        AddItem(hwnd);
    }
    else if ( wID == CTRL::BTN_EDIT )// 編集
    {
        EditItem(hwnd, list_cmd.SelectedIndex());
    }
    else if ( wID == CTRL::BTN_DEL ) // 削除
    {
        DeleteItem(hwnd, list_cmd.SelectedIndex());
    }
    return 0;
}

//---------------------------------------------------------------------------//

LRESULT CALLBACK SettingWnd::OnHotkey
(
    HWND, INT32 idHotKey, WORD, WORD
)
{
    // リストからコマンドを検索
    auto pcmd = GetCommandByIndex(idHotKey);
    if ( nullptr == pcmd )
    {
        return 0;
    }

    // コマンドを実行
    const auto ret = TTBPlugin_ExecuteCommand(pcmd->filename, pcmd->id);
    if ( ret )
    {
        return 0; // OK
    }

    // 実行ファイルを実行
    ::ShellExecute
    (
        nullptr, TEXT("open"),
        pcmd->filename, pcmd->param, nullptr, SW_SHOW
    );

    return 0;
}

//---------------------------------------------------------------------------//

void SettingWnd::MakeCommandList()
{
    if ( nullptr == settings ) { return; }

    TCHAR buf [MAX_PATH];

    INT32 index = 0;
    const auto& commands = settings->commands;
    for ( auto&& cmd: commands )
    {
        GetHotkeyString(cmd.key, buf, MAX_PATH);
        list_cmd.InsertItem(buf, index);

        GetCommandString(cmd.filename, cmd.id, buf, MAX_PATH);
        list_cmd.SetItem(buf, index, 1);

        ::StringCchPrintf(buf, MAX_PATH, TEXT("%u"), cmd.id);
        list_cmd.SetItem(buf, index, 2);

        list_cmd.SetItem(cmd.param, index, 3);

        ++index;
    }
}

//---------------------------------------------------------------------------//

void SettingWnd::ClearCommandList()
{
    list_cmd.DeleteAllItems();
}

//---------------------------------------------------------------------------//

void SettingWnd::AddItem(HWND hwnd)
{
    if ( nullptr == settings ) { return; }

    INT_PTR ret;
    command cmd;

    while ( true ) // キー重複の時にやり直しができるよう 無限ループにしておく
    {
        // コマンド編集ダイアログを表示
        ret = ::DialogBoxParam
        (
            g_hInst, MAKEINTRESOURCE(IDD_DIALOG_EDIT),
            hwnd, DialogProc, LPARAM(&cmd)
        );
        if ( ret != IDOK )
        {
            return;
        }

        auto&& commands = settings->commands;

        // キーが重複していないかチェック
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
                continue; // もう一回編集画面を出す
            }
            else
            {
                break; // 登録を諦める
            }
        }

        // コマンドを登録
        UnregisterAllHotkeys(hwnd);
        commands.push_back(std::move(cmd));

        ClearCommandList();
        MakeCommandList();

        if ( settings->enable )
        {
            RegisterAllHotkeys(hwnd);
        }

        // 設定ファイルに保存
        settings->save();

        // 無限ループから脱出
        break;
    }
}

//---------------------------------------------------------------------------//

void SettingWnd::EditItem(HWND hwnd, INT32 index)
{
    if ( nullptr == settings ) { return; }
    if ( index < 0 )           { return; }

    auto&& commands = settings->commands;

    // リストからインデックスで検索
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
        return; // ここには来ないはず…
    }

    // コマンド編集ダイアログを表示
    auto cmd = *pcmd; // 今の項目のコピーをとる
    const auto ret = ::DialogBoxParam
    (
        g_hInst, MAKEINTRESOURCE(IDD_DIALOG_EDIT),
        hwnd, DialogProc, LPARAM(&cmd)
    );
    if ( ret == IDOK )
    {
        *pcmd = cmd; // 書き戻す

        // 設定ファイルに保存
        settings->save();
    }

    // リストを再表示
    UnregisterAllHotkeys(hwnd);

    ClearCommandList();
    MakeCommandList();
    if ( settings->enable )
    {
        RegisterAllHotkeys(hwnd);
    }
}

//---------------------------------------------------------------------------//

void SettingWnd::DeleteItem(HWND hwnd, INT32 index)
{
    if ( nullptr == settings ) { return; }
    if ( index < 0 )           { return; }

    const auto ret = ::MessageBox
    (
        nullptr, TEXT("削除しますか？"), PLUGIN_NAME, MB_YESNO
    );
    if ( ret != IDYES )
    {
        return;
    }

    auto&& commands = settings->commands;

    INT32 i = 0;
    for ( auto it = commands.begin(); it != commands.end(); ++it, ++i )
    {
        if ( i == index )
        {
            // リストから削除
            UnregisterAllHotkeys(hwnd);
            commands.erase(it);

            // リストを再表示
            ClearCommandList();
            MakeCommandList();

            if ( settings->enable )
            {
                RegisterAllHotkeys(hwnd);
            }

            // 設定ファイルに保存
            settings->save();

            break;
        }
    }
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
            HWND hItem;
            TCHAR buf [MAX_PATH];

            // 情報を取得
            // グローバル変数なことに注意!! これはモーダルダイアログです
            pcmd  = (command*)lp;

            // キーの値を表示
            ShowVKeyValue(hwnd, pcmd);

            // Ctrl や Win キーなどの設定状態を表示
            CheckVKeyItem(hwnd, HIBYTE(pcmd->key));

            // コマンドのパスを表示
            hItem = ::GetDlgItem(hwnd, IDC_COMBO1);
            ::SetWindowText(hItem, pcmd->filename);

            // プラグインの一覧をコンボボックスに登録
            if ( nullptr == infos )
            {
                infos = TTBPlugin_GetAllPluginInfo(); // リークに注意
            }
            for ( size_t index = 0; ; ++index )
            {
                const auto info = infos[index];
                if ( nullptr == info ) { break; }

                ::SendMessage(hItem, CB_ADDSTRING, 0, (LPARAM)info->Filename);
            }

            // コマンド引数を表示
            hItem = ::GetDlgItem(hwnd, IDC_EDIT3);
            ::SetWindowText(hItem, pcmd->param);

            // コマンドIDを表示
            ::StringCchPrintf(buf, MAX_PATH, TEXT("%i"), pcmd->id);
            hItem = ::GetDlgItem(hwnd, IDC_EDIT4);
            ::SetWindowText(hItem, buf);

            // スピンコントロールをコマンドIDのエディットコントロールと紐付け
            hItem = ::GetDlgItem(hwnd, IDC_SPIN2);
            ::SendMessage
            (
                hItem, UDM_SETRANGE, 0, MAKELPARAM(UD_MAXVAL, 0)
            );
            ::SendMessage
            (
                hItem, UDM_SETBUDDY, (WPARAM)::GetDlgItem(hwnd, IDC_EDIT4), 0
            );

            // コマンドのキャプションを表示
            ShowCommandCaption(hwnd, pcmd);

            hItem = ::GetDlgItem(hwnd, IDC_EDIT1);
            ::SetWindowSubclass(hItem, SubclassProc, 0, DWORD_PTR(hwnd));

            return TRUE;
        }
        case WM_COMMAND:
        {
            const auto wID = LOWORD(wp);

            if ( wID == IDOK )
            {
                // 終了して変更を反映
                ::EndDialog(hwnd, IDOK);
            }
            else if ( wID == IDC_FILE )
            {
                // ファイル選択ダイアログを表示
                OpenFileDialog(pcmd->filename, MAX_PATH);

                const auto hItem = ::GetDlgItem(hwnd, IDC_COMBO1);
                ::SetWindowText(hItem, pcmd->filename);

                ShowCommandCaption(hwnd, pcmd);
            }
            else if ( wID == IDC_COMBO1 )
            {
                // コマンド名の変更を反映
                const auto hItem = ::GetDlgItem(hwnd, IDC_COMBO1);

                const auto code = HIWORD(wp);
                if ( code == CBN_SELCHANGE )
                {
                    const auto index = ::SendMessage(hItem, CB_GETCURSEL, 0, 0);
                    if ( index < 0 ) { return TRUE; }

                    ::SendMessage
                    (
                        hItem, CB_GETLBTEXT, index, (LPARAM)pcmd->filename
                    );
                    ShowCommandCaption(hwnd, pcmd);
                }
                else if ( code == CBN_EDITCHANGE )
                {
                    ::GetWindowText(hItem, pcmd->filename, MAX_PATH);
                    ShowCommandCaption(hwnd, pcmd);
                }
            }
            else if ( wID == IDC_EDIT3 )
            {
                // コマンド引数の変更を反映
                const auto code = HIWORD(wp);
                if ( code != EN_CHANGE ) { return TRUE; }

                const auto hItem = ::GetDlgItem(hwnd, IDC_EDIT3);
                ::GetWindowText(hItem, pcmd->param, MAX_PATH);
            }
            else if ( wID == IDC_CHECK1 )
            {
                pcmd->key ^= (HOTKEYF_CONTROL << 8); // Ctrl
            }
            else if ( wID == IDC_CHECK2 )
            {
                pcmd->key ^= (HOTKEYF_SHIFT << 8); // Shift
            }
            else if ( wID == IDC_CHECK3 )
            {
                pcmd->key ^= (HOTKEYF_ALT << 8); // Alt
            }
            else if ( wID == IDC_CHECK4 )
            {
                pcmd->key ^= (HOTKEYF_WIN << 8); // Win
            }

            // コマンドID は スピンコントロールから自動的に反映される

            return TRUE;
        }
        case WM_NOTIFY:
        {
            // スピンコントロールの変更を反映
            const auto pNMhdr = LPNMHDR(lp);
            if ( pNMhdr->code != UDN_DELTAPOS ) { return TRUE; }

            const auto pNMud = LPNMUPDOWN(lp);
            if ( pNMud->hdr.hwndFrom == ::GetDlgItem(hwnd, IDC_SPIN1) )
            {
                // キーの値を表示
                SetVKey(pcmd, pNMud->iDelta);
                ShowVKeyValue(hwnd, pcmd);
            }
            else if ( pNMud->hdr.hwndFrom == ::GetDlgItem(hwnd, IDC_SPIN2) )
            {
                // コマンドのキャプションを表示
                SetCommandID(pcmd, pNMud->iDelta);
                ShowCommandCaption(hwnd, pcmd);
            }

            return TRUE;
        }
        case WM_KEYDOWN:
        {
            const auto vk = LOBYTE(wp);
            pcmd->key &= 0xFF00;
            pcmd->key |= vk;

            // キーの名前を表示
            const auto hItem = ::GetDlgItem(hwnd, IDC_EDIT1);
            ShowVKeyString(hItem, vk);

            // キーの値を表示
            ShowVKeyValue(hwnd, pcmd);

            return TRUE;
        }
        case WM_CLOSE:
        {
            // 終了して変更を破棄
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

LRESULT CALLBACK SubclassProc
(
    HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp,
    UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData
)
{
    if ( uMsg == WM_KEYDOWN )
    {
        return ::SendMessage(HWND(dwRefData), uMsg, wp, lp);
    }
    else
    {
        return ::DefSubclassProc(hwnd, uMsg, wp, lp);
    }
}

//---------------------------------------------------------------------------//
// ユーティリティ関数
//---------------------------------------------------------------------------//

const command* const GetCommandByIndex(INT32 index)
{
    if ( nullptr == settings ) { return nullptr; }

    const auto& commands = settings->commands;

    INT32 i = 0;
    for ( auto&& cmd: commands )
    {
        if ( i == index )
        {
            return &cmd;
        }
        ++i;
    }

    return nullptr;
}

//---------------------------------------------------------------------------//

PLUGIN_INFO* GetInfoByFilename
(
    LPCTSTR Filename
)
{
    PLUGIN_INFO* info = nullptr;

    if ( nullptr == infos )
    {
        infos = TTBPlugin_GetAllPluginInfo();
    }

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

INT32 GetCommandIndex
(
    const PLUGIN_INFO* info, INT32 CmdID
)
{
    if ( nullptr == info ) { return -1; }

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

void GetKeynameString(INT16 vk, TCHAR* buf, size_t buf_size)
{
    constexpr const TCHAR* const KEY_NAME[] =
    {
        // 00-0F
        TEXT("(なし)"),       TEXT("Left Click"), TEXT("Right Click"), TEXT("Cancel"),
        TEXT("Center Click"), TEXT("X1 Click"),   TEXT("X2 Click"),    TEXT("未定義"),
        TEXT("Backspace"),    TEXT("Tab"),        TEXT("(予約)"),      TEXT("(予約)"),
        TEXT("Clear"),        TEXT("Enter"),      TEXT("未定義"),      TEXT("未定義"),

        // 10-1F
        TEXT("Shift"),     TEXT("Ctrl"),      TEXT("Alt"),    TEXT("Pause"),
        TEXT("Caps Lock"), TEXT("かな"),      TEXT("未定義"), TEXT(""),
        TEXT(""),          TEXT("漢字"),      TEXT("未定義"), TEXT("Esc"),
        TEXT("変換"),      TEXT("無変換"),    TEXT(""),       TEXT("IME"),

        // 20-2F
        TEXT("Space"),        TEXT("Page Up"), TEXT("Page Down"), TEXT("End"),
        TEXT("Home"),         TEXT("←"),      TEXT("↑"),        TEXT("→"),
        TEXT("↓"),           TEXT("Select"),  TEXT("Print"),     TEXT("Execute"),
        TEXT("Print Screen"), TEXT("Insert"),  TEXT("Delete"),    TEXT("Help"),

        // 30-3F
        TEXT("0"), TEXT("1"), TEXT("2"), TEXT("3"),
        TEXT("4"), TEXT("5"), TEXT("6"), TEXT("7"),
        TEXT("8"), TEXT("9"), TEXT("未定義"), TEXT("未定義"),
        TEXT("未定義"), TEXT("未定義"), TEXT("未定義"), TEXT("未定義"),

        // 40-4F
        TEXT("未定義"), TEXT("A"), TEXT("B"), TEXT("C"),
        TEXT("D"),      TEXT("E"), TEXT("F"), TEXT("G"),
        TEXT("H"),      TEXT("I"), TEXT("J"), TEXT("K"),
        TEXT("L"),      TEXT("M"), TEXT("N"), TEXT("O"),

        // 50-5F
        TEXT("P"), TEXT("Q"), TEXT("R"), TEXT("S"),
        TEXT("T"), TEXT("U"), TEXT("V"), TEXT("W"),
        TEXT("X"), TEXT("Y"), TEXT("Z"), TEXT("Left Win"),
        TEXT("Right Win"), TEXT("Application"), TEXT("(予約)"), TEXT("Sleep"),

        // 60-6F
        TEXT("Num 0"), TEXT("Num 1"), TEXT("Num 2"), TEXT("Num 3"),
        TEXT("Num 4"), TEXT("Num 5"), TEXT("Num 6"), TEXT("Num 7"),
        TEXT("Num 8"), TEXT("Num 9"), TEXT("Num *"), TEXT("Num +"),
        TEXT("Num Enter"), TEXT("Num -"), TEXT("Num ."), TEXT("Num /"),

        // 70-7F
        TEXT("F1"),  TEXT("F2"),  TEXT("F3"),  TEXT("F4"),
        TEXT("F5"),  TEXT("F6"),  TEXT("F7"),  TEXT("F8"),
        TEXT("F9"),  TEXT("F10"), TEXT("F11"), TEXT("F12"),
        TEXT("F13"), TEXT("F14"), TEXT("F15"), TEXT("F16"),

        // 80-8F
        TEXT("F17"), TEXT("F18"), TEXT("F19"), TEXT("F20"),
        TEXT("F21"), TEXT("F22"), TEXT("F23"), TEXT("F24"),
        TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"),
        TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"),

        // 90-9F
        TEXT("Num Lock"), TEXT("Scroll Lock"), TEXT("(なし)"), TEXT("(なし)"),
        TEXT("(なし)"),   TEXT("(なし)"),      TEXT("(なし)"), TEXT("(なし)"),
        TEXT("(なし)"),   TEXT("(なし)"),      TEXT("(なし)"), TEXT("(なし)"),
        TEXT("(なし)"),   TEXT("(なし)"),      TEXT("(なし)"), TEXT("(なし)"),

        // A0-AF
        TEXT("Left Shift"), TEXT("Right Shift"), TEXT("Left Ctrl"), TEXT("Right Ctrl"),
        TEXT("Left Alt"),   TEXT("Right Alt"),   TEXT("戻る"),      TEXT("進む"),
        TEXT("更新"),       TEXT("中止"),        TEXT("検索"),      TEXT("お気に入り"),
        TEXT("HomePage"),   TEXT("Mute"),        TEXT("Vol Down"),  TEXT("Vol Up"),

        // B0-BF
        TEXT("Next Track"), TEXT("Prev Track"), TEXT("Media Stop"), TEXT("Start/Stop"),
        TEXT("Mail"),       TEXT("Media"),      TEXT("App 1"),      TEXT("App 2"),
        TEXT("(予約)"),     TEXT("(予約)"),     TEXT(":"),          TEXT(";"),
        TEXT(","),          TEXT("-"),          TEXT("."),          TEXT("/"),

        // C0-CF
        TEXT("@"),      TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"),
        TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"),
        TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"),
        TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"),

        // D0-DF
        TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"),
        TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"), TEXT("(予約)"),
        TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"), TEXT("["),
      #if defined (_UNICODE) || defined(UNICODE)
        TEXT("¥"),      TEXT("]"),      TEXT("^"),      TEXT("(なし)"),
      #else
        TEXT("\\"),     TEXT("]"),      TEXT("^"),      TEXT("(なし)"),
      #endif

        // E0-EF
        TEXT("(予約)"), TEXT("-"),      TEXT("\\"),     TEXT("(なし)"),
        TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"),
        TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"),
        TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"), TEXT("(なし)"),

        // F0-FF
        TEXT("Caps Lock"), TEXT("(なし)"),    TEXT("ひらがな"), TEXT("半角"),
        TEXT("全角"),      TEXT("(なし)"),    TEXT("Attn"),     TEXT("CrSel"),
        TEXT("ExSel"),     TEXT("Erase EOF"), TEXT("Play"),     TEXT("Zoom"),
        TEXT("(予約)"),    TEXT("PA1"),       TEXT("(なし)"),   TEXT("(なし)"),
    };

    ::StringCchCopy(buf, buf_size, KEY_NAME[vk]);
}

//---------------------------------------------------------------------------//

void GetHotkeyString
(
    INT32 key, TCHAR* buf, size_t buf_size
)
{
    TCHAR mod_txt [MAX_PATH];
    TCHAR vk_txt  [MAX_PATH];
    mod_txt[0] = '\0';
    vk_txt[0]  = '\0';

    // 合成キーの文字列を取得
    const auto mod = HIBYTE(key);
    if ( mod & HOTKEYF_CONTROL ) // Ctrl
    {
        ::StringCchCat(mod_txt, 64, TEXT("Ctrl + "));
    }
    if ( mod & HOTKEYF_SHIFT ) // Shift
    {
        ::StringCchCat(mod_txt, 64, TEXT("Shift + "));
    }
    if ( mod & HOTKEYF_ALT ) // Alt
    {
        ::StringCchCat(mod_txt, 64, TEXT("Alt + "));
    }
    if ( mod & HOTKEYF_WIN ) // Win
    {
        ::StringCchCat(mod_txt, 64, TEXT("Win + "));
    }

    // 仮想キーの文字列を取得
    const auto vk = LOBYTE(key);
    GetKeynameString(vk, vk_txt, MAX_PATH);
    ::StringCchCat(mod_txt, MAX_PATH, vk_txt);

    // 合成キー名と仮想キー名を連結
    if ( mod_txt[0] == '\0' )
    {
        ::StringCchCopy(buf, buf_size, TEXT("(なし)"));
    }
    else
    {
        ::StringCchCopy(buf, buf_size, mod_txt);
    }
}

//---------------------------------------------------------------------------//

void GetCommandString
(
    LPCTSTR Filename, INT32 CmdID, TCHAR* buf, size_t buf_size
)
{
    const auto info = GetInfoByFilename(Filename);
    if ( nullptr == info )
    {
        ::StringCchCopy(buf, buf_size, Filename);
    }
    else
    {
        const auto cmd_idx = GetCommandIndex(info, CmdID);
        if ( cmd_idx < 0 )
        {
            ::StringCchPrintf
            (
                buf, buf_size, TEXT("%s - ?"), info ? info->Name : TEXT("?")
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
}

//---------------------------------------------------------------------------//

void CheckVKeyItem
(
    HWND hwnd, INT16 mod
)
{
    HWND hItem;

    // Ctrl
    if ( mod & HOTKEYF_CONTROL )
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK1);
        ::SendMessage(hItem, BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK1);
        ::SendMessage(hItem, BM_SETCHECK, BST_UNCHECKED, 0);
    }

    // Shift
    if ( mod & HOTKEYF_SHIFT )
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK2);
        ::SendMessage(hItem, BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK2);
        ::SendMessage(hItem, BM_SETCHECK, BST_UNCHECKED, 0);
    }

    // Alt
    if ( mod & HOTKEYF_ALT )
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK3);
        ::SendMessage(hItem, BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK3);
        ::SendMessage(hItem, BM_SETCHECK, BST_UNCHECKED, 0);
    }

    // Win
    if ( mod & HOTKEYF_WIN )
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK4);
        ::SendMessage(hItem, BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
        hItem = ::GetDlgItem(hwnd, IDC_CHECK4);
        ::SendMessage(hItem, BM_SETCHECK, BST_UNCHECKED, 0);
    }
}

//---------------------------------------------------------------------------//

void ShowVKeyString
(
    HWND hItem, INT16 vk
)
{
    TCHAR vk_txt[MAX_PATH];
    vk_txt[0] = '\0';

    GetKeynameString(vk, vk_txt, MAX_PATH);

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

void SetVKey
(
    command* pcmd, INT32 delta
)
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

void SetCommandID
(
    command* pcmd, INT32 delta
)
{
    if ( delta > 0 )
    {
        ++pcmd->id;
        if ( pcmd->id > UD_MAXVAL )
        {
            pcmd->id = 0;
        }
    }
    else if ( delta < 0 )
    {
        --pcmd->id;
        if ( pcmd->id < 0 )
        {
            pcmd->id = UD_MAXVAL;
        }
    }
}

//---------------------------------------------------------------------------//

void ShowVKeyValue
(
    HWND hwnd, command* pcmd
)
{
    HWND hItem;
    TCHAR buf [MAX_PATH];

    hItem = ::GetDlgItem(hwnd, IDC_EDIT1);
    ShowVKeyString(hItem, LOBYTE(pcmd->key));

    ::StringCchPrintf(buf, MAX_PATH, TEXT("0x%02X"), LOBYTE(pcmd->key));
    hItem = ::GetDlgItem(hwnd, IDC_EDIT6);
    ::SetWindowText(hItem, buf);
}

//---------------------------------------------------------------------------//

void ShowCommandCaption
(
    HWND hwnd, command* pcmd
)
{
    TCHAR buf [MAX_PATH];

    GetCommandString(pcmd->filename, pcmd->id, buf, MAX_PATH);

    const auto hItem = ::GetDlgItem(hwnd, IDC_EDIT5);
    ::SetWindowText(hItem, buf);
}

//---------------------------------------------------------------------------//

bool OpenFileDialog
(
    TCHAR* buf, size_t buf_size
)
{
    using namespace Microsoft::WRL;

    HRESULT hr;

    // 念のため COM を初期化
    hr = ::CoInitialize(nullptr);
    if ( FAILED(hr) )
    {
        return false;
    }

    // COM オブジェクトを取得
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
        WriteLog(elError, TEXT("%s: Could not get COM object"), PLUGIN_NAME);
        return false;
    }

    // ダイアログの表示
    hr = fd->Show(nullptr);
    if ( FAILED(hr) )
    {
        return false;
    }

    // 選択項目を取得
    ComPtr<IShellItem> item;
    hr = fd->GetResult(&item);
    if ( FAILED(hr) )
    {
        WriteLog(elError, TEXT("%s: Could not get IFileDialog result"), PLUGIN_NAME);
        return false;
    }

    // バッファにコピー
    LPWSTR pszFilePath;
    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    if ( FAILED(hr) )
    {
        WriteLog(elError, TEXT("%s: Could not get file name "), PLUGIN_NAME);
        return false;
    }

  #if defined(_UNICODE) || defined(UNICODE)
    ::StringCchCopyW(buf, buf_size, pszFilePath);
  #else
    char ansi_buf [MAX_PATH];
    tapetums::toMBCS(pszFilePath, ansi_buf, MAX_PATH);
    ::StringCchCopyA(buf, buf_size, ansi_buf);
  #endif
    ::CoTaskMemFree(pszFilePath);

    return true;
}

//---------------------------------------------------------------------------//

// SettingWnd.cpp