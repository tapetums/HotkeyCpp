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

//---------------------------------------------------------------------------//
// ユーティリティ関数
//---------------------------------------------------------------------------//

namespace
{
    inline void AdjustRect
    (
        HWND hwnd, INT32* w, INT32* h
    )
    {
        RECT rc{ 0, 0, *w, *h };
        const auto style   = (DWORD)::GetWindowLongPtr(hwnd, GWL_STYLE);
        const auto styleEx = (DWORD)::GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        const auto isChild = style & WS_CHILD;
        const BOOL hasMenu = (!isChild && ::GetMenu(hwnd)) ? TRUE : FALSE;

        ::AdjustWindowRectEx(&rc, style, hasMenu, styleEx);
        *w = rc.right  - rc.left;
        *h = rc.bottom - rc.top;
    }

    inline void GetRectForMonitorUnderCursor
    (
        RECT* rect
    )
    {
        POINT pt;
        ::GetCursorPos(&pt);
        const auto hMonitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

        MONITORINFOEX miex;
        miex.cbSize = (DWORD)sizeof(MONITORINFOEX);
        ::GetMonitorInfo(hMonitor, &miex);

        *rect = miex.rcMonitor;
    }
}

//---------------------------------------------------------------------------//
// クラス
//---------------------------------------------------------------------------//

class SettingWnd
{
private:
    LPCTSTR m_class_name { TEXT("SettingWnd") };
    HWND    m_hwnd       { nullptr };
    INT32   m_x          { 0 };
    INT32   m_y          { 0 };
    INT32   m_w          { 0 };
    INT32   m_h          { 0 };

public:
    SettingWnd()
    {
        Register(m_class_name);
        Create(PLUGIN_NAME, WS_OVERLAPPEDWINDOW, 0, nullptr, nullptr);
        Resize(320, 240);
    }

public:
    operator HWND() { return m_hwnd; }

public:
    static LRESULT CALLBACK WndMapProc
    (
        HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp
    )
    {
        SettingWnd* wnd;

        // UWndオブジェクトのポインタを取得
        if ( uMsg == WM_NCCREATE )
        {
            wnd = (SettingWnd*)((CREATESTRUCT*)lp)->lpCreateParams;

            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)wnd);
        }
        else
        {
            wnd = (SettingWnd*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        // まだマップされていない場合はデフォルトプロシージャに投げる
        if ( nullptr == wnd )
        {
            return ::DefWindowProc(hwnd, uMsg, wp, lp);
        }

        // メンバ変数に情報を保存
        switch ( uMsg )
        {
            case WM_CREATE:
            {
                wnd->m_hwnd = hwnd; // ウィンドウハンドル
                break;
            }
            case WM_MOVE:
            {
                wnd->m_x = GET_X_LPARAM(lp); // ウィンドウX座標
                wnd->m_y = GET_Y_LPARAM(lp); // ウィンドウY座標
                break;
            }
            case WM_SIZE:
            {
                wnd->m_w = LOWORD(lp); // ウィンドウ幅
                wnd->m_h = HIWORD(lp); // ウィンドウ高
                break;
            }
            default:
            {
                break;
            }
        }

        // ウィンドウプロシージャの呼び出し
        return wnd->WndProc(hwnd, uMsg, wp, lp);
    }

public:
    LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp)
    {
        if ( uMsg == WM_CLOSE )
        {
            Hide(); return 0;
        }
        else
        {
            return ::DefWindowProc(hwnd, uMsg, wp, lp);
        }
    }

public:
    ATOM Register
    (
        LPCTSTR lpszClassName
    )
    {
        WNDCLASSEX wcex
        {
            sizeof(WNDCLASSEX),
            CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
            WndMapProc,
            0, 0,
            ::GetModuleHandle(nullptr),
            nullptr,
            ::LoadCursor(nullptr, IDC_ARROW),
            nullptr,
            nullptr,
            lpszClassName,
            nullptr,
        };

        return ::RegisterClassEx(&wcex);
    }

    HWND Create
    (
        LPCTSTR lpszWindowName,
        DWORD   style,
        DWORD   styleEx,
        HWND    hwndParent,
        HMENU   hMenu
    )
    {
        if ( m_hwnd ) { return m_hwnd; } // 二重生成防止!

        const auto hwnd = ::CreateWindowEx
        (
            styleEx, m_class_name, lpszWindowName, style,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hwndParent, hMenu, ::GetModuleHandle(nullptr),
            reinterpret_cast<LPVOID>(this)
        );
        if ( nullptr == hwnd )
        {
            //ShowLastError(class_name);
        }
        else
        {
            ::UpdateWindow(hwnd);
        }

        return hwnd;
    }

    void Move
    (
        INT32 x, INT32 y
    )
    {
        ::SetWindowPos
        (
            m_hwnd, nullptr,
            x, y, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED
        );
    }

    void Resize
    (
        INT32 w, INT32 h
    )
    {
        AdjustRect(m_hwnd, &w, &h);

        ::SetWindowPos
        (
            m_hwnd, nullptr,
            0, 0, w, h,
            SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED
        );
    }

    void ToCenter()
    {
        RECT rc;
        INT32 mx, my, mw, mh;

        if ( const auto parent = GetParent() )
        {
            ::GetWindowRect(parent, &rc);
            mx = rc.left;
            my = rc.top;
            mw = rc.right  - rc.left;
            mh = rc.bottom - rc.top;
        }
        else
        {
            GetRectForMonitorUnderCursor(&rc);
            mx = rc.left;
            my = rc.top;
            mw = rc.right  - rc.left;
            mh = rc.bottom - rc.top;
        }

        ::GetClientRect(m_hwnd, &rc);
        const auto w = rc.right  - rc.left;
        const auto h = rc.bottom - rc.top;

        const auto x = (mw - w) / 2 + mx;
        const auto y = (mh - h) / 2 + my;

        return Move(x, y);
    }

    void Show()
    {
        ::ShowWindowAsync(m_hwnd, SW_SHOWNORMAL);
    }

    void Hide()
    {
        ::ShowWindowAsync(m_hwnd, SW_HIDE);
    }

    HWND GetParent() const noexcept
    {
        return (HWND)::GetWindowLongPtr(m_hwnd, GWLP_HWNDPARENT);
    }
};

//---------------------------------------------------------------------------//

// SettingWnd.hpp