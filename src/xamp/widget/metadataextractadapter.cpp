#include <atomic>
#include <utility>
#include <execution>
#include <forward_list>
#include <fstream>
#include <QMap>
#include <QDirIterator>
#include <QProgressDialog>
#include <unordered_set>

#include <base/base.h>
#include <base/str_utilts.h>
#include <base/threadpool.h>
#include <base/bom.h>
#include <base/logger_impl.h>

#include <metadata/api.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadataextractadapter.h>

#include <player/audio_util.h>

#include "thememanager.h"

#include <widget/cuesheet.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/http.h>
#include <widget/toast.h>
#include <widget/database.h>
#include <widget/appsettings.h>
#include <widget/playlisttableview.h>
#include <widget/pixmapcache.h>
#include <widget/metadataextractadapter.h>

class DatabaseIdCache final {
public:
    DatabaseIdCache()
        : cover_reader_(MakeMetadataReader()) {
    }

    QPixmap getEmbeddedCover(const Metadata& metadata) const;

    std::tuple<int32_t, int32_t, QString> addOrGetAlbumAndArtistId(int64_t dir_last_write_time,
        const QString& album,
        const QString& artist,
        bool is_podcast,
        const QString& disc_id) const;

    QString addCoverCache(int32_t album_id, const QString& album, const Metadata& metadata, bool is_unknown_album) const;

private:
    mutable LruCache<int32_t, QString> cover_id_cache_;
    // Key: Album + Artist
    mutable LruCache<QString, int32_t> album_id_cache_;
    mutable LruCache<QString, int32_t> artist_id_cache_;
    AlignPtr<IMetadataReader> cover_reader_;
};

QPixmap DatabaseIdCache::getEmbeddedCover(const Metadata& metadata) const {
    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(metadata.file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(),
            static_cast<uint32_t>(buffer.size()));
    }
    return pixmap;
}

QString DatabaseIdCache::addCoverCache(int32_t album_id, const QString& album, const Metadata& metadata, bool is_unknown_album) const {
    auto cover_id = qDatabase.getAlbumCoverId(album_id);
    if (!cover_id.isEmpty()) {
    	return cover_id;
    }

    QPixmap pixmap;
    const auto& buffer = cover_reader_->GetEmbeddedCover(metadata.file_path);
    if (!buffer.empty()) {
        pixmap.loadFromData(buffer.data(), static_cast<uint32_t>(buffer.size()));
    }
    else {
    	if (!is_unknown_album) {
            pixmap = PixmapCache::findFileDirCover(QString::fromStdWString(metadata.file_path));
    	}          
    }

    if (!pixmap.isNull()) {
        cover_id = qPixmapCache.savePixamp(pixmap);
        XAMP_ASSERT(!cover_id.isEmpty());
        cover_id_cache_.AddOrUpdate(album_id, cover_id);
        qDatabase.setAlbumCover(album_id, album, cover_id);
    }	
    return cover_id;
}

std::tuple<int32_t, int32_t, QString> DatabaseIdCache::addOrGetAlbumAndArtistId(int64_t dir_last_write_time,
    const QString &album,
    const QString &artist, 
    bool is_podcast,
    const QString& disc_id) const {
#if 1
    int32_t artist_id = 0;
    if (auto const * artist_id_op = this->artist_id_cache_.Find(artist)) {
        artist_id = *artist_id_op;
    }
    else {
        artist_id = qDatabase.addOrUpdateArtist(artist);
        this->artist_id_cache_.AddOrUpdate(artist, artist_id);
    }

    int32_t album_id = 0;
    if (auto const* album_id_op = this->album_id_cache_.Find(album + artist)) {
        album_id = *album_id_op;
    }
    else {
        album_id = qDatabase.addOrUpdateAlbum(album, artist_id, dir_last_write_time, is_podcast, disc_id);
        this->album_id_cache_.AddOrUpdate(album + artist, album_id);
    }

    QString cover_id;
    if (auto const* cover_id_op = this->cover_id_cache_.Find(album_id)) {
        cover_id = *cover_id_op;
    }
#else
    auto artist_id = qDatabase.addOrUpdateArtist(artist);
    auto album_id = qDatabase.addOrUpdateAlbum(album, artist_id, dir_last_write_time, is_podcast, disc_id);
    QString cover_id;
#endif
    return std::make_tuple(album_id, artist_id, cover_id);
}

class ExtractAdapterProxy final : public IMetadataExtractAdapter {
public:
    static constexpr std::wstring_view kCueFileExt{ L".cue" };

    explicit ExtractAdapterProxy(const QSharedPointer<::MetadataExtractAdapter> &adapter)
        : adapter_(adapter) {
        GetSupportFileExtensions();
    }

    [[nodiscard]] bool IsAccept(Path const& path) const noexcept override {
        if (!path.has_extension()) {
            return false;
        }
        /*if (path.extension() == kCueFileExt) {
            return true;
        }*/
        using namespace audio_util;
        const auto file_ext = String::ToLower(path.extension().string());
        const auto & support_file_set = GetSupportFileExtensions();
        return support_file_set.find(file_ext) != support_file_set.end();
    }

    XAMP_DISABLE_COPY(ExtractAdapterProxy)

    void OnWalkNew() override {
        qApp->processEvents();
    }

    void OnWalk(const Path&, Metadata metadata) override {
        /*if (metadata.file_ext == kCueFileExt) {
            std::wifstream file(metadata.file_path, std::ios::binary);
            ImbueFileFromBom(file);
            if (file.is_open()) {
                auto sheet = std::make_shared<CueSheet>();
                sheet->Parse(file.rdbuf());
            }
        }*/
        metadatas_.push_front(std::move(metadata));
        qApp->processEvents();
    }

    void OnWalkEnd(DirectoryEntry const& dir_entry) override {
        if (metadatas_.empty()) {
            return;
        }
        metadatas_.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });
        emit adapter_->readCompleted(ToTime_t(dir_entry.last_write_time()), metadatas_);
        metadatas_.clear();        
    }
	
private:
    QSharedPointer<::MetadataExtractAdapter> adapter_;
    ForwardList<Metadata> metadatas_;
};

::MetadataExtractAdapter::MetadataExtractAdapter(QObject* parent)
    : QObject(parent) {    
}

void ::MetadataExtractAdapter::readFileMetadata(const QSharedPointer<MetadataExtractAdapter>& adapter, QString const & file_path, bool show_progress_dialog, bool is_recursive) {
	const auto dialog = 
        makeProgressDialog(tr("Read file metadata"),
	    tr("Read progress dialog"), 
	    tr("Cancel"));

    if (show_progress_dialog) {
        dialog->show();
    }

    dialog->setMinimumDuration(1000);
    dialog->setWindowModality(Qt::ApplicationModal);

    HashSet<Path> dirs;
    dirs.reserve(100);

    const auto is_accept = [&dirs](auto path) {
        return Fs::is_directory(path) && dirs.find(path) == dirs.end();
    };
    const auto walk = [&dirs](auto path) {
        dirs.emplace(path);
    };
    const auto walk_end = [&dirs](auto path, bool is_new) {
        dirs.emplace(path);
    };

    ScanFolder(file_path.toStdWString(), is_accept, walk, walk_end, true);
    dialog->setMaximum(dirs.size());

    ExtractAdapterProxy proxy(adapter);

    const auto reader = MakeMetadataReader();
    auto progress = 0;

    if (dirs.empty()) {
        dirs.insert(file_path.toStdWString());
    }

    for (const auto& file_dir_or_path : dirs) {
        if (dialog->wasCanceled()) {
            return;
        }

        dialog->setLabelText(QString::fromStdWString(file_dir_or_path.wstring()));
        
        try {
            ScanFolder(file_dir_or_path, &proxy, reader.get(), is_recursive);
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("WalkPath has exception: {}", e.what());
        }
        dialog->setValue(progress++);
    }
}

void ::MetadataExtractAdapter::addMetadata(const ForwardList<Metadata>& result, PlayListTableView* playlist, int64_t dir_last_write_time, bool is_podcast) {
	auto playlist_id = -1;
	if (playlist != nullptr) {
		playlist_id = playlist->playlistId();
	}

	const DatabaseIdCache cache;

	for (const auto& metadata : result) {
		qApp->processEvents();

		auto album = QString::fromStdWString(metadata.album);
		auto artist = QString::fromStdWString(metadata.artist);
		auto disc_id = QString::fromStdString(metadata.disc_id);
     
		auto is_unknown_album = false;
		if (album.isEmpty()) {
			album = tr("Unknown album");
			is_unknown_album = true;
			// todo: 如果有內建圖片就把當作一張專輯.
			auto cover = cache.getEmbeddedCover(metadata);
			if (!cover.isNull()) {
				album = QString::fromStdWString(metadata.file_name_no_ext);
			}
		}
		if (artist.isEmpty()) {
			artist = tr("Unknown artist");
		}

		const auto music_id = qDatabase.addOrUpdateMusic(metadata);

		auto [album_id, artist_id, cover_id] = 
            cache.addOrGetAlbumAndArtistId(dir_last_write_time,
		             album, 
		             artist,
		             is_podcast, 
		             disc_id);

		if (playlist_id != -1) {            
			qDatabase.addMusicToPlaylist(music_id, playlist_id, album_id);
		}

		if (metadata.cover_id.empty()) {
			// Find cover id from database.
			if (cover_id.isEmpty()) {
				cover_id = qDatabase.getAlbumCoverId(album_id);
			}

			// Database not exist find others.
			if (cover_id.isEmpty()) {
				cover_id = cache.addCoverCache(album_id, album, metadata, is_unknown_album);
			}
		} else {
			qDatabase.setAlbumCover(album_id, album, QString::fromStdString(metadata.cover_id));
		}

		qDatabase.addOrUpdateAlbumMusic(album_id,
		                                artist_id,
		                                music_id);
	}

	if (playlist != nullptr) {
		playlist->excuteQuery();
	}
}

void ::MetadataExtractAdapter::processMetadata(const ForwardList<Metadata>& result, PlayListTableView* playlist, int64_t dir_last_write_time) {
    if (dir_last_write_time == -1) {
        dir_last_write_time = QDateTime::currentSecsSinceEpoch();
    }
    auto is_podcast_mode = false;
    if (playlist != nullptr) {
        is_podcast_mode = playlist->isPodcastMode();
    }
    try {
        qDatabase.transaction();
        addMetadata(result, playlist, dir_last_write_time, is_podcast_mode);
        qDatabase.commit();
    } catch (std::exception const &e) {
        qDatabase.rollback();
        XAMP_LOG_DEBUG("processMetadata throw exception! {}", e.what());
    }
}

Metadata getMetadata(QString const& file_path) {
    const Path path(file_path.toStdWString());
    auto reader = MakeMetadataReader();
    return reader->Extract(path);
}
