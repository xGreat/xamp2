#include <metadata/taglibmetareader.h>

#include <metadata/taglib.h>

#include <base/stl.h>
#include <base/str_utilts.h>
#include <base/memory.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/singleton.h>
#include <base/exception.h>

#include <vector>
#include <sstream>
#include <functional>

namespace xamp::metadata {

static double ParseStringList(std::string const& s, bool string_dummy = true) {
    std::stringstream ss;
    ss << s;
    if (string_dummy) {
        std::string dummy;
        ss >> dummy;
    }
    double d = 0;
    ss >> d;
    return d;
}

static bool GetID3V2TagCover(ID3v2::Tag* tag, Vector<uint8_t>& buffer) {
    if (!tag) {
        return false;
    }
    auto const & frame_list = tag->frameList("APIC");
    if (frame_list.isEmpty()) {
        return false;
    }

    const auto * frame = dynamic_cast<ID3v2::AttachedPictureFrame*>(frame_list.front());
    if (!frame) {
        return false;
    }
    buffer.resize(frame->picture().size());
    MemoryCopy(buffer.data(), frame->picture().data(), static_cast<int32_t>(frame->picture().size()));
    return true;
}

static bool GetApeTagCover(APE::Tag* tag, Vector<uint8_t>& buffer) {
    auto const & list_map = tag->itemListMap();

    if (!list_map.contains("COVER ART (FRONT)")) {
        return false;
    }

    const ByteVector null_string_terminator(1, 0);
    auto item = list_map["COVER ART (FRONT)"].binaryData();
    auto pos = item.find(null_string_terminator);	// Skip the filename
    if (++pos > 0) {
        auto pic = item.mid(pos);
        buffer.resize(pic.size());
        MemoryCopy(buffer.data(), pic.data(), static_cast<int32_t>(pic.size()));
        return true;
    }
    return false;
}

static std::optional<ReplayGain> GetMp3ReplayGain(File* file) {
    ReplayGain replay_gain;
    bool found = false;
    if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
        if (auto* tag = mp3_file->ID3v2Tag(false)) {
            const auto& frame_list = tag->frameList("TXXX");
            for (auto* it : frame_list) {
                auto* fr = dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(it);
                if (fr) {
                    const auto desc = fr->description().upper();
                    const auto value = ParseStringList(fr->fieldList().toString().to8Bit());
                    if (desc == String::AsStdString(kReplaygainAlbumGain)) {
                        replay_gain.album_gain = value;
                        found = true;
                    }
                    else if (desc == String::AsStdString(kReplaygainTrackGain)) {
                        replay_gain.track_gain = value;
                        found = true;
                    }
                    else if (desc == String::AsStdString(kReplaygainAlbumPeak)) {
                        replay_gain.album_peak = value;
                        found = true;
                    }
                    else if (desc == String::AsStdString(kReplaygainTrackPeak)) {
                        replay_gain.track_peak = value;
                        found = true;
                    }
                    else if (desc == String::AsStdString(kReplaygainReferenceLoudness)) {
                        replay_gain.ref_loudness = value;
                        found = true;
                    }
                }
            }
        }
    }
    if (!found) {
        return std::nullopt;
    }
    return replay_gain;
}

static bool GetMp3Cover(File* file, Vector<uint8_t>& buffer) {
    auto found = false;
    if (auto * mpeg_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
        if (mpeg_file->ID3v2Tag()) {
            found = GetID3V2TagCover(mpeg_file->ID3v2Tag(), buffer);
        }
        if (!found && mpeg_file->APETag()) {
            GetApeTagCover(mpeg_file->APETag(), buffer);
        }
    }
    return found;
}

static bool GetDSFCover(File* file, Vector<uint8_t>& buffer) {
    auto found = false;
    if (const auto* dsd_file = dynamic_cast<TagLib::DSF::File*>(file)) {
        if (dsd_file->tag()) {
            found = GetID3V2TagCover(dsd_file->tag(), buffer);
        }
    }
    return found;
}

static bool GetDSDIFFCover(File* file, Vector<uint8_t>& buffer) {
    auto found = false;
    if (const auto* dsd_file = dynamic_cast<TagLib::DSDIFF::File*>(file)) {
        if (dsd_file->ID3v2Tag()) {
            found = GetID3V2TagCover(dsd_file->ID3v2Tag(), buffer);
        }
    }
    return found;
}

static std::optional<ReplayGain> GetMp4ReplayGain(File* file) {
    ReplayGain replay_gain;
    auto found = false;
    if (const auto* mp4_file = dynamic_cast<TagLib::MP4::File*>(file)) {
        if (const auto* tag = mp4_file->tag()) {
            auto const& dict = tag->itemMap();
            const auto track_gain = String::AsStdString(kITunesReplaygainTrackGain);
            if (dict.contains(track_gain)) {
                replay_gain.track_gain = ParseStringList(dict[track_gain].toStringList()[0].to8Bit(), false);
                found = true;
            }
            const auto track_peak = String::AsStdString(kITunesReplaygainTrackPeak);
            if (dict.contains(track_peak)) {
                replay_gain.track_peak = ParseStringList(dict[track_peak].toStringList()[0].to8Bit(), false);
                found = true;
            }
            const auto album_gain = String::AsStdString(kITunesReplaygainAlbumGain);
            if (dict.contains(album_gain)) {
                replay_gain.album_gain = ParseStringList(dict[album_gain].toStringList()[0].to8Bit(), false);
                found = true;
            }
            const auto album_peak = String::AsStdString(kITunesReplaygainAlbumPeak);
            if (dict.contains(String::AsStdString(album_peak))) {
                replay_gain.album_peak = ParseStringList(dict[album_peak].toStringList()[0].to8Bit(), false);
                found = true;
            }
            const auto reference_loudness = String::AsStdString(kITunesReplaygainReferenceLoudness);
            if (dict.contains(reference_loudness)) {
                replay_gain.album_peak = ParseStringList(dict[reference_loudness].toStringList()[0].to8Bit(), false);
                found = true;
            }
        }
    }
    if (!found) {
        return std::nullopt;
    }
    return replay_gain;
}

static bool GetMp4Cover(File* file, Vector<uint8_t>& buffer) {
    if (const auto * mp4_file = dynamic_cast<TagLib::MP4::File*>(file)) {
        auto * tag = mp4_file->tag();
        if (!tag) {
            return false;
        }

        if (!tag->itemListMap().contains("covr")) {
            return false;
        }

        auto cover_list = tag->itemListMap()["covr"].toCoverArtList();
        if (cover_list.isEmpty()) {
            return false;
        }
        if (cover_list[0].data().size() > 0) {
            buffer.resize(cover_list[0].data().size());
            MemoryCopy(buffer.data(), cover_list[0].data().data(),
                static_cast<int32_t>(cover_list[0].data().size()));
            return true;
        }
    }
    return false;
}

static std::optional<ReplayGain> GetFlacReplayGain(File* file) {
    ReplayGain replay_gain;
    bool found = false;
    if (auto* const flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
        if (auto* xiph_comment = flac_file->xiphComment()) {
            auto field_map = xiph_comment->fieldListMap();
            for (auto& field : field_map) {
                if (field.first == String::AsStdString(kReplaygainAlbumGain)) {
                    replay_gain.album_gain = std::stod(field.second[0].to8Bit());
                    found = true;
                }
                else if (field.first == String::AsStdString(kReplaygainTrackPeak)) {
                    replay_gain.track_peak = std::stod(field.second[0].to8Bit());
                    found = true;
                }
                else if (field.first == String::AsStdString(kReplaygainAlbumPeak)) {
                    replay_gain.album_peak = std::stod(field.second[0].to8Bit());
                    found = true;
                }
                else if (field.first == String::AsStdString(kReplaygainTrackGain)) {
                    replay_gain.track_gain = std::stod(field.second[0].to8Bit());
                    found = true;
                }
                else if (field.first == String::AsStdString(kReplaygainReferenceLoudness)) {
                    replay_gain.ref_loudness = std::stod(field.second[0].to8Bit());
                    found = true;
                }
            }
        }
    }
    if (!found) {
        return std::nullopt;
    }
    return replay_gain;
}

static bool GetFlacCover(File* file, Vector<uint8_t>& buffer) {
    if (auto* flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
        const auto picture_list = flac_file->pictureList();
        if (picture_list.isEmpty()) {
            return false;
        }

        for (const auto &picture : picture_list) {
            if (picture->type() == TagLib::FLAC::Picture::FrontCover) {                
                buffer.resize(picture->data().size());
                MemoryCopy(buffer.data(), picture->data().data(), picture->data().size());
                return true;
            }
        }
    }
    return false;
}

static void SetFileInfo(Path const & path, TrackInfo& metadata) {
    metadata.file_path = path.wstring();
    metadata.file_name = path.filename().wstring();
    metadata.parent_path = path.parent_path().wstring();
    metadata.file_ext = path.extension().wstring();
    metadata.file_name_no_ext = path.stem().wstring();
    metadata.file_size = Fs::file_size(path);
}

static void SetAudioProperties(AudioProperties* audio_properties, TrackInfo& metadata) {
    if (audio_properties != nullptr) {
        metadata.duration = audio_properties->lengthInMilliseconds() / 1000.0;
        metadata.bitrate = audio_properties->bitrate();
		metadata.samplerate = audio_properties->sampleRate();
    }
}

static void ExtractTitleFromFileName(TrackInfo &metadata) {
    auto start_pos = metadata.file_name_no_ext.find(L'.');
    if (start_pos != std::wstring::npos) {
        metadata.title = metadata.file_name_no_ext.substr(start_pos + 1);
        std::wistringstream istr(metadata.file_name_no_ext.substr(0, start_pos));
        istr >> metadata.track >> metadata.title;
    }
    else {
        metadata.title = metadata.file_name_no_ext;
    }
#ifdef XAMP_OS_WIN
    auto track_id = 0;
    const auto res = swscanf_s(metadata.file_name_no_ext.c_str(), L"Track%02d",
        &track_id);
    if (res == 1) {
        metadata.track = track_id;
    }
#endif
}

static void ExtractTag(Path const & path, Tag const * tag, AudioProperties*audio_properties, TrackInfo& metadata) {
    try {
        if (!tag->isEmpty()) {
            metadata.artist = tag->artist().toWString();
            metadata.title = tag->title().toWString();
            metadata.album = tag->album().toWString();
            metadata.track = tag->track();
            metadata.year = tag->year();
            metadata.genre = tag->genre().toWString();
            metadata.comment = tag->comment().toWString();
        }
    }
    catch (const std::exception& e) {
        XAMP_LOG_DEBUG("ExtractTag path: {}", e.what());
    }    

    SetFileInfo(path, metadata);
    SetAudioProperties(audio_properties, metadata);
}

class TaglibHelper {
public:
    friend class Singleton<TaglibHelper>;

	[[nodiscard]] HashSet<std::string> const & GetSupportFileExtensions() const noexcept {
		return support_file_extensions_;
	}

	[[nodiscard]] bool IsSupported(Path const & path) const noexcept {
		const auto file_ext = String::ToLower(path.extension().string());
		return support_file_extensions_.find(file_ext) != support_file_extensions_.end();
	}

protected:
	TaglibHelper() {
		for (const auto& file_exts : FileRef::defaultFileExtensions()) {
			support_file_extensions_.insert(std::string(".") + file_exts.toCString());
		}
	}
private:
    HashSet<std::string> support_file_extensions_;
};

class TaglibMetadataReader::TaglibMetadataReaderImpl {
public:
    TaglibMetadataReaderImpl() = default;

    static FileRef GetFileRef(const Path& path) {
#ifdef XAMP_OS_WIN
        return FileRef(path.wstring().c_str(), true, TagLib::AudioProperties::Fast);
#else
        return FileRef(path.string().c_str(), true, TagLib::AudioProperties::Fast);
#endif
    }

    [[nodiscard]] TrackInfo Extract(const Path& path) const {
        TrackInfo metadata;

        try {
            // note: taglib會獨佔file, 所以要讀取LastWriteTime必須要在之前.
            metadata.last_write_time = GetLastWriteTime(path);
            // todo: MSVC STL bug memory leak!
            // https://developercommunity.visualstudio.com/t/reported-memory-leak-when-converting-file-time-typ/1467739
            //metadata.last_write_time = ToTime_t(Fs::last_write_time(path));
        }
        catch (Exception const &e) {
            XAMP_LOG_ERROR("Failed to get file: {} last write time error: {}", path.string(), e.GetErrorMessage());
        }

	    const auto fileref = GetFileRef(path);
        const auto* tag = fileref.tag();

        if (tag != nullptr) {
            ExtractTag(path, tag, fileref.audioProperties(), metadata);
        } else {            
            SetFileInfo(path, metadata);
            SetAudioProperties(fileref.audioProperties(), metadata);
        }

        // Tag not empty but title maybe empty!
        if (metadata.title.empty()) {
            ExtractTitleFromFileName(metadata);
        }

        const auto ext = String::ToLower(path.extension().string());
        metadata.replay_gain = GetReplayGain(ext, fileref.file());
        return metadata;
    }

    Vector<uint8_t> const & ExtractCover(Path const & path) {
        cover_.clear();

		if (!IsSupported(path)) {
			cover_.clear();
			return cover_;
		}

        auto fileref = GetFileRef(path);
        const auto* tag = fileref.tag();
        if (!tag) {
            cover_.clear();
            return cover_;
        }
        const auto ext = String::ToLower(path.extension().string());
        GetCover(ext, fileref.file(), cover_);
        return cover_;
    }

    HashSet<std::string> const & GetSupportFileExtensions() const {
        return Singleton<TaglibHelper>::GetInstance().GetSupportFileExtensions();
    }

    bool IsSupported(Path const & path) const noexcept {
		return Singleton<TaglibHelper>::GetInstance().IsSupported(path);
    }

    std::optional<ReplayGain> GetReplayGain(const Path& path) const {
        auto fileref = GetFileRef(path);
        const auto ext = String::ToLower(path.extension().string());
        return GetReplayGain(ext, fileref.file());
    }

private:
    static std::optional<ReplayGain> GetReplayGain(std::string const& ext, File* file) {
        static const HashMap<std::string_view, std::function<std::optional<ReplayGain>(File*)>>
            parse_replay_gain_table{
            { ".flac", GetFlacReplayGain },
            { ".mp3", GetMp3ReplayGain },
            { ".m4a", GetMp4ReplayGain },
        };
        auto itr = parse_replay_gain_table.find(ext);
        if (itr != parse_replay_gain_table.end()) {
            return (*itr).second(file);
        }
        return std::nullopt;
    }

    static void GetCover(std::string const & ext, File*file, Vector<uint8_t>& cover) {
        static const HashMap<std::string_view, std::function<bool(File *, Vector<uint8_t> &)>>
            parse_cover_table{
            { ".flac", GetFlacCover },
            { ".mp3",  GetMp3Cover },
            { ".m4a",  GetMp4Cover },
		    { ".dff", GetDSDIFFCover },
            { ".dsf", GetDSFCover }
        };
        auto itr = parse_cover_table.find(ext);
        if (itr != parse_cover_table.end()) {
            (*itr).second(file, cover);
        }
    }

    Vector<uint8_t> cover_;
};

XAMP_PIMPL_IMPL(TaglibMetadataReader)

TaglibMetadataReader::TaglibMetadataReader()
    : reader_(MakePimpl<TaglibMetadataReaderImpl>()) {
}

TrackInfo TaglibMetadataReader::Extract(const Path& path) {
    return reader_->Extract(path);
}

std::optional<ReplayGain> TaglibMetadataReader::GetReplayGain(const Path& path) {
    return reader_->GetReplayGain(path);
}

const Vector<uint8_t>& TaglibMetadataReader::GetEmbeddedCover(Path const & path) {
    return reader_->ExtractCover(path);
}

const HashSet<std::string>& TaglibMetadataReader::GetSupportFileExtensions() const {
    return reader_->GetSupportFileExtensions();
}

bool TaglibMetadataReader::IsSupported(Path const & path) const {
    return reader_->IsSupported(path);
}

}
