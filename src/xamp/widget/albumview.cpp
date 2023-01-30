#include <widget/albumview.h>

#include <widget/widget_shared.h>
#include <widget/scrolllabel.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/actionmap.h>
#include <widget/playlistpage.h>
#include <widget/appsettingnames.h>
#include <widget/xmessagebox.h>
#include <widget/processindicator.h>
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/pixmapcache.h>
#include <widget/ui_utilts.h>
#include <widget/xprogressdialog.h>

#include "thememanager.h"

#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QClipboard>
#include <QStandardPaths>
#include <QFileDialog>
#include <QPainterPath>
#include <QApplication>

enum {
    INDEX_ALBUM = 0,
    INDEX_COVER,
    INDEX_ARTIST,
    INDEX_ALBUM_ID,
    INDEX_ARTIST_ID,
    INDEX_ARTIST_COVER_ID
};

AlbumViewStyledDelegate::AlbumViewStyledDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , text_color_(Qt::black)
	, more_album_opt_button_(new QPushButton())
	, play_button_(new QPushButton())
	, rounded_image_cache_(kMaxAlbumRoundedImageCacheSize) {
    more_album_opt_button_->setStyleSheet(qTEXT("background-color: transparent"));
    play_button_->setStyleSheet(qTEXT("background-color: transparent"));
    mask_image_ = image_utils::RoundDarkImage(qTheme.albumCoverSize());
}

void AlbumViewStyledDelegate::SetTextColor(QColor color) {
    text_color_ = color;
}

void AlbumViewStyledDelegate::ClearImageCache() {
    rounded_image_cache_.Clear();
}

bool AlbumViewStyledDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
    auto* ev = static_cast<QMouseEvent*> (event);
    mouse_point_ = ev->pos();
    auto current_cursor = QApplication::overrideCursor();

    const auto default_cover_size = qTheme.defaultCoverSize();
    constexpr auto icon_size = 24;
    const QRect more_button_rect(
        option.rect.left() + default_cover_size.width() - 10,
        option.rect.top() + default_cover_size.height() + 28,
        icon_size, icon_size);

    switch (ev->type()) {
    case QEvent::MouseButtonPress:
        if (ev->button() == Qt::LeftButton) {
            if (current_cursor != nullptr) {
                if (current_cursor->shape() == Qt::PointingHandCursor) {
                    emit EnterAlbumView(index);
                }
            }
            if (more_button_rect.contains(mouse_point_)) {
                emit ShowAlbumMenu(index, mouse_point_);
            }
        }        
        break;
    default:
        break;
    }
    return true;
}

void AlbumViewStyledDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) {
        return;
    }

    constexpr auto kMoreIconSize = 20;
    QStyle* style = option.widget ? option.widget->style() : QApplication::style();

    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
    painter->setRenderHints(QPainter::TextAntialiasing, true);

    auto album = index.model()->data(index.model()->index(index.row(), 0)).toString();
    auto cover_id = index.model()->data(index.model()->index(index.row(), 1)).toString();
    auto artist = index.model()->data(index.model()->index(index.row(), 2)).toString();
    auto album_id = index.model()->data(index.model()->index(index.row(), 3)).toInt();

    const auto default_cover_size = qTheme.defaultCoverSize();
    const QRect cover_rect(option.rect.left() + 10,
        option.rect.top() + 10,
        default_cover_size.width(), 
        default_cover_size.height());

    auto album_artist_text_width = default_cover_size.width();

    QRect album_text_rect(option.rect.left() + 10,
        option.rect.top() + default_cover_size.height() + 15,
        album_artist_text_width,
        15);
    QRect artist_text_rect(option.rect.left() + 10,
        option.rect.top() + default_cover_size.height() + 35,
        default_cover_size.width(),
        15);

    painter->setPen(QPen(text_color_));

    auto f = painter->font();
#ifdef Q_OS_WIN
    f.setPointSize(8);
#endif
    f.setBold(true);
    painter->setFont(f);

    if (playing_album_id_ > 0 && playing_album_id_ == album_id) {
        const QRect playing_state_icon_rect(
            option.rect.left() + default_cover_size.width() - 10,
            option.rect.top() + default_cover_size.height() + 15,
            kMoreIconSize, kMoreIconSize);

        album_artist_text_width -= kMoreIconSize;
        painter->drawPixmap(playing_state_icon_rect, qTheme.playingIcon().pixmap(QSize(kMoreIconSize, kMoreIconSize)));
    }

    QFontMetrics album_metrics(painter->font());
    painter->drawText(album_text_rect, Qt::AlignVCenter,
        album_metrics.elidedText(album, Qt::ElideRight, album_artist_text_width));

    painter->setPen(QPen(Qt::gray));
    f.setBold(false);
    painter->setFont(f);

    QFontMetrics artist_metrics(painter->font());
    painter->drawText(artist_text_rect, Qt::AlignVCenter,
        artist_metrics.elidedText(artist, Qt::ElideRight, default_cover_size.width() - kMoreIconSize));

    auto album_cover = qPixmapCache.find(cover_id);
    auto album_image = image_utils::RoundImage(album_cover, image_utils::kSmallImageRadius);
    painter->drawPixmap(cover_rect, album_image);

    bool hit_play_button = false;
    if (option.state & QStyle::State_MouseOver && cover_rect.contains(mouse_point_)) {
        painter->drawPixmap(cover_rect, mask_image_);

        constexpr auto kIconSize = 48;
        constexpr auto offset = (kIconSize / 2) - 10;

        const QRect button_rect(
            option.rect.left() + default_cover_size.width() / 2 - offset,
            option.rect.top() + default_cover_size.height() / 2 - offset,
            kIconSize, kIconSize);

        QStyleOptionButton button;
        button.rect = button_rect;
        button.icon = qTheme.playCircleIcon();
        button.state |= QStyle::State_Enabled;
        button.iconSize = QSize(kIconSize, kIconSize);
        style->drawControl(QStyle::CE_PushButton, &button, painter, play_button_.get());

        if (button_rect.contains(mouse_point_)) {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
            hit_play_button = true;
        }
    }
    
    const QRect more_button_rect(
        option.rect.left() + default_cover_size.width() - 10,
        option.rect.top() + default_cover_size.height() + 35,
        kMoreIconSize, kMoreIconSize);
    
    QStyleOptionButton button;
    button.initFrom(more_album_opt_button_.get());
    button.rect = more_button_rect;
    button.icon = qTheme.fontIcon(Glyphs::ICON_MORE);
    button.state |= QStyle::State_Enabled;
    if (more_button_rect.contains(mouse_point_)) {
        button.state |= QStyle::State_Sunken;
        painter->setPen(qTheme.hoverColor());
        painter->setBrush(QBrush(qTheme.hoverColor()));
        painter->drawEllipse(more_button_rect);
    }

    if (more_album_opt_button_->isDefault()) {
        button.features = QStyleOptionButton::DefaultButton;
    }
    button.iconSize = QSize(kMoreIconSize, kMoreIconSize);
    style->drawControl(QStyle::CE_PushButton, &button, painter, more_album_opt_button_.get());
    if (!hit_play_button) {
        QApplication::restoreOverrideCursor();
    }
}

QSize AlbumViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    auto result = QStyledItemDelegate::sizeHint(option, index);
    const auto default_cover = qTheme.defaultCoverSize();
    result.setWidth(default_cover.width() + 30);
    result.setHeight(default_cover.height() + 80);
    return result;
}

AlbumViewPage::AlbumViewPage(QWidget* parent)
    : QFrame(parent) {
    setObjectName(qTEXT("albumViewPage"));
    setFrameStyle(QFrame::StyledPanel);

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setContentsMargins(0, 5, 0, 0);

    auto* close_button = new QPushButton(this);
    close_button->setObjectName(qTEXT("albumViewPageCloseButton"));
    close_button->setStyleSheet(qTEXT(R"(
                                         QPushButton#albumViewPageCloseButton {
                                         border: none;
                                         background-color: transparent;
										 border-radius: 0px;
                                         }
										 QPushButton#albumViewPageCloseButton:hover {	
										 border-radius: 0px;
                                         }
                                         )"));
    close_button->setFixedSize(QSize(24, 24));
    close_button->setIconSize(QSize(13, 13));
    close_button->setIcon(qTheme.fontIcon(Glyphs::ICON_CLOSE_WINDOW));

    auto* hbox_layout = new QHBoxLayout();
    hbox_layout->setSpacing(0);
    hbox_layout->setContentsMargins(0, 0, 0, 0);

    auto* button_spacer = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
    hbox_layout->addWidget(close_button);
    hbox_layout->addSpacerItem(button_spacer);

    page_ = new PlaylistPage(this);
    page_->playlist()->setPlaylistId(kDefaultAlbumPlaylistId, kAppSettingAlbumPlaylistColumnName);

    default_layout->addLayout(hbox_layout);
    default_layout->addWidget(page_);
    default_layout->setStretch(0, 0);
    default_layout->setStretch(1, 1);

    (void)QObject::connect(close_button, &QPushButton::clicked, [this]() {
        emit LeaveAlbumView();
    });

    setStyleSheet(qTEXT("QFrame#albumViewPage { background-color: transparent; }"));
    page_->playlist()->disableDelete();
    page_->playlist()->disableLoadFile();

    auto * fade_effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(fade_effect);
}

void AlbumViewPage::SetPlaylistMusic(const QString& album, int32_t album_id, const QString &cover_id) {
    page_->show();

    page_->playlist()->removeAll();
    ForwardList<int32_t> add_playlist_music_ids;

    qDatabase.ForEachAlbumMusic(album_id,
        [&add_playlist_music_ids](const PlayListEntity& entity) mutable {
            add_playlist_music_ids.push_front(entity.music_id);
        });

    qDatabase.AddMusicToPlaylist(add_playlist_music_ids, page_->playlist()->playlistId());

    page_->playlist()->executeQuery();
    page_->title()->SetText(album);
    page_->SetCoverById(cover_id);

    if (const auto album_stats = qDatabase.GetAlbumStats(album_id)) {
        page_->format()->setText(tr("%1 Tracks, %2, %3, %4")
            .arg(QString::number(album_stats.value().tracks))
            .arg(formatDuration(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year))
            .arg(formatBytes(album_stats.value().file_size))
        );
    }
}

AlbumView::AlbumView(QWidget* parent)
    : QListView(parent)
    , page_(new AlbumViewPage(this))
	, styled_delegate_(new AlbumViewStyledDelegate(this))
	, model_(this) {
    setModel(&model_);
    Refresh();
    setUniformItemSizes(true);
    setDragEnabled(false);
    setSelectionRectVisible(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setWrapping(true);
    setFlow(QListView::LeftToRight);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setFrameStyle(QFrame::StyledPanel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setItemDelegate(styled_delegate_);
    setAutoScroll(false);
    viewport()->setAttribute(Qt::WA_StaticContents);
    setMouseTracking(true);

    qTheme.setBackgroundColor(page_);
    page_->hide();

    (void)QObject::connect(page_,
                            &AlbumViewPage::ClickedArtist,
                            this,
                            &AlbumView::ClickedArtist);

    (void)QObject::connect(page_->playlistPage()->playlist(),
        &PlayListTableView::updatePlayingState,
        this,
        [this](auto entity, auto playing_state) {
            styled_delegate_->SetPlayingAlbumId(playing_state == PlayingState::PLAY_CLEAR ? -1 : entity.album_id);
        });

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::ShowAlbumMenu, [this](auto index, auto pt) {
        ShowMenu(pt);
        });

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::EnterAlbumView, [this](auto index) {
        auto album = GetIndexValue(index, INDEX_ALBUM).toString();
        auto cover_id = GetIndexValue(index, INDEX_COVER).toString();
        auto artist = GetIndexValue(index, INDEX_ARTIST).toString();
        auto album_id = GetIndexValue(index, INDEX_ALBUM_ID).toInt();
        auto artist_id = GetIndexValue(index, INDEX_ARTIST_ID).toInt();
        auto artist_cover_id = GetIndexValue(index, INDEX_ARTIST_COVER_ID).toString();

        const auto list_view_rect = this->rect();
        page_->SetPlaylistMusic(album, album_id, cover_id);
        page_->setFixedSize(QSize(list_view_rect.size().width() - 15, list_view_rect.height() + 25));
        page_->move(QPoint(list_view_rect.x() + 3, 0));

        if (enable_page_) {
            ShowPageAnimation();
            page_->show();
        }

        verticalScrollBar()->hide();
        emit ClickedAlbum(album, album_id, cover_id);
        });

    (void)QObject::connect(page_, &AlbumViewPage::LeaveAlbumView, [this]() {
        verticalScrollBar()->show();
		HidePageAnimation();
        });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        ShowAlbumViewMenu(pt);
    });

    setStyleSheet(qTEXT("background-color: transparent"));

    verticalScrollBar()->setStyleSheet(qTEXT(
        "QScrollBar:vertical { width: 6px; }"
    ));

    auto * fade_effect = page_->graphicsEffect();
    animation_ = new QPropertyAnimation(fade_effect, "opacity");
    QObject::connect(animation_, &QPropertyAnimation::finished, [this]() {
        if (hide_page_) {
            page_->hide();
        }
    });

    update();
}

void AlbumView::HidePageAnimation() {
    animation_->setStartValue(1.0);
    animation_->setEndValue(0.0);
    animation_->setDuration(kPageAnimationDurationMs);
    animation_->setEasingCurve(QEasingCurve::OutCubic);
    animation_->start();
    hide_page_ = true;
}

void AlbumView::ShowPageAnimation() {
    animation_->setStartValue(0.01);
    animation_->setEndValue(1.0);
    animation_->setDuration(kPageAnimationDurationMs);
    animation_->setEasingCurve(QEasingCurve::OutCubic);
    animation_->start();
    hide_page_ = false;
}

void AlbumView::SetPlayingAlbumId(int32_t album_id) {
    styled_delegate_->SetPlayingAlbumId(album_id);
}

void AlbumView::ShowAlbumViewMenu(const QPoint& pt) {
    auto index = indexAt(pt);

    ActionMap<AlbumView> action_map(this);

    auto removeAlbum = [=]() {
        if (!model_.rowCount()) {
            return;
        }

        const auto button = XMessageBox::ShowYesOrNo(tr("Remove all album?"));
        if (button == QDialogButtonBox::Yes) {
            const QScopedPointer<ProcessIndicator> indicator(new ProcessIndicator(this));
            indicator->StartAnimation();
            try {
                qDatabase.ForEachAlbum([](auto album_id) {
                    qDatabase.RemoveAlbum(album_id);
                    });
                qDatabase.RemoveAllArtist();
                update();
                emit RemoveAll();
                qPixmapCache.clear();
            }
            catch (...) {
            }
        }
    };

    auto* load_file_act = action_map.AddAction(tr("Load local file"), [this]() {
        const auto file_name = QFileDialog::getOpenFileName(this,
            tr("Open file"),
            AppSettings::GetMyMusicFolderPath(),
            tr("Music Files ") + GetFileDialogFileExtensions(),
            nullptr);
        if (file_name.isEmpty()) {
            return;
        }
        append(file_name);
        });
    load_file_act->setIcon(qTheme.fontIcon(Glyphs::ICON_LOAD_FILE));

    auto* load_dir_act = action_map.AddAction(tr("Load file directory"), [this]() {
        const auto dir_name = QFileDialog::getExistingDirectory(this,
            tr("Select a directory"),
            AppSettings::GetMyMusicFolderPath(),
            QFileDialog::ShowDirsOnly);
        if (dir_name.isEmpty()) {
            return;
        }
        append(dir_name);
        });
    load_dir_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FOLDER));
    action_map.AddSeparator();
    auto* remove_all_album_act = action_map.AddAction(tr("Remove all album"), [=]() {
        removeAlbum();
        styled_delegate_->ClearImageCache();
        });
    remove_all_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_REMOVE_ALL));

    action_map.exec(pt);
}

void AlbumView::ShowMenu(const QPoint &pt) {
    auto index = indexAt(pt);

    ActionMap<AlbumView> action_map(this);

    auto album = GetIndexValue(index, INDEX_ALBUM).toString();
    auto artist = GetIndexValue(index, INDEX_ARTIST).toString();
    auto album_id = GetIndexValue(index, INDEX_ALBUM_ID).toInt();
    auto artist_id = GetIndexValue(index, INDEX_ARTIST_ID).toInt();
    auto artist_cover_id = GetIndexValue(index, INDEX_ARTIST_COVER_ID).toString();

    auto* add_album_to_playlist_act = action_map.AddAction(tr("Add album to playlist"), [=]() {
        ForwardList<PlayListEntity> entities;
		ForwardList<int32_t> add_playlist_music_ids;
        qDatabase.ForEachAlbumMusic(album_id,
            [&entities, &add_playlist_music_ids](const PlayListEntity& entity) mutable {
                if (entity.track_loudness == 0.0) {
                    entities.push_front(entity);
                }
                add_playlist_music_ids.push_front(entity.music_id);
            });
        emit AddPlaylist(add_playlist_music_ids, entities);
        });
    add_album_to_playlist_act->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAYLIST));

    auto* copy_album_act = action_map.AddAction(tr("Copy album"), [album]() {
        QApplication::clipboard()->setText(album);
    });
    copy_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_COPY));

    action_map.AddAction(tr("Copy artist"), [artist]() {
        QApplication::clipboard()->setText(artist);
    });

    action_map.AddSeparator();

    auto remove_select_album_act = action_map.AddAction(tr("Remove select album"), [=]() {
        const auto button = XMessageBox::ShowYesOrNo(tr("Remove the album?"));
		if (button == QDialogButtonBox::Yes) {
            qDatabase.RemoveAlbum(album_id);
            Refresh();
		}
    });
    remove_select_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_REMOVE_ALL));

    action_map.exec(pt);
}

void AlbumView::EnablePage(bool enable) {
    enable_page_ = enable;
}

void AlbumView::OnThemeChanged(QColor backgroundColor, QColor color) {
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->SetTextColor(color);
    page_->setStyleSheet(backgroundColorToString(backgroundColor));
}

void AlbumView::SetFilterByArtistId(int32_t artist_id) {
    model_.setQuery(qSTR(R"(
    SELECT
        album,
        albums.coverId,
        artist,
        albums.albumId,
        artists.artistId,
        artists.coverId
    FROM
        albumArtist
    LEFT JOIN
        albums ON albums.albumId = albumArtist.albumId
    LEFT JOIN
        artists ON artists.artistId = albumArtist.artistId
    WHERE
        (artists.artistId = '%1') AND (albums.isPodcast = 0)
    )").arg(artist_id), qDatabase.database());
}

void AlbumView::update() {
    model_.setQuery(qTEXT(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
WHERE 
	albums.isPodcast = 0
    )"), qDatabase.database());
}

void AlbumView::Refresh() {
    update();
}

void AlbumView::OnSearchTextChanged(const QString& text) {
    QString query(qTEXT(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId
FROM
    albums
    LEFT JOIN artists ON artists.artistId = albums.artistId
WHERE
    (
    albums.album LIKE '%%1%' OR artists.artist LIKE '%%1%'
    ) AND (albums.isPodcast = 0)
LIMIT 200
    )"));
    model_.setQuery(query.arg(text), qDatabase.database());
}

void AlbumView::append(const QString& file_name) {
    read_progress_dialog_ =
        MakeProgressDialog(tr("Read track information"),
            tr("Read track information"),
            tr("Cancel"));
    ReadSingleFileTrackInfo(file_name);
}

void AlbumView::ReadSingleFileTrackInfo(const QString& file_name) {
    const auto adapter = QSharedPointer<DatabaseFacade>(new DatabaseFacade());

    (void)QObject::connect(adapter.get(),
        &DatabaseFacade::ReadCompleted,
        this,
        &AlbumView::ProcessTrackInfo);

    (void)QObject::connect(adapter.get(),
        &DatabaseFacade::ReadFileStart,
        this, &AlbumView::OnReadFileStart);

    (void)QObject::connect(adapter.get(),
        &DatabaseFacade::ReadFileProgress,
        this, &AlbumView::OnReadFileProgress);

    (void)QObject::connect(adapter.get(),
        &DatabaseFacade::ReadFileEnd,
        this, &AlbumView::OnReadFileEnd);

    emit ReadTrackInfo(adapter, file_name, -1, false);
}

void AlbumView::OnReadFileStart(int dir_size) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->SetRange(0, dir_size);
}

void AlbumView::OnReadFileProgress(const QString& dir, int progress) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->SetLabelText(dir);
    read_progress_dialog_->SetValue(progress);
}

void AlbumView::OnReadFileEnd() {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_.reset();
}

void AlbumView::ProcessTrackInfo(const ForwardList<TrackInfo>& ) {
    emit LoadCompleted();
}

void AlbumView::HideWidget() {
    if (!page_) {
        return;
    }
    page_->hide();
}

void AlbumView::resizeEvent(QResizeEvent* event) {
    if (page_ != nullptr) {
        if (!page_->isHidden()) {
            const auto list_view_rect = this->rect();
            page_->setFixedSize(QSize(list_view_rect.size().width() - 15, list_view_rect.height() + 15));
            page_->move(QPoint(list_view_rect.x() + 3, 0));
        }
    }    
    QListView::resizeEvent(event);
}
