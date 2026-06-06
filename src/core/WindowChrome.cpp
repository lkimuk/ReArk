#include "core/WindowChrome.h"

#include <QWindow>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

WindowChrome::WindowChrome(QObject* parent)
    : QObject(parent)
{
}

bool WindowChrome::isMaximized(QWindow* window) const
{
    if (!window) {
        return false;
    }

#ifdef Q_OS_WIN
    auto* hwnd = reinterpret_cast<HWND>(window->winId());
    if (hwnd) {
        return IsZoomed(hwnd) != FALSE;
    }
#endif

    return window->visibility() == QWindow::Maximized
        || window->windowStates().testFlag(Qt::WindowMaximized);
}

void WindowChrome::showSystemMenu(QWindow* window, const QPointF& globalPosition)
{
    if (!window) {
        return;
    }

#ifdef Q_OS_WIN
    auto* hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) {
        return;
    }

    HMENU systemMenu = GetSystemMenu(hwnd, FALSE);
    if (!systemMenu) {
        return;
    }

    const bool maximized = IsZoomed(hwnd) != FALSE;
    EnableMenuItem(systemMenu, SC_RESTORE, MF_BYCOMMAND | (maximized ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(systemMenu, SC_MOVE, MF_BYCOMMAND | (maximized ? MF_GRAYED : MF_ENABLED));
    EnableMenuItem(systemMenu, SC_SIZE, MF_BYCOMMAND | (maximized ? MF_GRAYED : MF_ENABLED));
    EnableMenuItem(systemMenu, SC_MAXIMIZE, MF_BYCOMMAND | (maximized ? MF_GRAYED : MF_ENABLED));

    const auto position = globalPosition.toPoint();
    const UINT command = TrackPopupMenu(systemMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
        position.x(),
        position.y(),
        0,
        hwnd,
        nullptr);
    if (command != 0) {
        PostMessage(hwnd, WM_SYSCOMMAND, command, 0);
    }
#else
    Q_UNUSED(globalPosition)
#endif
}
