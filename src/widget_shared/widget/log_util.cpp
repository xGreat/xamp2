#include <QMap>
#include <widget/log_util.h>
#include <widget/str_utilts.h>

namespace log_util {

QString GetLogLevelString(LogLevel level) {
    static const QMap<LogLevel, QString> logs{
        { LogLevel::LOG_LEVEL_INFO, qTEXT("info") },
        { LogLevel::LOG_LEVEL_DEBUG, qTEXT("debug") },
        { LogLevel::LOG_LEVEL_TRACE, qTEXT("trace") },
        { LogLevel::LOG_LEVEL_WARN, qTEXT("warn") },
        { LogLevel::LOG_LEVEL_ERROR, qTEXT("err") },
        { LogLevel::LOG_LEVEL_CRITICAL, qTEXT("critical") },
    };
    if (!logs.contains(level)) {
        return qTEXT("info");
    }
    return logs[level];
}

LogLevel ParseLogLevel(const QString &str) {
    static const QMap<QString, LogLevel> logs{
    	{ qTEXT("info"), LogLevel::LOG_LEVEL_INFO},
        { qTEXT("debug"), LogLevel::LOG_LEVEL_DEBUG},
        { qTEXT("trace"), LogLevel::LOG_LEVEL_TRACE},
        { qTEXT("warn"), LogLevel::LOG_LEVEL_WARN},
        { qTEXT("err"), LogLevel::LOG_LEVEL_ERROR},
        { qTEXT("critical"), LogLevel::LOG_LEVEL_CRITICAL},
    };
    if (!logs.contains(str)) {
        return LogLevel::LOG_LEVEL_INFO;
    }
    return logs[str];
}


}