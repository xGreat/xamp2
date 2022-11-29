#include <QDesktopWidget>
#include <QProgressBar>
#include <QCheckBox>
#include <QProgressDialog>

#include <version.h>
#include <widget/xdialog.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>

QString sampleRate2String(const AudioFormat& format) {
    return samplerate2String(format.GetSampleRate());
}

QString format2String(const PlaybackFormat& playback_format, const QString& file_ext) {
    auto format = playback_format.file_format;

    auto ext = file_ext;
    ext = ext.remove(qTEXT(".")).toUpper();

    auto precision = 1;
    auto is_mhz_sample_rate = false;
    if (format.GetSampleRate() / 1000 > 1000) {
        is_mhz_sample_rate = true;
    }
    else {
        precision = format.GetSampleRate() % 1000 == 0 ? 0 : 1;
    }

    auto bits = format.GetBitsPerSample();

    QString dsd_speed_format;
    if (playback_format.is_dsd_file
        && (playback_format.dsd_mode == DsdModes::DSD_MODE_NATIVE || playback_format.dsd_mode == DsdModes::DSD_MODE_DOP)) {
        dsd_speed_format = qTEXT("DSD") + QString::number(playback_format.dsd_speed);
        dsd_speed_format = qTEXT("(") + dsd_speed_format + qTEXT(") | ");
        bits = 1;
    }

    QString output_format_str;
    QString dsd_mode;

    switch (playback_format.dsd_mode) {
    case DsdModes::DSD_MODE_PCM:
    case DsdModes::DSD_MODE_DSD2PCM:
        output_format_str = sampleRate2String(playback_format.file_format);
        if (playback_format.file_format.GetSampleRate() != playback_format.output_format.GetSampleRate()) {
            output_format_str += qTEXT("/") + sampleRate2String(playback_format.output_format);
        }
        break;
    case DsdModes::DSD_MODE_NATIVE:
        dsd_mode = qTEXT("Native DSD");
        output_format_str = dsdSampleRate2String(playback_format.dsd_speed);
        break;
    case DsdModes::DSD_MODE_DOP:
        dsd_mode = qTEXT("DOP");
        output_format_str = dsdSampleRate2String(playback_format.dsd_speed);
        break;
    default: 
        break;
    }

    const auto bit_format = QString::number(bits) + qTEXT("bit");

    auto result = ext
        + qTEXT(" | ")
        + dsd_speed_format
        + output_format_str
        + qTEXT(" | ")
        + bit_format;

    if (!dsd_mode.isEmpty()) {
        result += qTEXT(" | ") + dsd_mode;
    }

    result += qTEXT(" |") + bitRate2String(playback_format.bitrate);
    return result;
}

QScopedPointer<QProgressDialog> makeProgressDialog(QString const& title, QString const& text, QString const& cancel) {
    auto* dialog = new QProgressDialog(text, cancel, 0, 100);
    dialog->setFont(qApp->font());
    dialog->setWindowTitle(title);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setMinimumSize(QSize(1000, 100));
    dialog->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
    dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    auto* progress_bar = new QProgressBar(dialog);
    progress_bar->setFont(QFont(qTEXT("FormatFont")));
    dialog->setBar(progress_bar);
    return QScopedPointer<QProgressDialog>(dialog);
}

std::tuple<bool, QMessageBox::StandardButton> showDontShowAgainDialog(bool show_agin) {
    bool is_show_agin = true;

    if (show_agin) {
        auto cb = new QCheckBox(qApp->tr("Don't show this again"));
        QMessageBox msgbox;
        msgbox.setWindowTitle(kAppTitle);
        msgbox.setText(qApp->tr("Hide XAMP to system tray?"));
        msgbox.setIcon(QMessageBox::Icon::Question);
        msgbox.addButton(QMessageBox::Ok);
        msgbox.addButton(QMessageBox::Cancel);
        msgbox.setDefaultButton(QMessageBox::Cancel);
        msgbox.setCheckBox(cb);

        (void)QObject::connect(cb, &QCheckBox::stateChanged, [&is_show_agin](int state) {
            if (static_cast<Qt::CheckState>(state) == Qt::CheckState::Checked) {
                is_show_agin = false;
            }
            });
        return { is_show_agin, static_cast<QMessageBox::StandardButton>(msgbox.exec()) };
    }
    return { is_show_agin, QMessageBox::Yes };
}

void centerParent(QWidget* widget) {
    if (widget->parent() && widget->parent()->isWidgetType()) {
        widget->move((widget->parentWidget()->width() - widget->width()) / 2,
            (widget->parentWidget()->height() - widget->height()) / 2);
    }
}

void centerDesktop(QWidget* widget) {
    auto desktop = QApplication::desktop();
    auto screen_num = desktop->screenNumber(QCursor::pos());
    QRect rect = desktop->screenGeometry(screen_num);
    widget->move(rect.center() - widget->rect().center());
}

QString getFileExtensions() {
    QString exts(qTEXT("("));
    for (const auto& file_ext : GetSupportFileExtensions()) {
        exts += qTEXT("*") + QString::fromStdString(file_ext);
        exts += qTEXT(" ");
    }
    exts += qTEXT(")");
    return exts;
}
