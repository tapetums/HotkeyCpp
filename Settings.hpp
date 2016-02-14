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

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib") // PathRenameExtension

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
    UINT32 key;
    INT32  id;
    TCHAR  filename [MAX_PATH];
    TCHAR  param    [MAX_PATH];

    command()
    {
        key = 0;
        id = -1;
        filename[0] = param[0] = L'\0';
    }

    command(const command&) = delete;

    command(command&& rhs) noexcept
    {
        key = rhs.key;
        id  = rhs.id;
        ::StringCchCopy(filename, MAX_PATH, rhs.filename);
        ::StringCchCopy(param,    MAX_PATH, rhs.param);
    }
};

//---------------------------------------------------------------------------//
// Class
//---------------------------------------------------------------------------//

class settings
{
public:
    static settings& get() { static settings s; return s; }

public:
    bool enable;
    std::list<command> commands;

private:
    settings();
    ~settings();
};

//---------------------------------------------------------------------------//
// Methods
//---------------------------------------------------------------------------//

inline settings::settings()
{
    TCHAR path [MAX_PATH];

    // iniファイル名取得
    ::GetModuleFileName(g_hInst, path, MAX_PATH);
    ::PathRenameExtension(path,TEXT(".ini"));

    // パラメータの取得
    enable = ::GetPrivateProfileInt
    (
       TEXT("Setting"),TEXT("enable"), -1, path
    )
    ? true : false;

    TCHAR num [16];
    for ( size_t idx = 0; ; ++idx )
    {
        ::wsprintf(num,TEXT("%u"), idx);

        command cmd;
        cmd.key = ::GetPrivateProfileInt(num,TEXT("key"), -1, path);
        if ( cmd.key == -1 )
        {
            break;
        }

        cmd.id = ::GetPrivateProfileInt(num,TEXT("id"), 0, path);
        ::GetPrivateProfileString
        (
            num,TEXT("filename"),TEXT(""), cmd.filename, MAX_PATH, path
        );
        ::GetPrivateProfileString
        (
            num,TEXT("param"),TEXT(""), cmd.param, MAX_PATH, path
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

inline settings::~settings()
{
    TCHAR path [MAX_PATH];
    TCHAR buf  [16];

    // iniファイル名取得
    ::GetModuleFileName(g_hInst, path, MAX_PATH);
    ::PathRenameExtension(path,TEXT(".ini"));

    // パラメータの書き出し
    ::wsprintf(buf,TEXT("%i"), enable);
    ::WritePrivateProfileString(TEXT("Setting"),TEXT("enable"), buf, path);

    size_t idx = 0;
    TCHAR num [16];
    for ( auto&& cmd: commands )
    {
        ::wsprintf(num,TEXT("%u"), idx);

        ::wsprintf(buf,TEXT("%u"), cmd.key);
        ::WritePrivateProfileString(num,TEXT("key"), buf, path);
        WriteLog(elDebug, TEXT("%s: %i"), PLUGIN_NAME, cmd.id);

        ::wsprintf(buf,TEXT("%i"), cmd.id);
        ::WritePrivateProfileString(num,TEXT("id"), buf, path);

        ::WritePrivateProfileString(num,TEXT("filename"), cmd.filename, path);
        ::WritePrivateProfileString(num,TEXT("param"), cmd.param, path);

        ++idx;
    }
}

//---------------------------------------------------------------------------//

// Settings.hpp