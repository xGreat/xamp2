#include <fstream>
#include <base/memory_mapped_file.h>
#include <base/str_utilts.h>
#include <base/logger.h>

#include <stream/basslib.h>
#include <stream/idsdstream.h>
#include <stream/bassfilestream.h>
#include <stream/ifileencoder.h>
#include <stream/basswavfileencoder.h>
#include <stream/bassflacfileencoder.h>
#include <stream/bassaacfileencoder.h>
#include <stream/bassfader.h>
#include <stream/basscddevice.h>
#include <stream/basscompressor.h>
#include <stream/bassequalizer.h>
#include <stream/bassvolume.h>
#include <stream/dspmanager.h>
#include <stream/fftwlib.h>
#include <stream/r8brainlib.h>
#include <stream/soxrlib.h>
#include <base/logger_impl.h>
#include <stream/api.h>

namespace xamp::stream {

using namespace xamp::base;

static bool TestDsdFileFormat(std::string_view const & file_chunks) noexcept {
    static constexpr std::array<std::string_view, 2> knows_chunks{
        "DSD ", // .dsd file
        "FRM8"  // .dsdiff file
    };	
    return std::find(knows_chunks.begin(), knows_chunks.end(), file_chunks)
        != knows_chunks.end();
}

bool TestDsdFileFormatStd(std::wstring const& file_path) {
#ifdef XAMP_OS_WIN
    std::ifstream file(file_path, std::ios_base::binary);
#else
    std::ifstream file(String::ToString(file_path), std::ios_base::binary);
#endif
    if (!file.is_open()) {
        return false;
    }
    std::array<char, 4> buffer{ 0 };
    file.read(buffer.data(), buffer.size());
    if (file.gcount() < 4) {
        return false;
    }
    const std::string_view file_chunks{ buffer.data(), 4 };
    return TestDsdFileFormat(file_chunks);
}

AlignPtr<IAudioStream> DspComponentFactory::MakeAudioStream() {
    return MakeAlign<IAudioStream, BassFileStream>();
}

AlignPtr<IFileEncoder> DspComponentFactory::MakeFlacEncoder() {
    return MakeAlign<IFileEncoder, BassFlacFileEncoder>();
}

AlignPtr<IFileEncoder> DspComponentFactory::MakeAACEncoder() {
    return MakeAlign<IFileEncoder, BassAACFileEncoder>();
}

AlignPtr<IFileEncoder> DspComponentFactory::MakeWaveEncoder() {
    return MakeAlign<IFileEncoder, BassWavFileEncoder>();
}

AlignPtr<IAudioProcessor> DspComponentFactory::MakeEqualizer() {
    return MakeAlign<IAudioProcessor, BassEqualizer>();
}

AlignPtr<IAudioProcessor> DspComponentFactory::MakeCompressor() {
    return MakeAlign<IAudioProcessor, BassCompressor>();
}

AlignPtr<IAudioProcessor> DspComponentFactory::MakeFader() {
    return MakeAlign<IAudioProcessor, BassFader>();
}

AlignPtr<IAudioProcessor> DspComponentFactory::MakeVolume() {
    return MakeAlign<IAudioProcessor, BassVolume>();
}

AlignPtr<IDSPManager> DspComponentFactory::MakeDSPManager() {
    return MakeAlign<IDSPManager, DSPManager>();
}

#ifdef XAMP_OS_WIN
AlignPtr<ICDDevice> DspComponentFactory::MakeCDDevice(int32_t driver_letter) {
    return MakeAlign<ICDDevice, BassCDDevice>(static_cast<char>(driver_letter));
}
#endif

IDsdStream* AsDsdStream(AlignPtr<IAudioStream> const& stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream.get());
}

FileStream* AsFileStream(AlignPtr<IAudioStream> const& stream) noexcept {
    return dynamic_cast<FileStream*>(stream.get());
}

IDsdStream* AsDsdStream(IAudioStream* stream) noexcept {
    return dynamic_cast<IDsdStream*>(stream);
}

HashSet<std::string> GetSupportFileExtensions() {
    return BASS.GetSupportFileExtensions();
}

void LoadBassLib() {
    if (!BASS.IsLoaded()) {
        Singleton<BassLib>::GetInstance().Load();
    }
    BASS.MixLib = MakeAlign<BassMixLib>();
    BASS.DSDLib = MakeAlign<BassDSDLib>();
    BASS.FxLib = MakeAlign<BassFxLib>();
#ifdef XAMP_OS_WIN
    BASS.CDLib = MakeAlign<BassCDLib>();
    BASS.AACEncLib = MakeAlign<BassAACEncLib>();
#endif
    try {
        BASS.EncLib = MakeAlign<BassEncLib>();
    }  catch (const Exception &e) {
        XAMP_LOG_DEBUG("Load EncLib error: {}", e.what());
    }
    BASS.FLACEncLib = MakeAlign<BassFLACEncLib>();
    BASS.LoadVersionInfo();
    for (const auto& info : BASS.GetVersions()) {
        XAMP_LOG_DEBUG("DLL {} ver: {}", info.first, info.second);
    }
}

std::map<std::string, std::string> GetBassDLLVersion() {
    return BASS.GetVersions();
}

void FreeBassLib() {
    BASS.Free();
}

void LoadFFTLib() {
    Singleton<FFTWLib>::GetInstance();
    //Singleton<FFTWFLib>::GetInstance();
}

#ifdef XAMP_OS_WIN
void LoadR8brainLib() {
    Singleton<R8brainLib>::GetInstance();
}
#endif

void LoadSoxrLib() {
    Singleton<SoxrLib>::GetInstance();
}

}
