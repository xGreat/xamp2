#include <player/ebur128reader.h>

#include <widget/widget_shared.h>
#include <widget/pixmapcache.h>
#include <base/logger_impl.h>

#if defined(Q_OS_WIN)
#include <player/mbdiscid.h>
#endif

#include <widget/metadataextractadapter.h>
#include <widget/podcast_uiltis.h>
#include <widget/http.h>
#include <widget/str_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/stackblur.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/appsettings.h>
#include <widget/widget_shared.h>
#include <widget/backgroundworker.h>

XAMP_DECLARE_LOG_NAME(BackgroundThreadPool);
XAMP_DECLARE_LOG_NAME(BackgroundWorker);

struct ReplayGainWorkerEntity {
    ReplayGainWorkerEntity(PlayListEntity item, std::optional<Ebur128Reader> scanner)
        : item(std::move(item))
        , scanner(std::move(scanner)) {
    }
    PlayListEntity item;
    std::optional<Ebur128Reader> scanner;
};

BackgroundWorker::BackgroundWorker()
	: blur_img_cache_(8) {
    writer_ = MakeMetadataWriter();
    logger_ = LoggerManager::GetInstance().GetLogger(kBackgroundWorkerLoggerName);
}

BackgroundWorker::~BackgroundWorker() = default;

void BackgroundWorker::stopThreadPool() {
    is_stop_ = true;
    if (executor_ != nullptr) {
        executor_->Stop();
    }
}

void BackgroundWorker::lazyInit() {
    if (executor_ != nullptr) {
        return;
    }
    executor_ = MakeThreadPoolExecutor(kBackgroundThreadPoolLoggerName,
        TaskSchedulerPolicy::LEAST_LOAD_POLICY,
        TaskStealPolicy::CONTINUATION_STEALING_POLICY);
}

void BackgroundWorker::onFetchCdInfo(const DriveInfo& drive) {
#if defined(Q_OS_WIN)
    MBDiscId mbdisc_id;
    std::string disc_id;
    std::string url;

    try {
        disc_id = mbdisc_id.GetDiscId(drive.drive_path.toStdString());
        url = mbdisc_id.GetDiscIdLookupUrl(drive.drive_path.toStdString());
    } catch (Exception const &e) {
        XAMP_LOG_DEBUG(e.GetErrorMessage());
        return;
    }

    try {
	    ForwardList<TrackInfo> track_infos;
	    auto cd = OpenCD(drive.driver_letter);
        cd->SetMaxSpeed();
        const auto tracks = cd->GetTotalTracks();

        auto track_id = 0;
        for (const auto& track : tracks) {
            auto metadata = getMetadata(QString::fromStdWString(track));
            metadata.file_path = tracks[track_id];
            metadata.duration = cd->GetDuration(track_id++);
            metadata.samplerate = 44100;
            metadata.disc_id = disc_id;
            track_infos.push_front(metadata);
        }

        track_infos.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        emit updateCdMetadata(QString::fromStdString(disc_id), track_infos);
    }
    catch (Exception const& e) {
        XAMP_LOG_DEBUG(e.GetErrorMessage());
        return;
    }

    XAMP_LOG_D(logger_, "Start fetch cd information form musicbrainz.");

    http::HttpClient(QString::fromStdString(url))
        .success([this, disc_id](const QString& content) {
        auto [image_url, mb_disc_id_info] = parseMbDiscIdXML(content);

        mb_disc_id_info.disc_id = disc_id;
        mb_disc_id_info.tracks.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        emit updateMbDiscInfo(mb_disc_id_info);

        XAMP_LOG_D(logger_, "Start fetch cd cover image.");

        http::HttpClient(QString::fromStdString(image_url))
            .success([this, disc_id](const QString& content) {
	            const auto cover_url = parseCoverUrl(content);
                http::HttpClient(cover_url).download([this, disc_id](const auto& content) mutable {
                    auto cover_id = qPixmapCache.addOrUpdate(content);
					XAMP_LOG_D(logger_, "Download cover image completed.");
                    emit updateDiscCover(QString::fromStdString(disc_id), cover_id);
                    });				
                }).get();
            }).get();
#endif
}

void BackgroundWorker::onBlurImage(const QString& cover_id, const QImage& image) {
    lazyInit();

    if (auto *cache_image = blur_img_cache_.Find(cover_id)) {
        XAMP_LOG_D(logger_, "Found blur image in cache!");
        emit updateBlurImage(cache_image->copy());
        return;
    }

    if (!AppSettings::getValueAsBool(kEnableBlurCover)) {
        emit updateBlurImage(QImage());
        return;
    }
    auto temp = image.copy();
    Stackblur blur(*executor_, temp, 50);
    emit updateBlurImage(temp.copy());
    blur_img_cache_.AddOrUpdate(cover_id, std::move(temp));
}

void BackgroundWorker::onDownloadPodcast() {
    XAMP_LOG_D(logger_, "Start download podcast.xml");

    auto download_json_file_error = [this](const QString& msg) {
        XAMP_LOG_D(logger_, "Download podcast error! {}", msg.toStdString());
        emit downloadPodcastCompleted({}, {});
    };

    auto download_cover_image_error = [this](const QString& msg) {
        XAMP_LOG_D(logger_, "Download podcast image error! {}", msg.toStdString());
        emit downloadPodcastCompleted({}, {});
    };

    http::HttpClient(qTEXT("https://suisei-podcast.outv.im/meta.json"))
        .error(download_json_file_error)
        .success([this, download_cover_image_error](const QString& json) {
			XAMP_LOG_D(logger_, "Download meta.json ({}) success!", String::FormatBytes(json.size()));

            Stopwatch sw;
            std::string image_url("https://cdn.jsdelivr.net/gh/suisei-cn/suisei-podcast@0423b62/logo/logo-202108.jpg");
            auto const podcast_info = std::make_pair(image_url, parseJson(json));
            XAMP_LOG_D(logger_, "Parse meta.json success! {}sec", sw.ElapsedSeconds());
            sw.Reset();

            XAMP_LOG_D(logger_, "Start download podcast image file");
            http::HttpClient(QString::fromStdString(podcast_info.first))
                .error(download_cover_image_error)
                .download([this, podcast_info](const QByteArray& data) {
					XAMP_LOG_D(logger_, "Download podcast image file ({}) success!", String::FormatBytes(data.size()));
					emit downloadPodcastCompleted(podcast_info.second, data);
                    });
            }).get();
}

void BackgroundWorker::onReadReplayGain(bool, const ForwardList<PlayListEntity>& entities) {
    lazyInit();

    auto entities_size = std::distance(entities.begin(), entities.end());
    XAMP_LOG_D(logger_, "Start read replay gain count:{}", entities_size);

    Vector<Task<>> replay_gain_tasks;

    const auto target_loudness = AppSettings::getValue(kAppSettingReplayGainTargetLoudnes).toDouble();
    const auto scan_mode = AppSettings::getAsEnum<ReplayGainScanMode>(kAppSettingReplayGainScanMode);

    replay_gain_tasks.reserve(entities_size);
    RcuPtr<Vector<std::shared_ptr<ReplayGainWorkerEntity>>> replay_gain_result(
        std::make_shared<Vector<std::shared_ptr<ReplayGainWorkerEntity>>>());

    for (const auto &item : entities) {
        replay_gain_tasks.push_back(Executor::Spawn(*executor_, [scan_mode, item, this, &replay_gain_result]() {
            auto progress = [scan_mode](auto p) {
                if (scan_mode == ReplayGainScanMode::RG_SCAN_MODE_FAST && p > 50) {
                    return false;
                }
                return true;
            };

            std::optional<Ebur128Reader> scanner;
            auto prepare = [&scanner](auto const& input_format) mutable {
                scanner = Ebur128Reader(input_format.GetSampleRate());
            };

            auto dps_process = [&scanner, this](auto const* samples, auto sample_size) {
                if (is_stop_) {
                    return;
                }
                scanner.value().Process(samples, sample_size);
            };

            read_utiltis::readAll(item.file_path.toStdWString(), progress, prepare, dps_process);
            replay_gain_result.copy_update([&](auto* result) {
                result->emplace_back(std::make_shared<ReplayGainWorkerEntity>(item, std::move(scanner)));
                });
        }));
    }

    Vector<Ebur128Reader> scanners;
    ReplayGainResult replay_gain;

    for (auto& task : replay_gain_tasks) {
        try {
            task.get();
        } catch (Exception const &) {
        }
    }

    auto copy_result = *replay_gain_result.read();
    for (auto& result : copy_result) {
        try {
            replay_gain.music_id.push_back((*result).item);
            if ((*result).scanner.has_value()) {
                scanners.push_back(std::move((*result).scanner.value()));
            }
        }
        catch (std::exception const& e) {
            XAMP_LOG_D(logger_, "ReplayGain got exception! {}", e.what());
        }
    }

    if (scanners.empty() || replay_gain_tasks.size() != scanners.size()) {
        XAMP_LOG_D(logger_, "ReplayGain no more work item!");
        return;
    }

    replay_gain.track_peak.reserve(entities_size);
    replay_gain.track_loudness.reserve(entities_size);
    replay_gain.track_gain.reserve(entities_size);

    replay_gain.album_loudness = Ebur128Reader::GetMultipleLoudness(scanners);

    for (size_t i = 0; i < entities_size; ++i) {
        auto track_loudness = scanners[i].GetLoudness();
        auto track_peak = scanners[i].GetTruePeek();
        replay_gain.track_peak.push_back(track_peak);
        replay_gain.track_peak_gain.push_back(20.0 * log10(track_peak));
        replay_gain.track_loudness.push_back(track_loudness);
        replay_gain.track_gain.push_back(target_loudness - track_loudness);
    }

    replay_gain.album_peak = *std::max_element(
        replay_gain.track_peak.begin(),
        replay_gain.track_peak.end(),
        [](auto& a, auto& b) {return a < b; }
    );

    replay_gain.album_gain = target_loudness - replay_gain.album_loudness;
    replay_gain.album_peak_gain = 20.0 * log10(replay_gain.album_peak);

    for (size_t i = 0; i < entities_size; ++i) {
        ReplayGain rg;
        rg.album_gain = replay_gain.album_gain;
        rg.track_gain = replay_gain.track_gain[i];
        rg.album_peak = replay_gain.album_peak;
        rg.track_peak = replay_gain.track_peak[i];
        rg.ref_loudness = target_loudness;

        writer_->WriteReplayGain(replay_gain.music_id[i].file_path.toStdWString(), rg);
        emit updateReplayGain(replay_gain.music_id[i],
            replay_gain.track_loudness[i],
            replay_gain.album_gain,
            replay_gain.album_peak,
            replay_gain.track_gain[i],
            replay_gain.track_peak[i]
        );
    }
}
