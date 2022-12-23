﻿#include <widget/http.h>

#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QUrl>
#include <QTemporaryFile>
#include <QNetworkProxy>
#include <QMetaEnum>

#include <memory>

#include <zlib.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>
#include <base/dll.h>
#include <base/scopeguard.h>

#include <version.h>
#include <widget/str_utilts.h>
#include <widget/widget_shared.h>


namespace http {

XAMP_DECLARE_LOG_NAME(Http);

static constexpr size_t kHttpBufferSize = 512 * 1024;
static constexpr int32_t kHttpDefaultTimeout = 3000;

class ZLibLib final {
public:
    ZLibLib();

private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(inflateInit2_);
    XAMP_DECLARE_DLL_NAME(inflate);
    XAMP_DECLARE_DLL_NAME(inflateEnd);
};

ZLibLib::ZLibLib()
	: module_(OpenSharedLibrary("zlib1"))
    , XAMP_LOAD_DLL_API(inflateInit2_)
    , XAMP_LOAD_DLL_API(inflate)
    , XAMP_LOAD_DLL_API(inflateEnd) {
}

#define ZLIB_DLL Singleton<ZLibLib>::GetInstance()

static ConstLatin1String networkErrorToString(QNetworkReply::NetworkError code) {
    const auto* mo = &QNetworkReply::staticMetaObject;
    const int index = mo->indexOfEnumerator("NetworkError");
    if (index == -1)
        return qEmptyString;
    const auto qme = mo->enumerator(index);
    return { qme.valueToKey(code) };
}

static bool isLoadZib() {
	try {
        Singleton<ZLibLib>::GetInstance();
        return true;
	} catch (...) {
        return false;
	}
}

static bool isZipEncoding(QNetworkReply const *reply) {
    bool is_gzipped = false;
    Q_FOREACH (const auto &header_pair, reply->rawHeaderPairs()) {
        if ((header_pair.first == "Content-Encoding") && (header_pair.second == "gzip")) {
            is_gzipped = true;
        }
    }
    return is_gzipped;
}

static QByteArray gzipUncompress(const QByteArray& data) {
#define XAMP_inflateInit2(strm, windowBits) \
          ZLIB_DLL.inflateInit2_((strm), (windowBits), ZLIB_VERSION, \
                        (int)sizeof(z_stream))
    QByteArray result;
    z_stream zlib_stream;

    zlib_stream.zalloc = nullptr;
    zlib_stream.zfree = nullptr;
    zlib_stream.opaque = nullptr;
    zlib_stream.avail_in = data.size();
    zlib_stream.next_in = (Bytef*)data.data();

    auto ret = XAMP_inflateInit2(&zlib_stream, 15 + 32); // gzip decoding
    if (ret != Z_OK) {
        return result;
    }

    XAMP_ON_SCOPE_EXIT(
        ZLIB_DLL.inflateEnd(&zlib_stream);
    );

    do {
        char output[kHttpBufferSize]{ };
        zlib_stream.avail_out = kHttpBufferSize;
        zlib_stream.next_out = reinterpret_cast<Bytef*>(output);

        ret = ZLIB_DLL.inflate(&zlib_stream, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            return result;
        }

        result.append(output, kHttpBufferSize - zlib_stream.avail_out);
    } while (zlib_stream.avail_out == 0);

    return result;
}

static void logHttpRequest(const LoggerPtr &logger,
    const ConstLatin1String& verb,
    const QString& url,
    const QNetworkRequest& request,
    const QNetworkReply *reply) {
    auto content_length = 0U;

    QString msg;
    QTextStream stream(&msg);

    if (!reply) {
        stream << "Request: ";
    }
    else {
        stream << "Response: ";
    }

    stream << verb;
    if (reply) {
        stream << " " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }

    stream << " " << url << " Header: { ";

    QString content_type;
    if (!reply) {
        const auto header_list = request.rawHeaderList();
        Q_FOREACH(const auto& head , header_list) {
            stream << head << ": ";
            stream << request.rawHeader(head);
            stream << ", ";
        }
        content_length = request.header(QNetworkRequest::ContentLengthHeader).toUInt();
        content_type = request.header(QNetworkRequest::ContentTypeHeader).toString();
    } else {
    	auto header_list = reply->rawHeaderList();
        Q_FOREACH(const auto & head, header_list) {
            stream << head << ": ";
            stream << reply->rawHeader(head);
            stream << ", ";
        }
        content_length = reply->header(QNetworkRequest::ContentLengthHeader).toUInt();
        content_type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    }
    stream << "} Data: [";
    if (content_length > 0) {
        stream << QString::fromStdString(String::FormatBytes(content_length)) << " of " << content_type << " data";
    }
    stream << "]";
    XAMP_LOG_D(logger, msg.toStdString());
}

ConstLatin1String requestVerb(QNetworkAccessManager::Operation operation, const QNetworkRequest& request) {
    switch (operation) {
    case QNetworkAccessManager::HeadOperation:
        return qTEXT("HEAD");
    case QNetworkAccessManager::GetOperation:
        return qTEXT("GET");
    case QNetworkAccessManager::PutOperation:
        return qTEXT("PUT");
    case QNetworkAccessManager::PostOperation:
        return qTEXT("POST");
    case QNetworkAccessManager::DeleteOperation:
        return qTEXT("DELETE");
    case QNetworkAccessManager::CustomOperation:
        return qTEXT(request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray());
    case QNetworkAccessManager::UnknownOperation:
        break;
    }
    Q_UNREACHABLE();
}

static void logHttpRequest(const LoggerPtr& logger,
    const ConstLatin1String& verb,
    const QNetworkRequest& request,
    const QNetworkReply* reply = nullptr) {
    logHttpRequest(logger, verb, request.url().toString(), request, reply);
}

struct HttpContext {
    bool use_internal{false};
    QString charset{ kDefaultCharset };
    QString user_agent{ kDefaultUserAgent };
    QNetworkAccessManager* manager{nullptr};
    std::function<void (const QString &)> success_handler;
    std::function<void(const QString&)> error_handler;
    LoggerPtr logger;
};

class HttpClient::HttpClientImpl {
public:
    HttpClientImpl(const QString &url, QObject *parent, QNetworkAccessManager* manager);

    ~HttpClientImpl();

    HttpContext createHttpContext() const;

    void setTimeout(int timeout);

    static QNetworkRequest createHttpRequest(QSharedPointer<HttpClientImpl> d, HttpMethod method);

    static void executeQuery(QSharedPointer<HttpClientImpl> d, HttpMethod method);

    static void download(QSharedPointer<HttpClientImpl> d, std::function<void (const QByteArray &)> ready_read);

    static QString readReply(QNetworkReply *reply, const QString &charset);

    static void handleFinish(const HttpContext& context, QNetworkReply *reply, const QString &success_message);

    bool use_json_;
    bool use_internal_;
    int32_t timeout_;
    QString url_;
    QString json_;
    QUrlQuery params;
    QString charset_;
    QString user_agent_;
    QNetworkAccessManager *manager_;
    QHash<QString, QString> headers_;
    std::function<void (const QString &)> success_handler_;
    std::function<void(const QString&)> error_handler_;
    std::function<void (const QByteArray &)> download_handler_;
    LoggerPtr logger_;
};

HttpClient::HttpClientImpl::~HttpClientImpl() = default;

HttpClient::HttpClientImpl::HttpClientImpl(const QString &url, QObject *parent, QNetworkAccessManager* manager)
    : use_json_(false)
    , use_internal_(manager == nullptr)
    , timeout_(kHttpDefaultTimeout)
    , url_(url)
    , charset_(kDefaultCharset)
    , manager_(manager == nullptr ? new QNetworkAccessManager(parent) : manager) {
    logger_ = LoggerManager::GetInstance().GetLogger(kHttpLoggerName);
}

HttpContext HttpClient::HttpClientImpl::createHttpContext() const {
    HttpContext context;
    context.success_handler = success_handler_;
    context.error_handler = error_handler_;
    context.manager = manager_;
    context.charset = charset_;
    context.user_agent = user_agent_;
    context.use_internal = use_internal_;
    context.logger = logger_;
    return context;
}

void HttpClient::HttpClientImpl::setTimeout(int timeout) {
    timeout_ = timeout;
}

void HttpClient::HttpClientImpl::executeQuery(QSharedPointer<HttpClientImpl> d, HttpMethod method) {
	auto context = d->createHttpContext();

    context.manager->setProxy(QNetworkProxy::NoProxy);

    const auto request = createHttpRequest(d, method);
    QNetworkReply *reply = nullptr;

    auto operation = QNetworkAccessManager::UnknownOperation;

    switch (method) {
    case HttpMethod::HTTP_GET:
        reply = context.manager->get(request);
        operation = QNetworkAccessManager::GetOperation;
        break;
    case HttpMethod::HTTP_POST:
        reply = context.manager->post(request,
                                      d->use_json_
                                      ? d->json_.toUtf8()
                                      : d->params.toString(QUrl::FullyEncoded).toUtf8());
        operation = QNetworkAccessManager::PostOperation;
        break;
    case HttpMethod::HTTP_PUT:
        reply = context.manager->put(request,
                                     d->use_json_
                                     ? d->json_.toUtf8()
                                     : d->params.toString(QUrl::FullyEncoded).toUtf8());
        operation = QNetworkAccessManager::PutOperation;
        break;
    case HttpMethod::HTTP_DELETE:
        reply = context.manager->deleteResource(request);
        operation = QNetworkAccessManager::DeleteOperation;
        break;
    }

    logHttpRequest(context.logger, requestVerb(operation, request), request);

    (void) QObject::connect(reply, &QNetworkReply::finished, [reply, context, request, operation, d] {
        logHttpRequest(context.logger, requestVerb(operation, request), request, reply);
	    const auto success_message = readReply(reply, context.charset);
	    handleFinish(context, reply, success_message);
    });
}

void HttpClient::HttpClientImpl::download(QSharedPointer<HttpClientImpl> d, std::function<void (const QByteArray &)> ready_read) {
    auto context = d->createHttpContext();

    auto request = createHttpRequest(d, HttpMethod::HTTP_GET);
    auto* reply = context.manager->get(request);

    (void) QObject::connect(reply, &QNetworkReply::readyRead, [reply, d, ready_read] {
        ready_read(reply->readAll());
    });

    (void) QObject::connect(reply, &QNetworkReply::finished, [reply, request, context, d] {
        logHttpRequest(context.logger, requestVerb(QNetworkAccessManager::GetOperation, request), request, reply);
        handleFinish(context, reply, QString());
    });
}

void HttpClient::HttpClientImpl::handleFinish(const HttpContext &context, QNetworkReply *reply, const QString &success_message) {
    const auto error = reply->error();
    if (error == QNetworkReply::NoError) {
        if (context.success_handler != nullptr) {
            context.success_handler(success_message);
        }
    } else {
        XAMP_LOG_D(context.logger, "{}", networkErrorToString(error).data());

        if (context.error_handler != nullptr) {
            context.error_handler(networkErrorToString(error));
        }
    }

    if (reply != nullptr) {
        reply->deleteLater();
    }

    if (context.use_internal) {
        if (context.manager != nullptr) {
            context.manager->deleteLater();
        }
    }
}

QString HttpClient::HttpClientImpl::readReply(QNetworkReply *reply, const QString &charset) {
    QScopedPointer<QTextStream> in;

    if (isZipEncoding(reply)) {
        in.reset(new QTextStream(gzipUncompress(reply->readAll())));
    }
    else {
        in.reset(new QTextStream(reply));
    }

    const auto content_length_var = reply->header(QNetworkRequest::ContentLengthHeader);
    auto content_length = kHttpBufferSize;
    if (content_length_var.isValid()) {
        content_length = content_length_var.toInt();
    }

    QString result;
    result.reserve(content_length);
    in->setCodec(charset.toUtf8());

    while (!in->atEnd()) {
        result.append(in->readLine());
    }

    return result;
}

QNetworkRequest HttpClient::HttpClientImpl::createHttpRequest(QSharedPointer<HttpClientImpl> d, HttpMethod method) {
	const auto get = method == HttpMethod::HTTP_GET;
	const auto with_form = !get && !d->use_json_;
	const auto with_json = !get &&  d->use_json_;

    if (get && !d->params.isEmpty()) {
        d->url_ += qTEXT("?") + d->params.toString(QUrl::FullyEncoded);
    }

    if (with_form) {
        d->headers_[qTEXT("Content-Type")] = qTEXT("application/x-www-form-urlencoded");
    } else if (with_json) {
        d->headers_[qTEXT("Content-Type")] = qTEXT("application/json; charset=utf-8");
    }

    if (d->user_agent_.isEmpty()) {
        d->headers_[qTEXT("User-Agent")] = kDefaultUserAgent;
    }

    XAMP_LOG_D(d->logger_, "Request url: {}", QUrl(d->url_).toEncoded().toStdString());

    QNetworkRequest request(QUrl(d->url_));
    for (auto i = d->headers_.cbegin(); i != d->headers_.cend(); ++i) {
        request.setRawHeader(i.key().toUtf8(), i.value().toUtf8());
    }

    if (request.url().scheme() == qTEXT("https")) {
        static const bool http2_enabled_env = qEnvironmentVariableIntValue("OWNCLOUD_HTTP2_ENABLED") == 1;
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, http2_enabled_env);
    }

    const auto request_id = generateUUID();

    request.setRawHeader("X-Request-ID", request_id);

    // todo: Add GZIP support.
    if (isLoadZib()) {
        request.setRawHeader("Accept-Encoding", "gzip");
        XAMP_LOG_D(d->logger_, "Use gzip compression.");
    }

    request.setTransferTimeout(d->timeout_);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}

HttpClient::HttpClient(const QUrl& url, QObject *parent, QNetworkAccessManager* manager)
    : HttpClient(url.toString(), parent, manager) {
}

HttpClient::HttpClient(const QString &url, QObject *parent, QNetworkAccessManager* manager)
    : impl_(new HttpClientImpl(url, parent, manager)) {
}

HttpClient::~HttpClient() = default;

void HttpClient::setTimeout(int timeout) {
    impl_->setTimeout(timeout);
}

HttpClient& HttpClient::param(const QString &name, const QVariant &value) {
    impl_->params.addQueryItem(name, value.toString());
    return *this;
}

HttpClient& HttpClient::params(const QMap<QString, QVariant> &ps) {
    for (auto itr = ps.cbegin(); itr != ps.cend(); ++itr) {
        impl_->params.addQueryItem(itr.key(), itr.value().toString());
    }
    return *this;
}

HttpClient& HttpClient::json(const QString &json) {
    impl_->json_    = json;
    impl_->use_json_ = true;
    return *this;
}

HttpClient& HttpClient::header(const QString &name, const QString &value) {
    impl_->headers_[name] = value;
    return *this;
}

HttpClient& HttpClient::headers(const QMap<QString, QString> name_values) {
    for (auto i = name_values.cbegin(); i != name_values.cend(); ++i) {
        impl_->headers_[i.key()] = i.value();
    }
    return *this;
}

HttpClient& HttpClient::success(std::function<void (const QString &)> success_handler) {
    impl_->success_handler_ = std::move(success_handler);
    return *this;
}

HttpClient& HttpClient::error(std::function<void(const QString&)> error_handler) {
    impl_->error_handler_ = std::move(error_handler);
    return *this;
}

void HttpClient::get() {
    HttpClientImpl::executeQuery(impl_, HttpMethod::HTTP_GET);
}

void HttpClient::post() {
    HttpClientImpl::executeQuery(impl_, HttpMethod::HTTP_POST);
}

void HttpClient::downloadFile(const QString& file_name, std::function<void(const QString&)> download_handler, std::function<void(const QString&)> error_handler) {
    QSharedPointer<QTemporaryFile> tempfile(new QTemporaryFile());
    if (!tempfile->open()) {
        return;
    }

    tempfile->setAutoRemove(false);

    success([=](auto) {
        tempfile->rename(file_name);
        download_handler(file_name);
        });

    error(std::move(error_handler));

    HttpClientImpl::download(impl_, [tempfile](auto buffer) {
        tempfile->write(buffer);
        });
}

void HttpClient::download(std::function<void (const QByteArray &)> download_handler, std::function<void(const QString&)> error_handler) {
    auto data = std::make_shared<QByteArray>();

    success([handler = std::move(download_handler), data](auto) {
        handler(*data);
    });

    error(std::move(error_handler));

    HttpClientImpl::download(impl_, [data](auto buffer) {
        data->append(buffer);
    });
}

void HttpClient::setUserAgent(const QString& user_agent) {
    impl_->user_agent_ = user_agent;
}

}
