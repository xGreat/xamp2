#include <stream/avlib.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>

namespace xamp::stream {

AvException::AvException(int32_t error)
	: Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR) {
	char buf[256]{};
	LIBAV_LIB.UtilLib->av_strerror(error, buf, sizeof(buf) - 1);
	message_.assign(buf);
}

AvException::~AvException() = default;

XAMP_DECLARE_LOG_NAME(LibAv);

AvFormatLib::AvFormatLib() try
    : module_(OpenSharedLibrary("avformat-58"))
	, XAMP_LOAD_DLL_API(avformat_open_input)
	, XAMP_LOAD_DLL_API(avformat_close_input)
	, XAMP_LOAD_DLL_API(avformat_find_stream_info)
	, XAMP_LOAD_DLL_API(av_seek_frame)
	, XAMP_LOAD_DLL_API(av_read_frame)
	, XAMP_LOAD_DLL_API(avformat_network_init)
	, XAMP_LOAD_DLL_API(avformat_network_deinit)
	, XAMP_LOAD_DLL_API(avformat_alloc_context)
	, XAMP_LOAD_DLL_API(avformat_new_stream)
	, XAMP_LOAD_DLL_API(avformat_query_codec)
	, XAMP_LOAD_DLL_API(av_oformat_next) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

AvCodecLib::AvCodecLib() try
    : module_(OpenSharedLibrary("avcodec-58"))
	, XAMP_LOAD_DLL_API(avcodec_close)
	, XAMP_LOAD_DLL_API(avcodec_open2)
	, XAMP_LOAD_DLL_API(avcodec_alloc_context3)
	, XAMP_LOAD_DLL_API(avcodec_find_decoder)
	, XAMP_LOAD_DLL_API(av_packet_alloc)
	, XAMP_LOAD_DLL_API(av_init_packet)
	, XAMP_LOAD_DLL_API(av_packet_unref)
	, XAMP_LOAD_DLL_API(avcodec_send_packet)
	, XAMP_LOAD_DLL_API(avcodec_receive_frame)
	, XAMP_LOAD_DLL_API(avcodec_flush_buffers)
	, XAMP_LOAD_DLL_API(av_get_bits_per_sample)
	, XAMP_LOAD_DLL_API(avcodec_find_decoder_by_name)
	, XAMP_LOAD_DLL_API(avcodec_find_encoder)
	, XAMP_LOAD_DLL_API(avcodec_configuration)
	, XAMP_LOAD_DLL_API(avcodec_parameters_from_context)
	, XAMP_LOAD_DLL_API(av_codec_next) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

AvUtilLib::AvUtilLib() try
    : module_(OpenSharedLibrary("avutil-56"))
	, XAMP_LOAD_DLL_API(av_free)
	, XAMP_LOAD_DLL_API(av_frame_unref)
	, XAMP_LOAD_DLL_API(av_frame_get_buffer)
	, XAMP_LOAD_DLL_API(av_get_bytes_per_sample)
	, XAMP_LOAD_DLL_API(av_strerror)
	, XAMP_LOAD_DLL_API(av_frame_alloc)
	, XAMP_LOAD_DLL_API(av_malloc)
	, XAMP_LOAD_DLL_API(av_samples_get_buffer_size)
	, XAMP_LOAD_DLL_API(av_log_set_callback)
	, XAMP_LOAD_DLL_API(av_log_format_line)
	, XAMP_LOAD_DLL_API(av_log_set_level)
	, XAMP_LOAD_DLL_API(av_dict_set)
	, XAMP_LOAD_DLL_API(av_get_channel_layout_nb_channels)
	, XAMP_LOAD_DLL_API(av_audio_fifo_alloc) 
	, XAMP_LOAD_DLL_API(av_audio_fifo_realloc)
	, XAMP_LOAD_DLL_API(av_audio_fifo_write) 
	, XAMP_LOAD_DLL_API(av_audio_fifo_free) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

AvSwLib::AvSwLib() try
    : module_(OpenSharedLibrary("swresample-3"))
	, XAMP_LOAD_DLL_API(swr_free)
	, XAMP_LOAD_DLL_API(swr_alloc_set_opts)
	, XAMP_LOAD_DLL_API(swr_convert)
	, XAMP_LOAD_DLL_API(swr_init)
	, XAMP_LOAD_DLL_API(swr_close) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

static void LogPrintf(void* ptr, int level, const char* fmt, va_list vl) {
	va_list valist;
	char message[1024]{};

	va_copy(valist, vl);
	int print_prefix = 1;
	LIBAV_LIB.UtilLib->av_log_format_line(ptr, level, fmt, valist, message, sizeof(message), &print_prefix);
	va_end(valist);

	const auto message_length = strlen(message) - 1;
	if (message[message_length] == '\n') {
		message[message_length] = '\0';
	}

	auto log_level = LogLevel::LOG_LEVEL_DEBUG;

	switch (level) {
	case AV_LOG_PANIC:
	case AV_LOG_FATAL:
		log_level = LogLevel::LOG_LEVEL_ERROR;
		break;
	case AV_LOG_ERROR:
	case AV_LOG_WARNING:
		log_level = LogLevel::LOG_LEVEL_WARN;
		break;
	case AV_LOG_INFO:
	case AV_LOG_VERBOSE:
		log_level = LogLevel::LOG_LEVEL_INFO;
		break;
	case AV_LOG_DEBUG:
		log_level = LogLevel::LOG_LEVEL_DEBUG;
		break;
	default:
		return;
	}

	XAMP_LOG_LEVEL(LIBAV_LIB.logger, log_level, "{}", message);
}

AvLib::~AvLib() {
	FormatLib->avformat_network_deinit();
	XAMP_LOG_D(logger, "Network deinit.");
}

AvLib::AvLib() {
	logger = LoggerManager::GetInstance().GetLogger(kLibAvLoggerName);
	XAMP_LOG_D(logger, "Load {} success.", LIBAVCODEC_IDENT);

	FormatLib = MakeAlign<AvFormatLib>();
	CodecLib = MakeAlign<AvCodecLib>();
	SwrLib = MakeAlign<AvSwLib>();
	UtilLib = MakeAlign<AvUtilLib>();

	UtilLib->av_log_set_callback(LogPrintf);
	UtilLib->av_log_set_level(AV_LOG_FATAL);

	FormatLib->avformat_network_init();

	const auto level = logger->GetLevel();
	logger->SetLevel(LOG_LEVEL_DEBUG);
	XAMP_LOG_D(logger, CodecLib->avcodec_configuration());
	logger->SetLevel(level);

	XAMP_LOG_D(logger, "Network init.");
}

HashSet<std::string> AvLib::GetSupportFileExtensions() const {
	HashSet<std::string> result;
	HashSet<const AVCodec*> audio_codecs;

	const auto level = logger->GetLevel();
	logger->SetLevel(LOG_LEVEL_DEBUG);

	const auto* codec = CodecLib->av_codec_next(nullptr);
	while (codec != nullptr) {
		if (!codec->decode) {
			if (codec->type == AVMEDIA_TYPE_AUDIO) {
				audio_codecs.insert(codec);
			}
		}
		codec = codec->next;
	}

	auto output_format = FormatLib->av_oformat_next(nullptr);
	while (output_format != nullptr) {
		for (auto* audio_codec : audio_codecs) {
			if (FormatLib->avformat_query_codec(output_format, audio_codec->id, FF_COMPLIANCE_STRICT) == 1) {
				if (!output_format->extensions)
					continue;
				for (const auto& extension : String::Split(output_format->extensions, ",")) {
					const auto file_extensions = String::Format(".{}", extension);
					if (!result.contains(file_extensions)) {
						XAMP_LOG_D(logger, "Load Libav name:{} extensions: {}", output_format->name, file_extensions);
						result.insert(file_extensions);
					}
				}
			}
		}
		output_format = output_format->next;
	}

	// Workaround!
	result.insert(".wav");
	result.insert(".mp3");
    result.insert(".dsf");
    result.insert(".dff");

	logger->SetLevel(level);
	return result;
}

}
