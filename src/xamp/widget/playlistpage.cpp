#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>

#include "thememanager.h"
#include <widget/image_utiltis.h>
#include <widget/pixmapcache.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/scrolllabel.h>
#include <widget/playlisttableview.h>
#include <widget/playlistpage.h>

PlaylistPage::PlaylistPage(QWidget* parent)
	: QFrame(parent) {
	initial();
}

void PlaylistPage::initial() {
	auto* default_layout = new QVBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 0, 0, 0);

	auto* child_layout = new QHBoxLayout();
	child_layout->setSpacing(0);
	child_layout->setObjectName(QString::fromUtf8("horizontalLayout_7"));
	child_layout->setContentsMargins(20, -1, 20, -1);
	auto* left_space_layout = new QVBoxLayout();
	left_space_layout->setSpacing(0);
	left_space_layout->setObjectName(QString::fromUtf8("verticalLayout_3"));
	auto* vertical_spacer = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	left_space_layout->addItem(vertical_spacer);

	cover_ = new QLabel();
	cover_->setObjectName(QString::fromUtf8("label"));
	cover_->setMinimumSize(QSize(150, 150));
	cover_->setMaximumSize(QSize(150, 150));
	cover_->setAttribute(Qt::WA_StaticContents);

	left_space_layout->addWidget(cover_);

	child_layout->addLayout(left_space_layout);

	auto* horizontal_spacer = new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

	child_layout->addItem(horizontal_spacer);

	auto* album_title_layout = new QVBoxLayout();
	album_title_layout->setSpacing(0);
	album_title_layout->setObjectName(QString::fromUtf8("verticalLayout_2"));
	album_title_layout->setContentsMargins(0, 5, -1, -1);
    title_ = new ScrollLabel(this);
	auto f = font();
	f.setWeight(QFont::DemiBold);
	f.setPointSize(18);
	title_->setFont(f);
	title_->setObjectName(QString::fromUtf8("label_2"));
	title_->setMinimumSize(QSize(0, 40));
	title_->setMaximumSize(QSize(16777215, 40));

	format_ = new QLabel(this);
	QFont format_font(Q_TEXT("FormatFont"));
	format_->setFont(format_font);

	album_title_layout->addWidget(title_);

	auto* middle_spacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
	album_title_layout->addItem(middle_spacer);

	album_title_layout->addWidget(format_);

	auto* right_spacer = new QSpacerItem(20, 108, QSizePolicy::Minimum, QSizePolicy::Expanding);
	album_title_layout->addItem(right_spacer);


	child_layout->addLayout(album_title_layout);

	child_layout->setStretch(2, 1);

	default_layout->addLayout(child_layout);

	auto* default_spacer = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	default_layout->addItem(default_spacer);

	auto* horizontalLayout_8 = new QHBoxLayout();
	horizontalLayout_8->setSpacing(0);
	horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
	auto* horizontalSpacer_4 = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	horizontalLayout_8->addItem(horizontalSpacer_4);

	playlist_ = new PlayListTableView(this);
	playlist_->setObjectName(QString::fromUtf8("tableView"));

	horizontalLayout_8->addWidget(playlist_);

	auto* horizontalSpacer_5 = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	horizontalLayout_8->addItem(horizontalSpacer_5);

	horizontalLayout_8->setStretch(1, 1);

	default_layout->addLayout(horizontalLayout_8); 

	default_layout->setStretch(2, 1);

	setStyleSheet(Q_TEXT("background-color: transparent;"));

	(void)QObject::connect(playlist_,
		&PlayListTableView::updateAlbumCover,
		this,
		&PlaylistPage::setCoverById);
}

void PlaylistPage::OnThemeColorChanged(QColor theme_color, QColor color) {
	title_->setStyleSheet(Q_TEXT("QLabel { color: ") + colorToString(color) + Q_TEXT("; background-color: transparent; }"));
	format_->setStyleSheet(Q_TEXT("QLabel { font-family: FormatFont; font-size: 16px; color: ") + colorToString(color) + Q_TEXT("; background-color: transparent; }"));
}

QLabel* PlaylistPage::format() {
	return format_;
}

ScrollLabel* PlaylistPage::title() {
	return title_;
}

QLabel* PlaylistPage::cover() {
	return cover_;
}

void PlaylistPage::setCover(const QPixmap * cover) {
	const auto playlist_cover = Pixmap::roundImage(
		Pixmap::scaledImage(*cover, QSize(130, 130), false),
		Pixmap::kPlaylistImageRadius);
	cover_->setPixmap(playlist_cover);
}

void PlaylistPage::setCoverById(const QString& cover_id) {
	const auto* cover = qPixmapCache.find(cover_id);
	if (cover != nullptr) {
		setCover(cover);
	} else {
		setCover(&qTheme.unknownCover());
	}
}

PlayListTableView* PlaylistPage::playlist() {
    return playlist_;
}
