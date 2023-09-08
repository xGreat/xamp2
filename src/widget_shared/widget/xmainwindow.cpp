#include <widget/xmainwindow.h>

#include <thememanager.h>

#include <FramelessHelper/Widgets/framelesswidgetshelper.h>
#include <base/logger_impl.h>

#include <widget/ui_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/actionmap.h>
#include <widget/globalshortcut.h>

#include <QStorageInfo>
#include <QApplication>
#include <QLayout>
#include <QPainter>
#include <QPainterPath>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QTimer>

#if defined(Q_OS_WIN)
#include <widget/win32/wintaskbar.h>
#include <base/platfrom_handle.h>
#include <windowsx.h>
#include <Dbt.h>
#else
#include <Carbon/Carbon.h>
#include <widget/osx/osx.h>
#endif

XMainWindow::XMainWindow()
    : IXMainWindow()
#if defined(Q_OS_WIN)
	, screen_number_(1)
#endif
	, content_widget_(nullptr) {    
    setObjectName(qTEXT("framelessWindow"));
}

void XMainWindow::SetShortcut(const QKeySequence& shortcut) {
    constexpr auto all_mods =
        Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
    const auto xkey_code = static_cast<uint>(shortcut.isEmpty() ? 0 : shortcut[0]);

    const uint key_code = QChar::toLower(xkey_code & ~all_mods);

    const auto key = static_cast<Qt::Key>(key_code);
    const auto mods = Qt::KeyboardModifiers(xkey_code & all_mods);

    const auto native_key = qGlobalShortcut.NativeKeycode(key);
    const auto native_mods = qGlobalShortcut.NativeModifiers(mods);

    if (qGlobalShortcut.RegisterShortcut(winId(), native_key, native_mods)) {
        shortcuts_.insert(qMakePair(native_key, native_mods), shortcut);
    }
}

void XMainWindow::SetContentWidget(IXFrame *content_widget) {
    content_widget_ = content_widget;
    if (content_widget_ != nullptr) {
        auto* default_layout = new QVBoxLayout(this);
        default_layout->addWidget(content_widget_);
        default_layout->setContentsMargins(0, 0, 0, 0);
        setLayout(default_layout);
    }

    setAcceptDrops(true);
    ReadDriveInfo();
    installEventFilter(this);
}

// QScopedPointer require default destructor.
XMainWindow::~XMainWindow() {
    XAMP_LOG_DEBUG("XMainWindow destory!");
}

void XMainWindow::EnsureInitTaskbar() {
    if (!task_bar_) {
        task_bar_.reset(new win32::WinTaskbar(this, content_widget_));
    }
}

void XMainWindow::SaveGeometry() {
    AppSettings::SetValue(kAppSettingGeometry, saveGeometry());
    AppSettings::SetValue(kAppSettingWindowState, isMaximized());
    AppSettings::SetValue(kAppSettingScreenNumber, screen_number_);
    XAMP_LOG_INFO("Screen number: {}", screen_number_);
}

void XMainWindow::SystemThemeChanged(ThemeColor theme_color) {
    if (!content_widget_) {
        return;
    }
    emit qTheme.CurrentThemeChanged(theme_color);
}

void XMainWindow::SetTaskbarProgress(const int32_t percent) {
#if defined(Q_OS_WIN)
    EnsureInitTaskbar();
    task_bar_->SetTaskbarProgress(percent);
#else
    (void)percent;
#endif
}

void XMainWindow::ResetTaskbarProgress() {
#if defined(Q_OS_WIN)
    EnsureInitTaskbar();
    task_bar_->ResetTaskbarProgress();
#endif
}

void XMainWindow::SetTaskbarPlayingResume() {
#if defined(Q_OS_WIN)
    EnsureInitTaskbar();
    task_bar_->SetTaskbarPlayingResume();
#endif
}

void XMainWindow::SetTaskbarPlayerPaused() {
#if defined(Q_OS_WIN)
    EnsureInitTaskbar();
    task_bar_->SetTaskbarPlayerPaused();
#endif
}

void XMainWindow::SetTaskbarPlayerPlaying() {
#if defined(Q_OS_WIN)
    EnsureInitTaskbar();
    task_bar_->SetTaskbarPlayerPlaying();
#endif
}

void XMainWindow::SetTaskbarPlayerStop() {
#if defined(Q_OS_WIN)
    EnsureInitTaskbar();
    task_bar_->SetTaskbarPlayerStop();
#endif
}

void XMainWindow::RestoreGeometry() {
#if defined(Q_OS_WIN)
    if (AppSettings::contains(kAppSettingWindowState)) {
        if (AppSettings::GetValue(kAppSettingWindowState).toBool()) {
            showMaximized();
            return;
        }
    }

    if (AppSettings::contains(kAppSettingGeometry)) {
        screen_number_ = AppSettings::GetValue(kAppSettingScreenNumber).toUInt();
        if (screen_number_ != 1) {
            CenterDesktop(this);
        } else {
            restoreGeometry(AppSettings::GetValue(kAppSettingGeometry).toByteArray());
        }
    }
    else {
        CenterDesktop(this);
    }    
#endif
}

bool XMainWindow::eventFilter(QObject * object, QEvent * event) {
    if (!content_widget_) {
        return QWidget::eventFilter(object, event);
    }

    if (event->type() == QEvent::KeyPress) {
        const auto* key_event = dynamic_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_Delete) {
            content_widget_->DeleteKeyPress();
        }
    } else {
        if (event->type() == QEvent::FocusIn) {
            focusInEvent(dynamic_cast<QFocusEvent*>(event));
        } else if (event->type() == QEvent::FocusOut) {
            focusOutEvent(dynamic_cast<QFocusEvent*>(event));
        }
    }
    return QWidget::eventFilter(object, event);
}

void XMainWindow::focusInEvent(QFocusEvent* event) {
    if (!content_widget_) {
        return;
    }
    content_widget_->FocusIn();
}

void XMainWindow::focusOutEvent(QFocusEvent* event) {
    if (!content_widget_) {
        return;
    }
    content_widget_->FocusOut();
}

void XMainWindow::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}

void XMainWindow::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void XMainWindow::dragLeaveEvent(QDragLeaveEvent* event) {
    event->accept();
}

void XMainWindow::dropEvent(QDropEvent* event) {
    if (!content_widget_) {
        return;
    }

    const auto* mime_data = event->mimeData();

    if (mime_data->hasUrls()) {
        for (auto const& url : mime_data->urls()) {
            content_widget_->AddDropFileItem(url);
        }
        event->acceptProposedAction();
    }
}

void XMainWindow::ShortcutsPressed(uint16_t native_key, uint16_t native_mods) {
    const auto shortcut = shortcuts_.value(qMakePair(native_key, native_mods));
    if (!shortcut.isEmpty()) {
        content_widget_->ShortcutsPressed(shortcut);
    }
}

void XMainWindow::DrivesRemoved(char driver_letter) {
#if defined(Q_OS_WIN)
    auto itr = std::ranges::find_if(exist_drives_, [driver_letter](auto drive) {
	                                    return drive.driver_letter == driver_letter;
                                    });
    if (itr != exist_drives_.end()) {
        content_widget_->DrivesRemoved(*itr);
        exist_drives_.erase(itr);
    }
#endif
}

bool XMainWindow::nativeEvent(const QByteArray& event_type, void* message, qintptr* result) {
#if defined(Q_OS_WIN)
    const auto* msg = static_cast<MSG const*>(message);
    switch (msg->message) {
    case DBT_DEVICEARRIVAL: {
        auto lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam);
        if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
            auto* lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lpdb);
            if (lpdbv->dbcv_flags & DBTF_MEDIA) {
                ReadDriveInfo();
            }
        }
        }
        break;
    case DBT_DEVICEREMOVECOMPLETE: {
        constexpr auto first_drive_from_mask = [](ULONG unitmask) -> char {
            char i = 0;
            for (i = 0; i < 26; ++i) {
                if (unitmask & 0x1)
                    break;
                unitmask = unitmask >> 1;
            }
            return i + 'A';
        };

        const auto lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam);
        if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
            const auto* lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lpdb);
            if (lpdbv->dbcv_flags & DBTF_MEDIA) {
                const auto driver_letter = first_drive_from_mask(lpdbv->dbcv_unitmask);
                DrivesRemoved(driver_letter);
            }
        }
        }
        break;
    case WM_HOTKEY: {
        const auto native_key = HIWORD(msg->lParam);
        const auto native_mods = LOWORD(msg->lParam);
        ShortcutsPressed(native_key, native_mods);
        XAMP_LOG_DEBUG("Hot key press native_key:{} native_mods:{}", native_key, native_mods);
        }
        break;
default: ;
    }    
#endif
    return IXMainWindow::nativeEvent(event_type, message, result);
}

void XMainWindow::ReadDriveInfo() {
#if defined(Q_OS_WIN)
    static const QSet<QByteArray> kCDFileSystemType = {
        "CDFS",
        "UDF",
        "ISO-9660",
        "ISO9660"
    };

    QList<DriveInfo> drives;
    Q_FOREACH(auto & storage, QStorageInfo::mountedVolumes()) {
        if (!storage.isValid() || !storage.isReady()) {
            return;
        }

        auto display_name = storage.displayName() + qTEXT("(") + storage.rootPath() + qTEXT(")");
        const auto driver_letter = storage.rootPath().left(1).toStdString()[0];
        const auto file_system_type = storage.fileSystemType();
        if (kCDFileSystemType.contains(file_system_type.toUpper())) {
            const auto device = OpenCD(driver_letter);
            const auto device_info = device->GetCDDeviceInfo();
            display_name += QString::fromStdWString(L" " + device_info.product);
            auto itr = exist_drives_.find(display_name);
            if (itr == exist_drives_.end()) {
                XAMP_LOG_DEBUG("Add new drive : {}", display_name.toStdString());
                DriveInfo info{
                   driver_letter ,
                   display_name,
                    QString(qTEXT("%1:\\")).arg(driver_letter)
                };
                exist_drives_[display_name] = info;
                drives.push_back(info);
            }
        }
    }
    if (drives.empty()) {
        return;
    }
    if (content_widget_ != nullptr) {
        content_widget_->DrivesChanges(drives);
    }
#endif
}

void XMainWindow::closeEvent(QCloseEvent* event) {
    if (!content_widget_) {
        QWidget::closeEvent(event);
        return;
    }

    content_widget_->close();
    QWidget::closeEvent(event);
}

void XMainWindow::InitMaximumState() {
    if (!content_widget_) {
        return;
    }
    content_widget_->UpdateMaximumState(isMaximized());
}

void XMainWindow::UpdateMaximumState() {
    if (!content_widget_) {
        return;
    }

    if (isMaximized()) {
        setWindowState(windowState() & ~Qt::WindowMinimized);
        showNormal();
        content_widget_->UpdateMaximumState(false);
    }
    else {
        showMaximized();
        content_widget_->UpdateMaximumState(true);
    }
}

void XMainWindow::changeEvent(QEvent* event) {
#if defined(Q_OS_MAC)
    if (content_widget_ != nullptr) {
        osx::hideTitleBar(content_widget_);
    }
#else
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            setUpdatesEnabled(false);
        }
        else if (windowState() == Qt::WindowNoState) {
            setAttribute(Qt::WA_Mapped);
            setUpdatesEnabled(true);
            update();
        }
    }
#endif
}

void XMainWindow::showEvent(QShowEvent* event) {
#if defined(Q_OS_WIN)
    if (!task_bar_) {
        return;
    }
    task_bar_->UpdateProgressIndicator();
    task_bar_->UpdateOverlay();
#endif
}

void XMainWindow::ShowWindow() {
    show();

    auto state = windowState();
    state &= ~Qt::WindowMinimized;
    state |= Qt::WindowActive;

    raise(); // for MacOS
    activateWindow(); // for Windows
}
