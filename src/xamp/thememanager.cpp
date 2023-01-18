#include "thememanager.h"

#include <widget/image_utiltis.h>
#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#else
#include <widget/osx/osx.h>
#endif
#include <widget/fonticonanimation.h>
#include <widget/appsettingnames.h>
#include <widget/widget_shared.h>
#include <widget/str_utilts.h>
#include <widget/appsettings.h>
#include <widget/iconsizestyle.h>

#include "ui_xamp.h"

#include <QDirIterator>
#include <QDesktopWidget>
#include <QGraphicsDropShadowEffect>
#include <QScreen>
#include <QResource>
#include <QPushButton>
#include <QApplication>
#include <QFontDatabase>
#include <QTextStream>
#include <QFileInfo>

template <typename Iterator>
static void sortFontWeight(Iterator begin, Iterator end) {
    std::sort(begin, end,
        [](const auto& left_font_name, const auto& right_font_name) {
            auto getFontWeight = [](auto name) {
            if (name.contains(qTEXT("Thin"), Qt::CaseInsensitive)) {
                return 100;
            }
            if (name.contains(qTEXT("Hairline"), Qt::CaseInsensitive)) {
                return 100;
            }
            if (name.contains(qTEXT("ExtraLight"), Qt::CaseInsensitive)) {
                return 200;
            }
            if (name.contains(qTEXT("Light"), Qt::CaseInsensitive)) {
                return 300;
            }
            if (name.contains(qTEXT("Normal"), Qt::CaseInsensitive)) {
                return 400;
            }
            if (name.contains(qTEXT("Regular"), Qt::CaseInsensitive)) {
                return 400;
            }
            if (name.contains(qTEXT("Medium"), Qt::CaseInsensitive)) {
                return 500;
            }
            if (name.contains(qTEXT("SemiBold"), Qt::CaseInsensitive)) {
                return 600;
            }
            if (name.contains(qTEXT("DemiBold"), Qt::CaseInsensitive)) {
                return 600;
            }
            if (name.contains(qTEXT("Bold"), Qt::CaseInsensitive)) {
                return 700;
            }
            if (name.contains(qTEXT("ExtraBold"), Qt::CaseInsensitive)) {
                return 800;
            }
            if (name.contains(qTEXT("UltraBold"), Qt::CaseInsensitive)) {
                return 800;
            }
            if (name.contains(qTEXT("Black"), Qt::CaseInsensitive)) {
                return 900;
            }
            if (name.contains(qTEXT("Heavy"), Qt::CaseInsensitive)) {
                return 900;
            }
            return 100;
        };
		return getFontWeight(left_font_name) < getFontWeight(right_font_name);
    });
}

bool ThemeManager::useNativeWindow() const {
    return use_native_window_;
}

QString ThemeManager::flagNamePath(const QString& countryIsoCode) {
    return
        qSTR("%1/flags/%2.png")
        .arg(QCoreApplication::applicationDirPath())
        .arg(countryIsoCode);
}

QString ThemeManager::fontNamePath(const QString& file_name) {
	return
		qSTR("%1/fonts/%2")
		.arg(QCoreApplication::applicationDirPath())
		.arg(file_name);
}

void ThemeManager::installFileFonts(const QString& font_name_prefix, QList<QString>& ui_fallback_fonts) {
	const auto font_path = qSTR("%1/fonts/").arg(QCoreApplication::applicationDirPath());

    QMap<int32_t, QString> font_weight_map;
    QDirIterator itr(font_path, QDir::Files | QDir::NoDotAndDotDot);
    while (itr.hasNext()) {
        auto file_path = itr.next();
        if (file_path.contains(font_name_prefix)) {
            installFileFont(itr.fileName(), ui_fallback_fonts);
        }
    }
}

void ThemeManager::installFileFont(const QString& file_name, QList<QString> &ui_fallback_fonts) {
	const auto font_path = fontNamePath(file_name);
	const QFileInfo info(font_path);
    if (!info.exists()) {
        XAMP_LOG_ERROR("Not found font file name: {}", file_name.toStdString());
        return;
    }
    PrefetchFile(font_path.toStdWString());
    const auto loaded_font_id = QFontDatabase::addApplicationFont(font_path);
    const auto font_families = QFontDatabase::applicationFontFamilies(loaded_font_id);
    if (!ui_fallback_fonts.contains(font_families[0])) {
        ui_fallback_fonts.push_back(font_families[0]);
    }
}

void ThemeManager::setFontAwesomeIcons() {
    const HashMap<char32_t, uint32_t> glyphs{
    { ICON_VOLUME_UP ,                0xF6A8 },
    { ICON_VOLUME_OFF,                0xF6A9 },
    { ICON_SPEAKER,                   0xF8DF },
    { ICON_FOLDER,                    0xF07B },
    { ICON_AUDIO,                     0xF001 },
    { ICON_LOAD_FILE,                 0xF15B },
    { ICON_LOAD_DIR,                  0xF07C },
    { ICON_RELOAD,                    0xF2F9 },
    { ICON_REMOVE_ALL,                0xE2AE },
    { ICON_OPEN_FILE_PATH,            0xF07C },
    { ICON_READ_REPLAY_GAIN,          0xF5F0 },
    { ICON_EXPORT_FILE,               0xF56E },
    { ICON_COPY,                      0xF0C5 },
    { ICON_DOWNLOAD,                  0xF0ED },
    { ICON_PLAYLIST,                  0xF8C9 },
    { ICON_EQUALIZER,                 0xF3F2 },
    { ICON_PODCAST,                   0xF2CE },
    { ICON_ALBUM,                     0xF89F },
    { ICON_CD,                        0xF51F },
    { ICON_LEFT_ARROW,                0xF177 },
    { ICON_ARTIST,                    0xF500 },
    { ICON_SUBTITLE,                  0xE1DE },
    { ICON_PREFERENCE,                0xF013 },
    { ICON_ABOUT,                     0xF05A },
    { ICON_DARK_MODE,                 0xF186 },
    { ICON_LIGHT_MODE,                0xF185 },
    { ICON_SEARCH,                    0xF002 },
    { ICON_THEME,                     0xF042 },
    { ICON_DESKTOP,                   0xF390 },
    { ICON_SHUFFLE_PLAY_ORDER,        0xF074 },
    { ICON_REPEAT_ONE_PLAY_ORDER,     0xF363 },
    { ICON_REPEAT_ONCE_PLAY_ORDER,    0xF365 },
    { ICON_MINIMIZE_WINDOW,           0xF2D1 },
    { ICON_MAXIMUM_WINDOW,            0xF2D0 },
    { ICON_CLOSE_WINDOW,              0xF00D },
    { ICON_RESTORE_WINDOW,            0xF2D2 },
    { ICON_SLIDER_BAR,                0xF0C9 },
    { ICON_PLAY_LIST_PLAY,            0xF04B },
    { ICON_PLAY_LIST_PAUSE,           0xF04C },
    { ICON_PLAY,                      0xF144 },
    { ICON_PAUSE,                     0xF28B },
    { ICON_STOP_PLAY,                 0xF04D },
    { ICON_PLAY_FORWARD,              0xF04E },
    { ICON_PLAY_BACKWARD,             0xF04A },
    { ICON_MORE,                      0xF142 },
    { ICON_HIDE,                      0xF070 },
    { ICON_SHOW,                      0xF06E },
    { ICON_USB,                       0xF8E9 },
    { ICON_BUILD_IN_SPEAKER,          0xF8DF },
    { ICON_BLUE_TOOTH,                0xF293 },
	{ ICON_MESSAGE_BOX_WARNING,       0xF071 },
    { ICON_MESSAGE_BOX_INFORMATION,   0xF05A },
    { ICON_MESSAGE_BOX_ERROR,         0xF05E },
	{ ICON_MESSAGE_BOX_QUESTION,      0xF059 },
    };
    
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        qFontIcon.addFont(fontNamePath(qTEXT("fa-solid-900.ttf")));
        break;
    case ThemeColor::LIGHT_THEME:
        qFontIcon.addFont(fontNamePath(qTEXT("fa-regular-400.ttf")));
        break;
    }
    qFontIcon.setGlyphs(glyphs);
}

QFont ThemeManager::loadFonts() {
    QList<QString> format_font;
    QList<QString> mono_fonts;
    QList<QString> display_fonts;
    QList<QString> ui_fonts;
    QList<QString> debug_fonts;

    setFontAwesomeIcons();

    installFileFont(qTEXT("Karla-Regular.ttf"), format_font);
    installFileFonts(qTEXT("Roboto-Regular"), mono_fonts);
    installFileFonts(qTEXT("Poppins"), ui_fonts);
    installFileFonts(qTEXT("MiSans"), ui_fonts);
    installFileFonts(qTEXT("FiraCode-Regular"), debug_fonts);

    sortFontWeight(ui_fonts.begin(), ui_fonts.end());

    if (display_fonts.isEmpty()) {
        display_fonts = ui_fonts;
    }
    if (mono_fonts.isEmpty()) {
        mono_fonts = ui_fonts;
    }

    QFont::insertSubstitutions(qTEXT("DebugFont"), debug_fonts);
    QFont::insertSubstitutions(qTEXT("DisplayFont"), display_fonts);
    QFont::insertSubstitutions(qTEXT("FormatFont"), format_font);
    QFont::insertSubstitutions(qTEXT("MonoFont"), mono_fonts);
    QFont::insertSubstitutions(qTEXT("UIFont"), ui_fonts);

    QFont ui_font(qTEXT("UIFont"));
    ui_font.setStyleStrategy(QFont::PreferAntialias);
#ifdef Q_OS_WIN
    ui_font.setWeight(QFont::Weight::Medium);
#else
    ui_font.setWeight(QFont::Weight::Normal);
#endif
    ui_font.setKerning(false);

    return ui_font;
}

void ThemeManager::setPalette() {
    palette_ = QPalette();
    if (theme_color_ == ThemeColor::LIGHT_THEME) {
        palette_.setColor(QPalette::WindowText, QColor(250, 250, 250));
        background_color_ = QColor(250, 250, 250);
    } else {
        palette_.setColor(QPalette::WindowText, QColor(25, 35, 45));
        background_color_ = QColor(40, 41, 42);
    }
}

ThemeManager::ThemeManager() {
    cover_size_ = QSize(210, 210);
    album_cover_size_ = QSize(250, 250);
    save_cover_art_size_ = QSize(500, 500);
    //setThemeColor(ThemeColor::DARK_THEME);
    //setThemeColor(ThemeColor::LIGHT_THEME);
    const auto theme = AppSettings::getAsEnum<ThemeColor>(kAppSettingTheme);
    setThemeColor(theme);
    ui_font_ = loadFonts();
    use_native_window_ = !AppSettings::getValueAsBool(kAppSettingUseFramelessWindow);
    ui_font_.setPointSize(fontSize());
    unknown_cover_ = QPixmap(qTEXT(":/xamp/Resource/White/unknown_album.png"));
	default_size_unknown_cover_ = ImageUtils::resizeImage(unknown_cover_, cover_size_);
}

void ThemeManager::setThemeColor(ThemeColor theme_color) {
    theme_color_ = theme_color;

    setPalette();
    AppSettings::setEnumValue(kAppSettingTheme, theme_color_);

    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        font_icon_opts_.insert(FontIconOption::colorAttr, QVariant(QColor(240, 241, 243)));
        font_icon_opts_.insert(FontIconOption::selectedColorAttr, QVariant(QColor(240, 241, 243)));
        break;
    case ThemeColor::LIGHT_THEME:
        font_icon_opts_.insert(FontIconOption::colorAttr, QVariant(QColor(97, 97, 101)));
        font_icon_opts_.insert(FontIconOption::selectedColorAttr, QVariant(QColor(97, 97, 101)));
        break;
    }
}

QLatin1String ThemeManager::themeColorPath() const {
    if (theme_color_ == ThemeColor::DARK_THEME) {
        return qTEXT("Black");
    }
    return qTEXT("White");
}

QColor ThemeManager::themeTextColor() const {
    auto color = Qt::black;
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        color = Qt::white;
        break;
    case ThemeColor::LIGHT_THEME:
        color = Qt::black;
        break;
    }
    return color;
}

QColor ThemeManager::backgroundColor() const noexcept {
    return background_color_;
}

QString ThemeManager::backgroundColorString() const {
    QString color;

    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        color = qTEXT("#19232D");
        break;
    case ThemeColor::LIGHT_THEME:
        color = qTEXT("#FAFAFA");
        break;
    }
    return color;
}

void ThemeManager::setMenuStyle(QWidget* menu) {
	menu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setAttribute(Qt::WA_StyledBackground);
    menu->setStyle(new IconSizeStyle(12));
}

QIcon ThemeManager::fontIcon(const char32_t code) const {
    switch (code) {
    case Glyphs::ICON_MINIMIZE_WINDOW:
    case Glyphs::ICON_MAXIMUM_WINDOW:
    case Glyphs::ICON_CLOSE_WINDOW:
    case Glyphs::ICON_RESTORE_WINDOW:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::scaleFactorAttr, QVariant::fromValue(1.0));
			return qFontIcon.icon(code, temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_WARNING:
	    {
			auto temp = font_icon_opts_;
            temp.insert(FontIconOption::colorAttr, QVariant(QColor(255, 164, 6)));
            return qFontIcon.icon(code, temp);
	    }
    case Glyphs::ICON_MESSAGE_BOX_ERROR:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::colorAttr, QVariant(QColor(189, 29, 29)));
			return qFontIcon.icon(code, temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_INFORMATION:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::colorAttr, QVariant(QColor(43, 128, 234)));
		    return qFontIcon.icon(code, temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_QUESTION:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::colorAttr, QVariant(QColor(53, 193, 31)));
			return qFontIcon.icon(code, temp);
		}
    }
    return qFontIcon.icon(code, font_icon_opts_);
}

QIcon ThemeManager::appIcon() const {
    return QIcon(qTEXT(":/xamp/xamp.ico"));
}

QIcon ThemeManager::playCircleIcon() const {
    return QIcon(qTEXT(":/xamp/Resource/Black/play_circle.png"));
}

QIcon ThemeManager::playlistPauseIcon(QSize icon_size) const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::scaleFactorAttr, QVariant::fromValue(0.3));
    font_options.insert(FontIconOption::colorAttr, QColor(250, 88, 106));
    font_options.insert(FontIconOption::selectedColorAttr, QColor(250, 88, 106));

    auto icon = qFontIcon.icon(Glyphs::ICON_PLAY_LIST_PAUSE, font_options);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::Off),
        QIcon::Selected, QIcon::Off);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::On),
        QIcon::Selected, QIcon::On);
    return icon;
}

QIcon ThemeManager::playlistPlayingIcon(QSize icon_size) const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::scaleFactorAttr, QVariant::fromValue(0.3));
    font_options.insert(FontIconOption::colorAttr, QColor(250, 88, 106));
    font_options.insert(FontIconOption::selectedColorAttr, QColor(250, 88, 106));
    auto icon = qFontIcon.icon(Glyphs::ICON_PLAY_LIST_PLAY, font_options);

    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::Off),
        QIcon::Selected, QIcon::Off);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::On),
        QIcon::Selected, QIcon::On);
    return icon;
}

QIcon ThemeManager::playingIcon() const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::colorAttr, QColor(252, 215, 75));
    return qFontIcon.icon(0xF8F2, font_options);
}

QIcon ThemeManager::hiResIcon() const {
    QVariantMap options;
    options.insert(FontIconOption::colorAttr, QColor(250, 197, 24));
    options.insert(FontIconOption::scaleFactorAttr, 0.8);
    return QIcon(qFontIcon.icon(0xE1AE, options));
}

void ThemeManager::setShufflePlayorder(Ui::XampWindow& ui) const {
    ui.repeatButton->setIcon(fontIcon(Glyphs::ICON_SHUFFLE_PLAY_ORDER));
}

void ThemeManager::setRepeatOnePlayOrder(Ui::XampWindow& ui) const {
    ui.repeatButton->setIcon(fontIcon(Glyphs::ICON_REPEAT_ONE_PLAY_ORDER));
}

void ThemeManager::setRepeatOncePlayOrder(Ui::XampWindow& ui) const {
    ui.repeatButton->setIcon(fontIcon(Glyphs::ICON_REPEAT_ONCE_PLAY_ORDER));
}

QPixmap ThemeManager::githubIcon() const {
	if (themeColor() == ThemeColor::DARK_THEME) {
        return QPixmap(qTEXT(":/xamp/Resource/Black/GitHub-Mark.png"));
	} else {
        return QPixmap(qTEXT(":/xamp/Resource/White/GitHub-Mark.png"));
	}
}

const QPixmap& ThemeManager::unknownCover() const noexcept {
    return unknown_cover_;
}

const QPixmap& ThemeManager::defaultSizeUnknownCover() const noexcept {
    return default_size_unknown_cover_;
}

void ThemeManager::updateMaximumIcon(Ui::XampWindow& ui, bool is_maximum) const {
    if (is_maximum) {
        ui.maxWinButton->setIcon(fontIcon(Glyphs::ICON_RESTORE_WINDOW));
    } else {
        ui.maxWinButton->setIcon(fontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
    }
}

void ThemeManager::setBitPerfectButton(Ui::XampWindow& ui, bool enable) {
    ui.bitPerfectButton->setText(qSTR("Bit-Perfect"));

    if (enable) {        
        ui.bitPerfectButton->setStyleSheet(qSTR(
            R"(
                QToolButton#bitPerfectButton {
					font-family: "FormatFont";
					font-weight: bold;
					color: white;
                    border: none;
                    background-color: rgb(167, 200, 255);
                }
            )"
        ));
    }
    else {        
        ui.bitPerfectButton->setStyleSheet(qSTR(
            R"(
                QToolButton#bitPerfectButton {
					font-family: "FormatFont";
					font-weight: bold;
					color: white;
                    border: none;
                    background-color: rgba(167, 200, 255, 60);
                }
            )"
        ));
    }
}

void ThemeManager::setPlayOrPauseButton(Ui::XampWindow& ui, bool is_playing) {
    if (is_playing) {
        ui.playButton->setIcon(fontIcon(Glyphs::ICON_PAUSE));
    }
    else {
        ui.playButton->setIcon(fontIcon(Glyphs::ICON_PLAY));
    }
}

const QSize& ThemeManager::defaultCoverSize() const noexcept {
    return cover_size_;
}

QSize ThemeManager::cacheCoverSize() const noexcept {
    return defaultCoverSize() * 2;
}

QSize ThemeManager::albumCoverSize() const noexcept {
    return album_cover_size_;
}

QSize ThemeManager::saveCoverArtSize() const noexcept {
    return save_cover_art_size_;
}

void ThemeManager::setBackgroundColor(QWidget* widget) {
    auto color = palette().color(QPalette::WindowText);
    widget->setStyleSheet(backgroundColorToString(color));
}

void ThemeManager::applyTheme() {
    qApp->setFont(defaultFont());

    QString filename;

    if (themeColor() == ThemeColor::LIGHT_THEME) {
        filename = qTEXT(":/xamp/Resource/Theme/light/style.qss");
    } else {
        filename = qTEXT(":/xamp/Resource/Theme/dark/style.qss");
    }

    QFile f(filename);
    f.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&f);
    qApp->setStyleSheet(ts.readAll());
    f.close();
}

void ThemeManager::setBackgroundColor(Ui::XampWindow& ui, QColor color) {
    background_color_ = color;
    AppSettings::setValue(kAppSettingBackgroundColor, color);
}

QColor ThemeManager::titleBarColor() const {
    return QColor(backgroundColor());
}

QColor ThemeManager::coverShadownColor() const {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        return Qt::black;
    case ThemeColor::LIGHT_THEME:
    default:
        return QColor(qTEXT("#DCDCDC"));
    }
}

QSize ThemeManager::tabIconSize() const {
#ifdef XAMP_OS_MAC
    return QSize(20, 20);
#else
    return QSize(18, 18);
#endif
}

void ThemeManager::updateTitlebarState(QFrame *title_bar, bool is_focus) {
}

QColor ThemeManager::hoverColor() const {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        return QColor(qTEXT("#43474e"));
    case ThemeColor::LIGHT_THEME:
    default:
        return QColor(qTEXT("#C9CDD0"));
    }
}

QColor ThemeManager::highlightColor() const {
    switch (themeColor()) {
    case ThemeColor::LIGHT_THEME:
        return QColor(qTEXT("#9FCBFF"));
    case ThemeColor::DARK_THEME:
    default:
        return QColor(qTEXT("#a7c8ff"));
    }
}

void ThemeManager::setStandardButtonStyle(QToolButton* close_button, QToolButton* min_win_button, QToolButton* max_win_button) const {
    const QColor hover_color = hoverColor();
    auto font_size = 10;

    close_button->setStyleSheet(qSTR(R"(
                                         QToolButton#closeButton {
                                         border: none;
                                         background-color: transparent;
										 border-radius: 0px;
                                         }

										 QToolButton#closeButton:hover {
										 background-color: %1;
										 border-radius: 0px;
                                         }
                                         )").arg(colorToString(hover_color)));
    close_button->setIconSize(QSize(font_size, font_size));
    close_button->setIcon(fontIcon(Glyphs::ICON_CLOSE_WINDOW));

    min_win_button->setStyleSheet(qSTR(R"(
                                          QToolButton#minWinButton {
                                          border: none;
                                          background-color: transparent;
										  border-radius: 0px;
                                          }
										  QToolButton#minWinButton:hover {		
										  background-color: %1;
										  border-radius: 0px;				 
                                          }
                                          )").arg(colorToString(hover_color)));
    min_win_button->setIconSize(QSize(font_size, font_size));
    min_win_button->setIcon(fontIcon(Glyphs::ICON_MINIMIZE_WINDOW));

    max_win_button->setStyleSheet(qSTR(R"(
                                          QToolButton#maxWinButton {
                                          border: none;
                                          background-color: transparent;
										  border-radius: 0px;
                                          }
										  QToolButton#maxWinButton:hover {		
										  background-color: %1;
										  border-radius: 0px;								 
                                          }
                                          )").arg(colorToString(hover_color)));
    max_win_button->setIconSize(QSize(font_size, font_size));
    max_win_button->setIcon(fontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
}

void ThemeManager::setThemeIcon(Ui::XampWindow& ui) const {
    setStandardButtonStyle(ui.closeButton, ui.minWinButton, ui.maxWinButton);

    const QColor hover_color = hoverColor();

    if (useNativeWindow()) {
        ui.logoButton->hide();
    } else {
        ui.logoButton->setStyleSheet(qSTR(R"(
                                         QToolButton#logoButton {
                                         border: none;
                                         image: url(":/xamp/xamp.ico");
                                         background-color: transparent;
                                         }
										)"));
    }

    ui.sliderBarButton->setStyleSheet(qSTR(R"(
                                            QToolButton#sliderBarButton {
                                            border: none;
                                            background-color: transparent;
											border-radius: 0px;
                                            }
											QToolButton#sliderBarButton:hover {												
											background-color: %1;
											border-radius: 0px;								 
											}
                                            QToolButton#sliderBarButton::menu-indicator {
                                            image: none;
                                            }
                                            )").arg(colorToString(hover_color)));
    ui.sliderBarButton->setIcon(fontIcon(Glyphs::ICON_SLIDER_BAR));

    ui.stopButton->setStyleSheet(qSTR(R"(
                                         QToolButton#stopButton {
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));
    ui.stopButton->setIcon(fontIcon(ICON_STOP_PLAY));

    ui.nextButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#nextButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.nextButton->setIcon(fontIcon(Glyphs::ICON_PLAY_FORWARD));

    ui.prevButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#prevButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.prevButton->setIcon(fontIcon(Glyphs::ICON_PLAY_BACKWARD));

    ui.selectDeviceButton->setStyleSheet(qTEXT(R"(
                                                QToolButton#selectDeviceButton {                                                
                                                border: none;
                                                background-color: transparent;                                                
                                                }
                                                QToolButton#selectDeviceButton::menu-indicator { image: none; }
                                                )"));
    ui.selectDeviceButton->setIcon(fontIcon(Glyphs::ICON_SPEAKER));

    ui.mutedButton->setStyleSheet(qSTR(R"(
                                         QToolButton#mutedButton {
                                         image: url(:/xamp/Resource/%1/volume_up.png);
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )").arg(themeColorPath()));

    ui.eqButton->setStyleSheet(qTEXT(R"(
                                         QToolButton#eqButton {
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));
    ui.eqButton->setIcon(fontIcon(Glyphs::ICON_EQUALIZER));
}

void ThemeManager::setTextSeparator(QFrame *frame) {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        frame->setStyleSheet(qTEXT("background-color: #37414F;"));
        break;
    case ThemeColor::LIGHT_THEME:
        frame->setStyleSheet(qTEXT("background-color: #CED1D4;"));
        break;
    }
}

int32_t ThemeManager::fontSize() const {
    const auto dpi = qApp->desktop()->logicalDpiX();
    if (dpi >= 96) {
        return 10;
    }
    return 14;
}

void ThemeManager::setMuted(QAbstractButton *button, bool is_muted) {
    if (!is_muted) {
        button->setIcon(qTheme.fontIcon(Glyphs::ICON_VOLUME_UP));
        AppSettings::setValue(kAppSettingIsMuted, false);
    }
    else {
        button->setIcon(qTheme.fontIcon(Glyphs::ICON_VOLUME_OFF));
        AppSettings::setValue(kAppSettingIsMuted, true);
    }
}

void ThemeManager::setVolume(QSlider *slider, QAbstractButton* button, uint32_t volume) {
    if (!slider->isEnabled()) {
        return;
    }
    if (volume == 0) {
        setMuted(button, true);
    }
    else {
        setMuted(button, false);
    }
    slider->setValue(volume);
}

void ThemeManager::setMuted(Ui::XampWindow& ui, bool is_muted) {
    setMuted(ui.mutedButton, is_muted);
}

void ThemeManager::setDeviceConnectTypeIcon(QAbstractButton* button, DeviceConnectType type) {
    switch (type) {
    case DeviceConnectType::UKNOWN:
    case DeviceConnectType::BUILT_IN:
        button->setIcon(qTheme.fontIcon(Glyphs::ICON_BUILD_IN_SPEAKER));
        break;
    case DeviceConnectType::USB:
        button->setIcon(qTheme.fontIcon(Glyphs::ICON_USB));
        break;
    case DeviceConnectType::BLUE_TOOTH:
        button->setIcon(qTheme.fontIcon(Glyphs::ICON_BLUE_TOOTH));
        break;
    }
}

void ThemeManager::setSliderTheme(QSlider* slider) {
    QString slider_border_color;
    QString slider_background_color;

    switch (themeColor()) {
    case ThemeColor::LIGHT_THEME:
        slider_border_color = qTEXT("#C9CDD0");
        slider_background_color = qTEXT("#9FCBFF");
        break;
    case ThemeColor::DARK_THEME:
        slider_border_color = qTEXT("#43474e");
        slider_background_color = qTEXT("#a7c8ff");
        break;
    }

    slider->setStyleSheet(qTEXT(R"(
    QSlider {
		background-color: transparent;
    }

	QSlider#%1::groove:horizontal {        
        background: %2;
		border: 1px solid %2;
        height: 2px;
        border-radius: 2px;
        padding-left: -1px;
        padding-right: -5px;
    }

    QSlider#%1::sub-page:horizontal {
		background: %2;
		border: 1px solid %2;		
		height: 2px;
		border-radius: 2px;
    }

    QSlider#%1::add-page:horizontal {
		background: %3;
		border: 0px solid %2;
		height: 2px;
		border-radius: 2px;
    }

    QSlider#%1::handle:horizontal {
        width: 10px;
		height: 10px;
        margin: -5px 0px -5px 0px;
		border-radius: 5px;
		background-color: %2;
		border: 1px solid %2;
    }
    )"
    ).arg(slider->objectName())
        .arg(slider_background_color)
        .arg(slider_border_color));
}

void ThemeManager::setWidgetStyle(Ui::XampWindow& ui) {
    ui.playButton->setIconSize(QSize(32, 32));
    ui.selectDeviceButton->setIconSize(QSize(32, 32));
    ui.mutedButton->setIconSize(QSize(24, 24));

    ui.nextButton->setIconSize(QSize(24, 24));
    ui.prevButton->setIconSize(QSize(24, 24));
    ui.stopButton->setIconSize(QSize(24, 24));

    QString slider_border_color;
    QString slider_background_color;

    switch (themeColor()) {
    case ThemeColor::LIGHT_THEME:
        slider_border_color = qTEXT("#C9CDD0");
        slider_background_color = qTEXT("#9FCBFF");
        break;
    case ThemeColor::DARK_THEME:
        slider_border_color = qTEXT("#43474e");
        slider_background_color = qTEXT("#a7c8ff");
        break;
    }

    ui.playButton->setStyleSheet(qTEXT(R"(
                                            QToolButton#playButton {
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )"));

    ui.searchLineEdit->setStyleSheet(qTEXT(""));

    ui.searchFrame->setStyleSheet(qTEXT("QFrame#searchFrame { background-color: transparent; border: none; }"));

    ui.startPosLabel->setStyleSheet(qTEXT(R"(
                                           QLabel#startPosLabel {
                                           color: gray;
                                           background-color: transparent;
                                           }
                                           )"));

    ui.endPosLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#endPosLabel {
                                         color: gray;
                                         background-color: transparent;
                                         }
                                         )"));

    ui.titleFrameLabel->setStyleSheet(qSTR(R"(
    QLabel#titleFrameLabel {
    border: none;
    background: transparent;
    }
    )"));

    ui.tableLabel->setStyleSheet(qSTR(R"(
    QLabel#tableLabel {
    border: none;
    background: transparent;
	color: gray;
    }
    )"));

    ui.searchLineEdit->setClearButtonEnabled(true);
    if (theme_color_ == ThemeColor::DARK_THEME) {
        ui.titleLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#titleLabel {
                                         color: white;
                                         background-color: transparent;
                                         }
                                         )"));

        ui.searchLineEdit->setStyleSheet(qSTR(R"(
                                            QLineEdit#searchLineEdit {
                                            background-color: %1;
                                            border: gray;
                                            color: white;
                                            border-radius: 10px;
                                            }
                                            )").arg(colorToString(Qt::black)));

        ui.currentView->setStyleSheet(qSTR(R"(
			QStackedWidget#currentView {
				background-color: #121212;
				border: 1px solid black;
				border-top-left-radius: 8px;
            }			
            )"));

        ui.bottomFrame->setStyleSheet(
            qTEXT(R"(
            QFrame#bottomFrame{
                border-top: 1px solid black;
                border-radius: 0px;
				border-bottom: none;
				border-left: none;
				border-right: none;
            }
            )"));
    }
    else {
        ui.titleLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#titleLabel {
                                         color: black;
                                         background-color: transparent;
                                         }
                                         )"));

        ui.searchLineEdit->setStyleSheet(qSTR(R"(
                                            QLineEdit#searchLineEdit {
                                            background-color: %1;
                                            border: gray;
                                            color: black;
                                            border-radius: 10px;
                                            }
                                            )").arg(colorToString(Qt::white)));

        ui.currentView->setStyleSheet(qTEXT(R"(
			QStackedWidget#currentView {
				background-color: #f9f9f9;
				border: 1px solid #eaeaea;
				border-top-left-radius: 8px;
            }			
            )"));

        ui.bottomFrame->setStyleSheet(
            qTEXT(R"(
            QFrame#bottomFrame {
                border-top: 1px solid #eaeaea;
                border-radius: 0px;
				border-bottom: none;
				border-left: none;
				border-right: none;
            }
            )"));
    }

    ui.artistLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#artistLabel {
                                         color: rgb(250, 88, 106);
                                         background-color: transparent;
                                         }
                                         )"));

    ui.repeatButton->setStyleSheet(qTEXT(R"(
    QToolButton#repeatButton {
    border: none;
    background: transparent;
    }
    )"
    ));

    QString slider_bar_left_color;
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        slider_bar_left_color = qTEXT("42, 130, 218");
        break;
    case ThemeColor::LIGHT_THEME:
        slider_bar_left_color = qTEXT("42, 130, 218");
        break;
    }

    ui.sliderBar->setStyleSheet(qSTR(R"(
	QListView#sliderBar {
		border: none; 
	}
	QListView#sliderBar::item {
		border: 0px;
		padding-left: 6px;
	}
	QListView#sliderBar::item:hover {
		border-radius: 2px;
	}
	QListView#sliderBar::item:selected {
		padding-left: 4px;
		border-left-width: 2px;
		border-left-style: solid;
		border-left-color: rgb(%1);
	}	
	)").arg(slider_bar_left_color));

    setSliderTheme(ui.seekSlider);

    setThemeIcon(ui);
    ui.sliderBarButton->setIconSize(tabIconSize());
    ui.sliderFrame->setStyleSheet(qTEXT("background: transparent; border: none;"));    
    ui.currentViewFrame->setStyleSheet(qTEXT("background: transparent; border: none;"));
}
