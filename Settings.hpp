#pragma once

//---------------------------------------------------------------------------//
//
// Settings.hpp
//  設定ファイル 管理クラス
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

#include <list>

#include <windows.h>
#include <strsafe.h>

#include "Plugin.hpp"

//---------------------------------------------------------------------------//
// Global Variables
//---------------------------------------------------------------------------//

// WinMain.cpp で宣言
extern HINSTANCE g_hInst;

//---------------------------------------------------------------------------//
// Structures
//---------------------------------------------------------------------------//

struct command
{
    INT32 key;
    INT32 id;
    TCHAR filename [MAX_PATH];
    TCHAR param    [MAX_PATH];

    command()
    {
        key = 0x41;
        id  = 0;
        filename[0] = param[0] = L'\0';
    }

    command(const command& lhs)             { copy(lhs); }
    command& operator =(const command& lhs) { copy(lhs); return *this; }

    command(command&& rhs)             noexcept { copy(rhs); }
    command& operator =(command&& rhs) noexcept { copy(rhs); return *this; }

    ~command() = default;

    void copy(const command& lhs)
    {
        key = lhs.key;
        id  = lhs.id;
        ::StringCchCopy(filename, MAX_PATH, lhs.filename);
        ::StringCchCopy(param,    MAX_PATH, lhs.param);
    }
};

//---------------------------------------------------------------------------//
// Class
//---------------------------------------------------------------------------//

class Settings
{
public:
    bool enable;
    std::list<command> commands;

public:
    Settings();
    ~Settings();
};

extern Settings* settings;

//---------------------------------------------------------------------------//
// Methods
//---------------------------------------------------------------------------//

inline Settings::Settings()
{
    TCHAR path [MAX_PATH];
    TCHAR name [MAX_PATH];

    // iniファイル名取得
    const auto len = ::GetModuleFileName(g_hInst, path, MAX_PATH);
    path[len - 3] = 'i';
    path[len - 2] = 'n';
    path[len - 1] = 'i';

    // パラメータの取得
    enable = ::GetPrivateProfileInt
    (
       TEXT("Setting"),TEXT("enable"), -1, path
    )
    ? true : false;

    // ホットキー設定の読み込み
    for ( size_t idx = 0; ; ++idx )
    {
        command cmd;

        ::StringCchPrintf(name, MAX_PATH, TEXT("%03u_key"), idx);
        cmd.key = ::GetPrivateProfileInt(TEXT("Hotkey"), name, -1, path);
        if ( cmd.key == -1 )
        {
            break;
        }

        ::StringCchPrintf(name, MAX_PATH, TEXT("%03u_id"), idx);
        cmd.id = ::GetPrivateProfileInt(TEXT("Hotkey"), name, 0, path);

        ::StringCchPrintf(name, MAX_PATH, TEXT("%03u_filename"), idx);
        ::GetPrivateProfileString
        (
            TEXT("Hotkey"), name, TEXT(""), cmd.filename, MAX_PATH, path
        );

        ::StringCchPrintf(name, MAX_PATH, TEXT("%03u_param"), idx);
        ::GetPrivateProfileString
        (
            TEXT("Hotkey"), name, TEXT(""), cmd.param, MAX_PATH, path
        );

        bool already_reged = false;
        for ( const auto& cmd_reged: commands )
        {
            if ( cmd_reged.key == cmd.key )
            {
                already_reged = true; break;
            }
        }
        if ( ! already_reged )
        {
            commands.push_back(std::move(cmd));
        }
    }
}

//---------------------------------------------------------------------------//

inline Settings::~Settings()
{
    TCHAR path [MAX_PATH];
    TCHAR name [MAX_PATH];
    TCHAR buf  [16];

    // iniファイル名取得
    const auto len = ::GetModuleFileName(g_hInst, path, MAX_PATH);
    path[len - 3] = 'i';
    path[len - 2] = 'n';
    path[len - 1] = 'i';

    // パラメータの書き出し
    ::StringCchPrintf(buf, MAX_PATH, TEXT("%i"), enable);
    ::WritePrivateProfileString(TEXT("Setting"),TEXT("enable"), buf, path);

    //セクションの削除
    ::WritePrivateProfileString(TEXT("Hotkey"), nullptr, nullptr, path);

    // ホットキー設定の書き出し
    size_t idx = 0;
    for ( auto&& cmd: commands )
    {
        ::StringCchPrintf(name, MAX_PATH, TEXT("%03u_key"), idx);
        ::StringCchPrintf(buf, MAX_PATH, TEXT("%u"), cmd.key);
        ::WritePrivateProfileString(TEXT("Hotkey"), name, buf, path);

        ::StringCchPrintf(name, MAX_PATH, TEXT("%03u_id"), idx);
        ::StringCchPrintf(buf, MAX_PATH, TEXT("%i"), cmd.id);
        ::WritePrivateProfileString(TEXT("Hotkey"), name, buf, path);

        ::StringCchPrintf(name, MAX_PATH, TEXT("%03u_filename"), idx);
        ::WritePrivateProfileString(TEXT("Hotkey"), name, cmd.filename, path);

        ::StringCchPrintf(name, MAX_PATH, TEXT("%03u_param"), idx);
        ::WritePrivateProfileString(TEXT("Hotkey"), name, cmd.param, path);

        ++idx;
    }
}

//---------------------------------------------------------------------------//

// Settings.hpp