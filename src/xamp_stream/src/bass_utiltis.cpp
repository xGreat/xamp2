#include <stream/bass_utiltis.h>

#include <base/buffer.h>
#include <stream/bassfilestream.h>
#include <stream/basslib.h>

namespace xamp::stream::BassUtiltis {

void Encode(BassFileStream& stream, std::function<bool(uint32_t) > const& progress) {
    constexpr uint32_t kReadSampleSize = 8192 * 4;

    auto buffer = MakeBuffer<float>(kReadSampleSize * AudioFormat::kMaxChannel);

    uint32_t num_samples = 0;
    const auto max_duration = static_cast<uint64_t>(stream.GetDuration());

    while (stream.IsActive()) {
        const auto read_size = stream.GetSamples(buffer.data(), kReadSampleSize)
            / AudioFormat::kMaxChannel;
        if (read_size == kBassError || read_size == 0) {
            break;
        }
        num_samples += read_size;
        const auto percent = static_cast<uint32_t>(num_samples / stream.GetFormat().GetSampleRate() * 100 / max_duration);
        if (!progress(percent)) {
            break;
        }
    }
}

}
