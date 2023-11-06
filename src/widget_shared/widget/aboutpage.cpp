#include <widget/aboutpage.h>
#include <ui_aboutdialog.h>
#include <QFile>

#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <version.h>
#include <thememanager.h>

namespace {
#ifdef Q_OS_WIN32
    constexpr ConstLatin1String VisualStudioVersion() {
        if constexpr (_MSC_VER >= 1930) {
            return "2022";
        }
        return "2019";
    }
    QString GetCompileTime() {
        return qSTR("Visual Studio %1.%2.%3")
            .arg(VisualStudioVersion())
            .arg((_MSC_FULL_VER / 100000) % 100)
            .arg(_MSC_FULL_VER % 100000);
    }
#else
    QString GetCompileTime() {
        return qSTR("Clang %1.%2.%3")
            .arg(__clang_major__)
            .arg(__clang_minor__)
            .arg(__clang_patchlevel__);
    }
#endif
}

AboutPage::AboutPage(QWidget* parent)
    : QFrame(parent) {
    ui_ = new Ui::AboutDialog();
    ui_->setupUi(this);

    ui_->lblLogo->setPixmap(qTheme.GetApplicationIcon().pixmap(128, 128));

    QFont dp_font(qTheme.GetDisplayFont());
    dp_font.setBold(true);
    dp_font.setPointSizeF(qTheme.GetFontSize(26));

    ui_->lblProjectTitle->setFont(dp_font);
    ui_->lblProjectTitle->setText(kApplicationName);
    ui_->lblProjectTitle->setStyleSheet(qTEXT("QLabel#lblProjectTitle { border: none; background: transparent; }"));

    ui_->lblDescription->setText(QString::fromStdWString(L"Cross-platform native DSD and low latency playback music player."));

    QString domain_txt(qTEXT("<html><head/><body><a href=\"http://%1\">Github</a></body></html>"));
    ui_->lblDomain->setText(domain_txt.arg(qTEXT("github.com/billlin0904/xamp2")));
    ui_->lbIGithubIcon->setPixmap(qTheme.GetGithubIcon());
    ui_->lblCopying->setText(QString::fromStdWString(L"Copyright \u00A9 2018-2023 XAMP2 Project."));

    QFont font(qTheme.GetUiFont());
    font.setBold(false);
    font.setPointSize(qTheme.GetDefaultFontSize());
    ui_->txtBws->setFont(font);
    ui_->txtBws->setVisible(false);

    QFile lincense_file(qTEXT("lincense.txt"));
    if (lincense_file.open(QIODevice::ReadOnly)) {
        license_ = QLatin1String(lincense_file.readAll());
    }

    QFile credits_file(qTEXT("credits.txt"));
    if (credits_file.open(QIODevice::ReadOnly)) {
        credits_ = QLatin1String(credits_file.readAll());
    }

    ui_->lblAppBuild->setText(qApp->tr("Version ")
        + FormatVersion(kApplicationVersionValue));

    (void)QObject::connect(ui_->btnCredits,
        &QPushButton::clicked,
        this, 
        &AboutPage::OnCreditsOrLicenseChecked);
    (void)QObject::connect(ui_->btnLicense,
        &QPushButton::clicked,
        this,
        &AboutPage::OnCreditsOrLicenseChecked);

    (void)QObject::connect(ui_->restartAppButton,
        &QPushButton::clicked, [this]() {
            emit RestartApp();
        });

    ui_->restartAppButton->hide();

    ui_->lblAppBuild->setText(qApp->tr("Version ")
        + FormatVersion(kApplicationVersionValue));
    ui_->waitForUpdateProcessIndicator->StartAnimation();

    ui_->lblLogo->setStyleSheet(qTEXT("background-color: transparent"));
    ui_->lblProjectTitle->setStyleSheet(qTEXT("background-color: transparent"));
    ui_->lblCopying->setStyleSheet(qTEXT("background-color: transparent"));
    ui_->lblDescription->setStyleSheet(qTEXT("background-color: transparent"));
    ui_->lblAppBuild->setStyleSheet(qTEXT("background-color: transparent; color: gray;"));
}

AboutPage::~AboutPage() {
    delete ui_;
}

void AboutPage::OnCurrentThemeChanged(ThemeColor theme_color) {
    ui_->lbIGithubIcon->setPixmap(qTheme.GetGithubIcon());
}

void AboutPage::OnCreditsOrLicenseChecked(bool checked) {
	const auto* sender_ptr = sender();
    if (sender_ptr->objectName() == qTEXT("btnLicense")) {
        ui_->txtBws->setText(license_);
        ui_->btnCredits->setChecked(false);
    }
    else if (sender_ptr->objectName() == qTEXT("btnCredits")) {
        ui_->txtBws->setText(credits_);
        ui_->btnLicense->setChecked(false);
    }
    ui_->txtBws->setVisible(checked);
    ui_->wdtContent->setVisible(!checked);
}

void AboutPage::OnUpdateNewVersion(const Version& version) {    
    ui_->lblAppBuild->setText(qApp->tr("Version ")
        + FormatVersion(version));
    Delay(1);
    ui_->waitForUpdateProcessIndicator->StopAnimation();
    ui_->restartAppButton->show();
}
