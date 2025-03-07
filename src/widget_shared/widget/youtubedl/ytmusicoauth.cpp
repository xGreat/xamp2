#include <QJsonDocument>
#include <QDesktopServices>
#include <QSaveFile>
#include <QDateTime>
#include <QJsonObject>
#include <QFile>

#include <widget/youtubedl/ytmusicoauth.h>

inline constexpr auto kOAuthCodeUrl = qTEXT("https://www.youtube.com/o/oauth2/device/code");
inline constexpr auto kOAuthYouTubeScope = qTEXT("https://www.googleapis.com/auth/youtube");

inline constexpr auto kOAuthTokenUrl = qTEXT("https://oauth2.googleapis.com/token");
inline constexpr auto kOAuthClientSecret = qTEXT("SboVhoG9s0rNafixCSGGKXAT");

inline constexpr auto kOAuthClientId =
qTEXT("861556708454-d6dlm3lh05idd8npek18k6be8ba3oc68.apps.googleusercontent.com");

inline constexpr auto kOAuthUserAgent =
qTEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:88.0) Gecko/20100101 Firefox/88.0 Cobalt/Version");

inline constexpr auto kGrantType = qTEXT("urn:ietf:params:oauth:grant-type:device_code");

XAMP_DECLARE_LOG_NAME(YtMusicOAuth);

YtMusicOAuth::YtMusicOAuth() {
	logger_ = XampLoggerFactory.GetLogger(kYtMusicOAuthLoggerName);
}

void YtMusicOAuth::setup() {
	http::HttpClient(kOAuthCodeUrl)
		.header(qTEXT("User-Agent"), kOAuthUserAgent)
		.param(qTEXT("client_id"), kOAuthClientId)
		.param(qTEXT("scope"), kOAuthYouTubeScope)
		.error([this](const auto& url, const auto& content) {
			emit setupError();
		})
		.success([this](const auto &url, const auto &content) {
		XAMP_LOG_DEBUG("{}", content.toStdString());

		QJsonParseError error;
		auto code = QJsonDocument::fromJson(content.toUtf8(), &error);
		if (error.error != QJsonParseError::NoError) {
			emit setupError();
			return;
		}

		user_code_ = code[qTEXT("user_code")].toString();
		verification_url_ = code[qTEXT("verification_url")].toString();
		device_code_ = code[qTEXT("device_code")].toString();

		const auto open_url = qSTR("%1?user_code=%2").arg(verification_url_).arg(user_code_);
		emit acceptAuthorization(open_url);
		})
		.post();
}

void YtMusicOAuth::requestGrant() {
	http::HttpClient(kOAuthTokenUrl)
		.header(qTEXT("User-Agent"), kOAuthUserAgent)
		.param(qTEXT("client_secret"), kOAuthClientSecret)
		.param(qTEXT("grant_type"), kGrantType)
		.param(qTEXT("device_code"), device_code_)
		.param(qTEXT("client_id"), kOAuthClientId)
		.error([this](const auto& url, const auto& content) {
			emit requestGrantError();
		})
		.success([this](const auto& url, const auto& content) {
		XAMP_LOG_DEBUG("{}", content.toStdString());

		QJsonParseError error;
		auto code = QJsonDocument::fromJson(content.toUtf8(), &error);
		if (error.error != QJsonParseError::NoError) {
			emit requestGrantError();
			return;
		}
		auto root = code.object();
		root[qTEXT("expires_at")] = QDateTime::currentSecsSinceEpoch() + code[qTEXT("expires_in")].toInt();
		root[qTEXT("filepath")] = qTEXT("oauth.json");
		code.setObject(root);
		auto json = code.toJson();

		QSaveFile file(qTEXT("oauth.json"));
		file.open(QIODevice::WriteOnly);
		file.write(json);
		if (file.commit()) {
			emit requestGrantCompleted();
		} else {
			emit requestGrantError();
		}
	})
	.post();
}

Expected<OAuthToken, std::string> YtMusicOAuth::parseOAuthJson() {
	QFile file(qTEXT("oauth.json"));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return MakeUnexpected("Failed to open file.");
	}

	const auto content = file.readAll();
	QJsonParseError error;
	const auto code = QJsonDocument::fromJson(content, &error);
	if (error.error != QJsonParseError::NoError) {
		return MakeUnexpected("Failed to parse json ({}).", error.errorString().toStdString());
	}

	const auto root = code.object();
	const QList<QString> keys{
		qTEXT("access_token"),
		qTEXT("expires_in"),
		qTEXT("refresh_token"),
		qTEXT("scope"),
		qTEXT("token_type"),
		qTEXT("expires_at"),
		qTEXT("filepath"),
	};

	OAuthToken token;
	Q_FOREACH(auto key, keys) {
		if (!root.contains(key)) {
			return MakeUnexpected("Not found json key: {}", key.toStdString());
		}
	}

	token.expires_in = root[qTEXT("expires_in")].toInteger();
	token.expires_at = root[qTEXT("expires_at")].toInteger();
	token.refresh_token = root[qTEXT("refresh_token")].toString();
	token.access_token = root[qTEXT("access_token")].toString();
	token.token_type = root[qTEXT("token_type")].toString();
	token.filepath = root[qTEXT("filepath")].toString();
	token.scope = root[qTEXT("scope")].toString();

	return token;
}
