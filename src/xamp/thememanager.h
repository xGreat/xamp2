//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPixmap>
#include <QIcon>
#include <QMenu>
#include <QFrame>

#include <widget/widget_shared.h>
#include <widget/str_utilts.h>
#include <widget/fonticon.h>

class QPushButton;
class QToolButton;

namespace Ui {
class XampWindow;
}

inline constexpr int32_t kUIRadius = 9;

enum class ThemeColor {
    DARK_THEME,
    LIGHT_THEME,
};

class ThemeManager {
public:
    friend class SharedSingleton<ThemeManager>;

    bool useNativeWindow() const;

    const QPalette& palette() const {
        return palette_;
    }

    const QFont& defaultFont() const {
        return ui_font_;
    }

    const QPixmap& unknownCover() const noexcept;

	const QPixmap& defaultSizeUnknownCover() const noexcept;

    QIcon desktopIcon() const;

    QIcon folderIcon() const;

    QIcon appIcon() const;

    QIcon volumeUp() const;

    QIcon volumeOff() const;

    QIcon playlistIcon() const;

    QIcon podcastIcon() const;

    QIcon equalizerIcon() const;

    QIcon albumsIcon() const;

    QIcon artistsIcon() const;

    QIcon subtitleIcon() const;

    QIcon preferenceIcon() const;

    QIcon aboutIcon() const;

    QIcon darkModeIcon() const;

    QIcon lightModeIcon() const;

    QIcon seachIcon() const;

    QIcon themeIcon() const;

    QIcon cdIcon() const;

    QIcon moreIcon() const;

    void setPlayOrPauseButton(Ui::XampWindow &ui, bool is_playing);

    void setBitPerfectButton(Ui::XampWindow& ui, bool enable);

    void setWidgetStyle(Ui::XampWindow &ui);    

    const QSize& defaultCoverSize() const noexcept;

    QSize cacheCoverSize() const noexcept;

    QSize albumCoverSize() const noexcept;

    QSize saveCoverArtSize() const noexcept;

    QIcon playArrow() const;

    QIcon playCircleIcon() const;

    QIcon speakerIcon() const;

    QIcon usbIcon() const;

    QIcon minimizeWindowIcon() const;

    QIcon maximumWindowIcon() const;

    QIcon closeWindowIcon() const;

    QIcon restoreWindowIcon() const;

    QIcon sliderBarIcon() const;

    void updateMaximumIcon(Ui::XampWindow& ui, bool is_maximum) const;

    void updateTitlebarState(QFrame* title_bar, bool is_focus);

    void setThemeIcon(Ui::XampWindow& ui) const;

    void setShufflePlayorder(Ui::XampWindow& ui) const;

    void setRepeatOnePlayOrder(Ui::XampWindow& ui) const;

    void setRepeatOncePlayOrder(Ui::XampWindow& ui) const;

    void setThemeColor(ThemeColor theme_color);

    void setBackgroundColor(Ui::XampWindow& ui, QColor color);

    void setThemeButtonIcon(Ui::XampWindow& ui);

    void applyTheme();

    void setBackgroundColor(QWidget* widget);

    QLatin1String themeColorPath() const;

    void setMenuStyle(QWidget* menu);

    QColor themeTextColor() const;

    QString backgroundColorString() const;

    QColor backgroundColor() const noexcept;

    QColor hoverColor() const;

    QColor titleBarColor() const;

    QColor coverShadownColor() const;

    ThemeColor themeColor() const {
        return theme_color_;
    }

    QSize tabIconSize() const;

    void setStandardButtonStyle(QToolButton* close_button, QToolButton* min_win_button, QToolButton* max_win_button) const;

private:
    static QString fontNamePath(const QString& file_name);

    QFont loadFonts();

    void installFileFont(const QString& file_name, QList<QString>& ui_fallback_fonts);

    void setPalette();

    QIcon makeIcon(const QString& path) const;

    ThemeManager();
    
    bool use_native_window_;
    ThemeColor theme_color_;
    QSize album_cover_size_;
    QSize cover_size_;
    QSize save_cover_art_size_;
    QColor background_color_;
    QPalette palette_;
    QFont ui_font_;
    QIcon play_arrow_;
    QPixmap unknown_cover_;
    QPixmap default_size_unknown_cover_;
    FontIcon mdl2_font_icon_;
};

#define qTheme SharedSingleton<ThemeManager>::GetInstance()

