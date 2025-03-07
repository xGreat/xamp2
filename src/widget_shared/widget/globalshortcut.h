//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>

#include <qwindowdefs.h>

#include <base/memory.h>
#include <base/base.h>

class GlobalShortcut {
public:
    GlobalShortcut();

    XAMP_PIMPL(GlobalShortcut)

	bool registerShortcut(const WId wid, quint32 native_key, quint32 native_mods);

    bool unregisterShortcut(const WId wid, quint32 native_key, quint32 native_mods);

    quint32 nativeModifiers(Qt::KeyboardModifiers modifiers);

    quint32 nativeKeycode(Qt::Key key);
private:
    class GlobalShortcutImpl;
    AlignPtr<GlobalShortcutImpl> impl_;
};

#define qGlobalShortcut SharedSingleton<GlobalShortcut>::GetInstance()
