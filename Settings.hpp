#pragma once

//---------------------------------------------------------------------------//
//
// Settings.hpp
//  設定ファイル 管理クラス
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

#include <array>

#include <windows.h>

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib") // PathRenameExtension

#include "list.hpp"

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
};

//---------------------------------------------------------------------------//
// Class
//---------------------------------------------------------------------------//

class Settings
{
public:
    bool enable;
    tapetums::list<command> commands;

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
    std::array<wchar_t, MAX_PATH> path;

    // iniファイル名取得
    ::GetModuleFileNameW(g_hInst, path.data(), (DWORD)path.size());
    ::PathRenameExtensionW(path.data(), L".ini");

    // パラメータの取得
    enable = ::GetPrivateProfileIntW
    (
        L"Setting", L"enable", -1, path.data()
    )
    ? true : false;

    std::array<wchar_t, 16> num;
    for ( size_t idx = 0; ; ++idx )
    {
        ::wsprintfW(num.data(), L"%u", idx);

        command cmd;
        cmd.key = ::GetPrivateProfileIntW(num.data(), L"key", -1, path.data());
        if ( cmd.key == -1 )
        {
            break;
        }

        cmd.id = ::GetPrivateProfileIntW(num.data(), L"id", 0, path.data());
        ::GetPrivateProfileStringW
        (
            num.data(), L"filename", nullptr, cmd.filename, MAX_PATH, path.data()
        );
        ::GetPrivateProfileStringW
        (
            num.data(), L"param", nullptr, cmd.param, MAX_PATH, path.data()
        );

        commands.push_back(std::move(cmd));
    }
}

//---------------------------------------------------------------------------//

inline Settings::~Settings()
{
    std::array<wchar_t, MAX_PATH> path;
    std::array<wchar_t, 16> buf;

    // iniファイル名取得
    ::GetModuleFileNameW(g_hInst, path.data(), (DWORD)path.size());
    ::PathRenameExtensionW(path.data(), L".ini");

    // パラメータの書き出し
    ::wsprintfW(buf.data(), L"%i", enable);
    ::WritePrivateProfileStringW(L"Setting", L"enable", buf.data(), path.data());

    size_t idx = 0;
    std::array<wchar_t, 16> num;
    for ( auto&& cmd: commands )
    {
        ::wsprintfW(num.data(), L"%u", idx);

        ::wsprintfW(num.data(), L"%u", cmd.key);
        ::WritePrivateProfileStringW(buf.data(), L"key", buf.data(), path.data());

        ::wsprintfW(num.data(), L"%i", cmd.id);
        ::WritePrivateProfileStringW(buf.data(), L"id", buf.data(), path.data());

        ::WritePrivateProfileStringW(buf.data(), L"id", cmd.filename, path.data());
        ::WritePrivateProfileStringW(buf.data(), L"id", cmd.param, path.data());

        ++idx;
    }
}

//---------------------------------------------------------------------------//

// Settings.hpp