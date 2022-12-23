#include <widget/read_utiltis.h>
#include <widget/widget_shared.h>

#include <base/align_ptr.h>
#include <base/buffer.h>
#include <base/memory.h>
#include <base/logger_impl.h>
#include <base/exception.h>

#include <stream/idsdstream.h>
#include <stream/api.h>
#include <stream/filestream.h>
#include <stream/ifileencoder.h>

#include <metadata/api.h>
#include <metadata/imetadatawriter.h>

#include <player/ebur128reader.h>

#include <functional>
#include <optional>
#include <utility>
#include <tuple>

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::player;
using namespace xamp::metadata;

namespace read_utiltis {

inline constexpr uint32_t kReadSampleSize = 8192 * 4;

class ExceptedFile final {
public:
    explicit ExceptedFile(std::filesystem::path const& dest_file_path) {
        dest_file_path_ = dest_file_path;
        temp_file_path_ = Fs::temp_directory_path()
                          / Fs::path(MakeTempFileName() + ".tmp");
    }

    template <typename Func>
    bool Try(Func&& func) {
        try {
            func(temp_file_path_);
            Fs::rename(temp_file_path_, dest_file_path_);
			return true;
        }
        catch (Exception const &e) {
            XAMP_LOG_DEBUG("{}", e.GetErrorMessage());
        }
        catch (...) {
        }
        std::filesystem::remove(temp_file_path_);
		return false;
    }

private:
    Fs::path dest_file_path_;
    Fs::path temp_file_path_;
};

double readAll(Path const& file_path,
	std::function<bool(uint32_t)> const& progress,
	std::function<void(AudioFormat const&)> const& prepare,
	std::function<void(float const*, uint32_t)> const& dsp_process,
    uint64_t max_duration) {
	const auto is_dsd_file = TestDsdFileFormatStd(file_path.wstring());
    const auto file_stream = StreamFactory::MakeFileStream(is_dsd_file
                                                               ? DsdModes::DSD_MODE_NATIVE : DsdModes::DSD_MODE_PCM);

	if (auto* stream = AsDsdStream(file_stream)) {
		if (is_dsd_file) {
			stream->SetDSDMode(DsdModes::DSD_MODE_DSD2PCM);
		}
	}

	file_stream->OpenFile(file_path.wstring());

	const auto source_format = file_stream->GetFormat();
	const AudioFormat input_format = AudioFormat::ToFloatFormat(source_format);

	const auto buffer_size = GetPageAlignSize(1024 + kReadSampleSize * input_format.GetChannels());
	auto buffer = MakeBuffer<float>(buffer_size);
	uint32_t num_samples = 0;

	prepare(input_format);

	if (max_duration == (std::numeric_limits<uint64_t>::max)()) {
		max_duration = static_cast<uint64_t>(file_stream->GetDuration());
	}

    while (num_samples / input_format.GetSampleRate() < max_duration && file_stream->IsActive()) {
		const auto read_size = file_stream->GetSamples(buffer.get(),
			kReadSampleSize) / input_format.GetChannels();

		num_samples += read_size;
		if (progress != nullptr) {
			const auto percent = static_cast<uint32_t>((num_samples / input_format.GetSampleRate() * 100) / max_duration);
			if (!progress(percent)) {
				break;
			}
		}

		dsp_process(buffer.get(), read_size * input_format.GetChannels());
	}

	return file_stream->GetDuration();
}

std::tuple<double, double> readFileLUFS(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    uint64_t max_duration) {
	Ebur128Reader scanner;

    readAll(file_path, progress,
		[&scanner](AudioFormat const& input_format)
		{
			scanner.SetSampleRate(input_format.GetSampleRate());
		}, [&scanner](auto const* samples, auto sample_size)
		{
			scanner.Process(samples, sample_size);
        }, max_duration);

    return std::make_tuple(scanner.GetLoudness(),
                           scanner.GetTruePeek());
}

void encodeFile(AnyMap const& config,
	AlignPtr<IFileEncoder>& encoder,
	std::function<bool(uint32_t)> const& progress,
	TrackInfo const& track_info) {
	const auto file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
	const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
	const auto command = config.AsPath(FileEncoderConfig::kCommand);
	ExceptedFile excepted(output_file_path);
	if (excepted.Try([&](auto const& dest_file_path) {
		encoder->Start(config);
		encoder->Encode(progress);
		encoder.reset();
		})) {
		const auto writer = MakeMetadataWriter();
		writer->Write(output_file_path, track_info);
	}
}

void encodeFile(Path const& file_path,
	Path const& output_file_path,
	AlignPtr<IFileEncoder>& encoder,
    std::wstring const& command,
    std::function<bool(uint32_t)> const& progress,
    TrackInfo const& track_info) {
    ExceptedFile excepted(output_file_path);
	AnyMap config;
	config.AddOrReplace(FileEncoderConfig::kInputFilePath, file_path);
	config.AddOrReplace(FileEncoderConfig::kOutputFilePath, output_file_path);
	config.AddOrReplace(FileEncoderConfig::kCommand, command);
	encodeFile(config, encoder, progress, track_info);
}

}
