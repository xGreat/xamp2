//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include "xampplayer.h"
#include "thememanager.h"

#include <widget/driveinfo.h>

#if defined(Q_OS_WIN)
class QScreen;
namespace win32 {
    class WinTaskbar;
}
#endif

class XWindow final : public IXWindow {
public:
    XWindow();

	virtual ~XWindow() override;

    void setShortcut(const QKeySequence& shortcut) override;

    void setContentWidget(IXPlayerControlFrame *content_widget);

    void setTaskbarProgress(int32_t percent) override;

    void resetTaskbarProgress() override;

    void setTaskbarPlayingResume() override;

    void setTaskbarPlayerPaused() override;

    void setTaskbarPlayerPlaying() override;

    void setTaskbarPlayerStop() override;

    void setTitleBarAction(QFrame* title_bar) override;

    void restoreGeometry();

    void initMaximumState() override;

    void updateMaximumState() override;

    void readDriveInfo();

    void drivesRemoved(char driver_letter);

    void systemThemeChanged(ThemeColor theme_color);

    void shortcutsPressed(uint16_t native_key, uint16_t native_mods);

protected:
    bool eventFilter(QObject* object, QEvent* event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void dropEvent(QDropEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent * event) override;

    void mouseMoveEvent(QMouseEvent * event) override;

    void mouseDoubleClickEvent(QMouseEvent* event) override;

    void changeEvent(QEvent * event) override;

    void closeEvent(QCloseEvent* event) override;

    void saveGeometry() override;

    bool nativeEvent(const QByteArray& event_type, void* message, long* result) override;
private:
    void addSystemMenu(QWidget *widget);

    void focusInEvent(QFocusEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

    void updateScreenNumber();

    QPoint last_pos_;
    QRect last_rect_;
    uint32_t screen_number_;

#if defined(Q_OS_WIN)
	void showEvent(QShowEvent* event) override;

    QScreen* current_screen_;
    QScopedPointer<win32::WinTaskbar> taskbar_;
    QMap<QString, DriveInfo> exist_drives_;
#endif
    QMap<QPair<quint32, quint32>, QKeySequence>  shortcuts_;
    IXPlayerControlFrame *player_control_frame_;
};
