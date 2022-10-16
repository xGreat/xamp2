#include <base/singleton.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>
#include <stream/basslib.h>

namespace xamp::stream {

template <typename T>
constexpr uint8_t HiByte(T val) noexcept {
    return static_cast<uint8_t>(val >> 8);
}

template <typename T>
constexpr uint8_t LowByte(T val) noexcept {
    return static_cast<uint8_t>(val);
}

template <typename T>
constexpr uint16_t HiWord(T val) noexcept {
    return static_cast<uint16_t>((static_cast<uint32_t>(val) >> 16) & 0xFFFF);
}

template <typename T>
constexpr uint16_t LoWord(T val) noexcept {
    return static_cast<uint16_t>(static_cast<uint32_t>(val) & 0xFFFF);
}

std::string GetBassVersion(uint32_t version) {
    const uint32_t major_version(HiByte(HiWord(version)));
    const uint32_t minor_version(LowByte(HiWord(version)));
    const uint32_t micro_version(HiByte(LoWord(version)));
    const uint32_t build_version(LowByte(LoWord(version)));

    std::ostringstream ostr;
    ostr << major_version << "."
        << minor_version << "."
        << micro_version << "."
        << build_version;

    return ostr.str();
}

BassDSDLib::BassDSDLib() try
#ifdef XAMP_OS_WIN
    : module_(LoadModule("bassdsd.dll"))
#else
    : module_(LoadModule("libbassdsd.dylib"))
#endif
    , XAMP_LOAD_DLL_API(BASS_DSD_StreamCreateFile) {
    PrefetchModule(module_);
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

BassMixLib::BassMixLib() try
#ifdef XAMP_OS_WIN
    : module_(LoadModule("bassmix.dll"))
#else
    : module_(LoadModule("libbassmix.dylib"))
#endif
    , XAMP_LOAD_DLL_API(BASS_Mixer_StreamCreate)
    , XAMP_LOAD_DLL_API(BASS_Mixer_StreamAddChannel)
    , XAMP_LOAD_DLL_API(BASS_Mixer_GetVersion) {
    PrefetchModule(module_);
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

std::string BassMixLib::GetName() const {
#ifdef XAMP_OS_WIN
    return "bassmix.dll";
#else
    return "libbassmix.dylib";
#endif
}

BassFxLib::BassFxLib() try
#ifdef XAMP_OS_WIN
    : module_(LoadModule("bass_fx.dll"))
#else
    : module_(LoadModule("libbass_fx.dylib"))
#endif
    , XAMP_LOAD_DLL_API(BASS_FX_TempoGetSource)
    , XAMP_LOAD_DLL_API(BASS_FX_TempoCreate)
    , XAMP_LOAD_DLL_API(BASS_FX_GetVersion) {
    PrefetchModule(module_);
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

std::string BassFxLib::GetName() const {
#ifdef XAMP_OS_WIN
    return "bass_fx.dll";
#else
    return "libbass_fx.dylib";
#endif
}

#ifdef XAMP_OS_WIN
BassCDLib::BassCDLib() try
#ifdef XAMP_OS_WIN
    : module_(LoadModule("basscd.dll"))
#else
    : module_(LoadModule("libbasscd.dylib"))
#endif
    , XAMP_LOAD_DLL_API(BASS_CD_GetInfo)
    , XAMP_LOAD_DLL_API(BASS_CD_GetSpeed)
    , XAMP_LOAD_DLL_API(BASS_CD_Door)
    , XAMP_LOAD_DLL_API(BASS_CD_DoorIsLocked)
    , XAMP_LOAD_DLL_API(BASS_CD_DoorIsOpen)
    , XAMP_LOAD_DLL_API(BASS_CD_SetInterface)
    , XAMP_LOAD_DLL_API(BASS_CD_SetOffset)
    , XAMP_LOAD_DLL_API(BASS_CD_SetSpeed)
    , XAMP_LOAD_DLL_API(BASS_CD_Release)
    , XAMP_LOAD_DLL_API(BASS_CD_IsReady)
    , XAMP_LOAD_DLL_API(BASS_CD_GetID)
    , XAMP_LOAD_DLL_API(BASS_CD_GetTracks)
    , XAMP_LOAD_DLL_API(BASS_CD_GetTrackLength) {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}
#endif

#ifdef XAMP_OS_WIN
std::string BassCDLib::GetName() const {
#ifdef XAMP_OS_WIN
    return "basscd.dll";
#else
    return "libbasscd.dylib";
#endif
}
#endif

BassEncLib::BassEncLib()  try
#ifdef XAMP_OS_WIN
    : module_(LoadModule("bassenc.dll"))
#else
    : module_(LoadModule("libbassenc.dylib"))
#endif
    , XAMP_LOAD_DLL_API(BASS_Encode_StartACMFile)
	, XAMP_LOAD_DLL_API(BASS_Encode_GetVersion)
	, XAMP_LOAD_DLL_API(BASS_Encode_GetACMFormat) {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

std::string BassEncLib::GetName() const {
#ifdef XAMP_OS_WIN
    return "bassenc.dll";
#else
    return "libbassenc.dylib";
#endif
}

#ifdef XAMP_OS_WIN
BassAACEncLib::BassAACEncLib() try
    : module_(LoadModule("bassenc_aac.dll"))
    , XAMP_LOAD_DLL_API(BASS_Encode_AAC_StartFile)
    , XAMP_LOAD_DLL_API(BASS_Encode_AAC_GetVersion) {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

std::string BassAACEncLib::GetName() const {
    return "bass_aac.dll";
}
#else
BassCAEncLib::BassCAEncLib() try
    : module_(LoadModule("libbassenc.dylib"))
    , XAMP_LOAD_DLL_API(BASS_Encode_StartCAFile) {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

std::string BassCAEncLib::GetName() const {
    return "bass_aac.dll";
}
#endif

BassFLACEncLib::BassFLACEncLib() try
#ifdef XAMP_OS_WIN
    : module_(LoadModule("bassenc_flac.dll"))
#else
    : module_(LoadModule("libbassenc_flac.dylib"))
#endif
    , XAMP_LOAD_DLL_API(BASS_Encode_FLAC_StartFile)
	, XAMP_LOAD_DLL_API(BASS_Encode_FLAC_GetVersion) {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

std::string BassFLACEncLib::GetName() const {
#ifdef XAMP_OS_WIN
    return "bassenc_flac.dll";
#else
    return "libbassenc_flac.dylib";
#endif
}

BassLib::BassLib() try
#ifdef XAMP_OS_WIN
    : module_(LoadModule("bass.dll"))
#else
    : module_(LoadModule("libbass.dylib"))
#endif
    , XAMP_LOAD_DLL_API(BASS_Init)
    , XAMP_LOAD_DLL_API(BASS_GetVersion)
    , XAMP_LOAD_DLL_API(BASS_SetConfig)
    , XAMP_LOAD_DLL_API(BASS_SetConfigPtr)
    , XAMP_LOAD_DLL_API(BASS_PluginLoad)
    , XAMP_LOAD_DLL_API(BASS_PluginGetInfo)
    , XAMP_LOAD_DLL_API(BASS_Free)
    , XAMP_LOAD_DLL_API(BASS_StreamCreateFile)
    , XAMP_LOAD_DLL_API(BASS_ChannelGetInfo)
    , XAMP_LOAD_DLL_API(BASS_StreamFree)
    , XAMP_LOAD_DLL_API(BASS_PluginFree)
    , XAMP_LOAD_DLL_API(BASS_ChannelGetData)
    , XAMP_LOAD_DLL_API(BASS_ChannelGetLength)
    , XAMP_LOAD_DLL_API(BASS_ChannelBytes2Seconds)
    , XAMP_LOAD_DLL_API(BASS_ChannelSeconds2Bytes)
    , XAMP_LOAD_DLL_API(BASS_ChannelSetPosition)
    , XAMP_LOAD_DLL_API(BASS_ChannelGetPosition)
    , XAMP_LOAD_DLL_API(BASS_ErrorGetCode)
    , XAMP_LOAD_DLL_API(BASS_ChannelGetAttribute)
    , XAMP_LOAD_DLL_API(BASS_ChannelSetAttribute)
    , XAMP_LOAD_DLL_API(BASS_StreamCreate)
    , XAMP_LOAD_DLL_API(BASS_StreamPutData)
    , XAMP_LOAD_DLL_API(BASS_ChannelSetFX)
    , XAMP_LOAD_DLL_API(BASS_ChannelRemoveFX)
    , XAMP_LOAD_DLL_API(BASS_FXSetParameters)
    , XAMP_LOAD_DLL_API(BASS_FXGetParameters)
    , XAMP_LOAD_DLL_API(BASS_StreamCreateURL)
    , XAMP_LOAD_DLL_API(BASS_StreamGetFilePosition)
    , XAMP_LOAD_DLL_API(BASS_ChannelIsActive) {
    PrefetchModule(module_);
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

BassLib::~BassLib() {
	if (!module_.is_valid()) {
        return;
	}
    Free();
}

std::string BassLib::GetName() const {
#ifdef XAMP_OS_WIN
    return "bass.dll";
#else
    return "libbass.dylib";
#endif
}

HPLUGIN BassPluginLoadDeleter::invalid() noexcept {
    return 0;
}

 void BassPluginLoadDeleter::close(HPLUGIN value) {
     BASS.BASS_PluginFree(value);
}

HSTREAM BassStreamDeleter::invalid() noexcept {
    return 0;
}

void BassStreamDeleter::close(HSTREAM value) {
    BASS.BASS_StreamFree(value);
}

void BassLib::Load() {
    if (IsLoaded()) {
        return;
    }

#ifdef XAMP_OS_WIN
    // Disable 1ms timer resolution
#define BASS_CONFIG_NOTIMERES 29
    BASS.BASS_SetConfig(BASS_CONFIG_NOTIMERES, 1);
#endif

    BASS.BASS_Init(0, 44100, 0, nullptr, nullptr);
    XAMP_LOG_DEBUG("Load BASS {} successfully.", GetBassVersion(BASS.BASS_GetVersion()));
#ifdef XAMP_OS_WIN
    // Disable Media Foundation
    BASS.BASS_SetConfig(BASS_CONFIG_MF_DISABLE, true);
    BASS.BASS_SetConfig(BASS_CONFIG_MF_VIDEO, false);
    LoadPlugin("bass_aac.dll");
    LoadPlugin("bassflac.dll");
    LoadPlugin("bass_ape.dll");
    LoadPlugin("basscd.dll");
    // For GetSupportFileExtensions need!
    LoadPlugin("bassdsd.dll");
#else
    LoadPlugin("libbassflac.dylib");
    LoadPlugin("libbassdsd.dylib");
#endif

    BASS.BASS_SetConfig(BASS_CONFIG_DSD_FREQ, 88200);
    BASS.BASS_SetConfig(BASS_CONFIG_FLOATDSP, true);
    BASS.BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 15 * 1000);
    BASS.BASS_SetConfig(BASS_CONFIG_NET_BUFFER, 50000);
    BASS.BASS_SetConfig(BASS_CONFIG_NET_PREBUF, 50);
    BASS.BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, false);
    BASS.BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);
    BASS.BASS_SetConfigPtr(BASS_CONFIG_NET_AGENT, L"xamp2");
}

void BassLib::Free() {
    XAMP_LOG_INFO("Release BassLib dll.");
    plugins_.clear();
    if (module_.is_valid()) {
        try {
            BASS.BASS_Free();
        }
        catch (const Exception& e) {
            XAMP_LOG_INFO("{}", e.what());
        }
    }
}

void BassLib::LoadPlugin(std::string const & file_name) {
    BassPluginHandle plugin(BASS.BASS_PluginLoad(file_name.c_str(), 0));
    if (!plugin) {
        XAMP_LOG_DEBUG("Load {} failure. error:{}",
            file_name,
            BASS.BASS_ErrorGetCode());
        return;
    }

    const auto* info = BASS.BASS_PluginGetInfo(plugin.get());
    XAMP_LOG_DEBUG("Load {} {} successfully.", file_name, GetBassVersion(info->version));

    plugins_[file_name] = std::move(plugin);
}

std::map<std::string, std::string> BassLib::GetPluginVersion() const {
    std::map<std::string, std::string> vers;

    for (const auto& [key, value] : plugins_) {
        const auto* info = BASS.BASS_PluginGetInfo(value.get());
        vers[key] = GetBassVersion(info->version);
    }
    return vers;
}

void BassLib::LoadVersionInfo() {
    dll_versions_ = GetPluginVersion();
    dll_versions_[BASS.GetName()] = GetBassVersion(BASS.BASS_GetVersion());
    dll_versions_[BASS.MixLib->GetName()] = GetBassVersion(BASS.MixLib->BASS_Mixer_GetVersion());
    dll_versions_[BASS.FxLib->GetName()] = GetBassVersion(BASS.FxLib->BASS_FX_GetVersion());
    if (BASS.EncLib != nullptr) {
        dll_versions_[BASS.EncLib->GetName()] = GetBassVersion(BASS.EncLib->BASS_Encode_GetVersion());
    }
    dll_versions_[BASS.FLACEncLib->GetName()] = GetBassVersion(BASS.FLACEncLib->BASS_Encode_FLAC_GetVersion());
#ifdef XAMP_OS_WIN
    dll_versions_[BASS.AACEncLib->GetName()] = GetBassVersion(BASS.AACEncLib->BASS_Encode_AAC_GetVersion());
#endif
}

std::map<std::string, std::string> BassLib::GetVersions() const {
    return dll_versions_;
}

HashSet<std::string> BassLib::GetSupportFileExtensions() const {
    HashSet<std::string> result;
	
	for (const auto& [key, value] : plugins_) {
        const auto* info = BASS.BASS_PluginGetInfo(value.get());
		
        for (DWORD i = 0; i < info->formatc; ++i) {
            XAMP_LOG_TRACE("Load BASS {} {}", info->formats[i].name, info->formats[i].exts);
        	for (auto file_ext : String::Split(info->formats[i].exts, ";")) {
                std::string ext(file_ext);
                auto pos = ext.find('*');
        		if (pos != std::string::npos) {
                    ext.erase(pos, 1);
        		}                
                result.insert(ext);
        	}           
        }
	}

	// Workaround!
    result.insert(".wav");
    result.insert(".mp3");

    #ifdef XAMP_OS_MAC
    result.insert(".m4a");
    result.insert(".aac");
    #endif

    return result;
} 

}
