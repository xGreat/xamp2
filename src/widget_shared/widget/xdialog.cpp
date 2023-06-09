#include <widget/xdialog.h>

#include <thememanager.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <FramelessHelper/Widgets/framelesswidgetshelper.h>

#include <QLabel>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

XDialog::XDialog(QWidget* parent)
    : FramelessDialog(parent) {
}

void XDialog::SetContent(QWidget* content) {
    content_ = content;

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setObjectName(QString::fromUtf8("default_layout"));
    default_layout->setContentsMargins(5, 0, 5, 5);
    setLayout(default_layout);

    auto* title_frame = new QFrame();
    title_frame->setObjectName(QString::fromUtf8("titleFrame"));
    title_frame->setMinimumSize(QSize(0, 24));
    title_frame->setFrameShape(QFrame::NoFrame);
    title_frame->setFrameShadow(QFrame::Plain);

    auto f = font();
    f.setBold(true);
    f.setPointSize(qTheme.GetFontSize(10));
    title_frame_label_ = new QLabel(title_frame);
    title_frame_label_->setObjectName(QString::fromUtf8("titleFrameLabel"));
    QSizePolicy size_policy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
    size_policy3.setHorizontalStretch(1);
    size_policy3.setVerticalStretch(0);
    size_policy3.setHeightForWidth(title_frame_label_->sizePolicy().hasHeightForWidth());
    title_frame_label_->setFont(f);
    title_frame_label_->setSizePolicy(size_policy3);
    title_frame_label_->setAlignment(Qt::AlignCenter);
    title_frame_label_->setStyleSheet(qSTR(R"(
    QLabel#titleFrameLabel {
    border: none;
    background: transparent;
	color: gray;
    }
    )"));

    close_button_ = new QToolButton(title_frame);
    close_button_->setObjectName(QString::fromUtf8("closeButton"));
    close_button_->setMinimumSize(QSize(24, 24));
    close_button_->setMaximumSize(QSize(24, 24));
    close_button_->setFocusPolicy(Qt::NoFocus);

    max_win_button_ = new QToolButton(title_frame);
    max_win_button_->setObjectName(QString::fromUtf8("maxWinButton"));
    max_win_button_->setMinimumSize(QSize(24, 24));
    max_win_button_->setMaximumSize(QSize(24, 24));
    max_win_button_->setFocusPolicy(Qt::NoFocus);

    min_win_button_ = new QToolButton(title_frame);
    min_win_button_->setObjectName(QString::fromUtf8("minWinButton"));
    min_win_button_->setMinimumSize(QSize(24, 24));
    min_win_button_->setMaximumSize(QSize(24, 24));
    min_win_button_->setFocusPolicy(Qt::NoFocus);
    min_win_button_->setPopupMode(QToolButton::InstantPopup);

    icon_ = new QToolButton(title_frame);
    icon_->setObjectName(QString::fromUtf8("minWinButton"));
    icon_->setMinimumSize(QSize(24, 24));
    icon_->setMaximumSize(QSize(24, 24));
    icon_->setFocusPolicy(Qt::NoFocus);
    icon_->setStyleSheet(qTEXT("background: transparent; border: none;"));
    icon_->hide();

    auto* horizontal_spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto* horizontal_layout = new QHBoxLayout(title_frame);
    horizontal_layout->addWidget(icon_);
    horizontal_layout->addItem(horizontal_spacer);
    horizontal_layout->addWidget(title_frame_label_);
    horizontal_layout->addWidget(min_win_button_);
    horizontal_layout->addWidget(max_win_button_);
    horizontal_layout->addWidget(close_button_);

    horizontal_layout->setSpacing(0);
    horizontal_layout->setObjectName(QString::fromUtf8("horizontalLayout"));
    horizontal_layout->setContentsMargins(0, 0, 0, 0);

    default_layout->addWidget(title_frame, 0);
    default_layout->addWidget(content_, 1);
    default_layout->setContentsMargins(0, 0, 0, 0);

    qTheme.SetTitleBarButtonStyle(close_button_, min_win_button_, max_win_button_);

    max_win_button_->setDisabled(true);
    min_win_button_->setDisabled(true);

    max_win_button_->hide();
    min_win_button_->hide();

    (void)QObject::connect(close_button_, &QToolButton::pressed, [this]() {
        close();
        });

    (void)QObject::connect(&qTheme,
        &ThemeManager::CurrentThemeChanged,
        this,
        &XDialog::OnCurrentThemeChanged);

    FramelessWidgetsHelper::get(this)->setTitleBarWidget(title_frame);
    FramelessWidgetsHelper::get(this)->setSystemButton(min_win_button_, Global::SystemButtonType::Minimize);
    FramelessWidgetsHelper::get(this)->setSystemButton(max_win_button_, Global::SystemButtonType::Maximize);
    FramelessWidgetsHelper::get(this)->setSystemButton(close_button_, Global::SystemButtonType::Close);

    WaitForReady();    
}

void XDialog::WaitForReady() {
    FramelessWidgetsHelper::get(this)->waitForReady();
    //FramelessWidgetsHelper::get(this)->moveWindowToDesktopCenter();
    //CenterParent(this);
}

void XDialog::OnCurrentThemeChanged(ThemeColor theme_color) {
    qTheme.SetTitleBarButtonStyle(close_button_, min_win_button_, max_win_button_);
}

void XDialog::SetTitle(const QString& title) const {
    title_frame_label_->setText(title);
}

void XDialog::SetContentWidget(QWidget* content, bool transparent_frame, bool disable_resize) {
    SetContent(content);
}

void XDialog::SetIcon(const QIcon& icon) const {
    icon_->setIcon(icon);
    icon_->setHidden(false);
}

void XDialog::showEvent(QShowEvent* event) {
    auto* opacity_effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacity_effect);
    auto* opacity_animation = new QPropertyAnimation(opacity_effect, "opacity", this);
    opacity_animation->setStartValue(0);
    opacity_animation->setEndValue(1);
    opacity_animation->setDuration(200);
    opacity_animation->setEasingCurve(QEasingCurve::OutCubic);
    (void)QObject::connect(opacity_animation,
        &QPropertyAnimation::finished,
        opacity_effect,
        &QGraphicsOpacityEffect::deleteLater);
    opacity_animation->start();

    setAttribute(Qt::WA_Mapped);
    QDialog::showEvent(event);
}
