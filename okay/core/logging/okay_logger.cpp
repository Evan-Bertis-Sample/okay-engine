#include "okay_logger.hpp"

#include <cstring>
#include <ctime>

namespace okay {

OkayLogger::OkayLogger(const OkayLoggerOptions& options) : _options(options) {
#if OKAY_LOGGER_ENABLED
    if (_options.ToFile) {
        _logFileName = _makeStartFilename(_options.FilePrefix);
        _file.open(_logFileName.c_str(), std::ios::out | std::ios::trunc);
        if (!_file) {
            std::cerr << "[WARN][logger] Failed to open log file: " << _logFileName << "\n";
        }
    }
#endif
}

OkayLogger OkayLogger::createDefault() {
    OkayLoggerOptions options;
    options.ToFile = true;
    options.FilePrefix = "okay_log_";
    options.UseColor = true;
    return OkayLogger(options);
}

const char* OkayLogger::_severityString(Severity s) {
    switch (s) {
        case Severity::Debug:
            return "DEBUG";
        case Severity::Info:
            return "INFO";
        case Severity::Warning:
            return "WARN";
        case Severity::Error:
            return "ERROR";
        default:
            return "?";
    }
}

const char* OkayLogger::_severityColor(Severity s) {
    switch (s) {
        case Severity::Debug:
            return "\033[37m";
        case Severity::Info:
            return "\033[32m";
        case Severity::Warning:
            return "\033[33m";
        case Severity::Error:
            return "\033[31m";
        default:
            return "\033[0m";
    }
}

const char* OkayLogger::_colorReset() {
    return "\033[0m";
}

const char* OkayLogger::_basename(const char* path) {
    if (!path) return "";
    const char* slashForward = std::strrchr(path, '/');
#if defined(_WIN32)
    const char* slashBackward = std::strrchr(path, '\\');
    const char* cut = (slashForward && slashBackward)
                          ? ((slashForward > slashBackward) ? slashForward : slashBackward)
                          : (slashForward ? slashForward : slashBackward);
#else
    const char* cut = slashForward;
#endif
    return cut ? (cut + 1) : path;
}

std::string OkayLogger::_makeStartFilename(const std::string& prefix) {
    std::time_t now = std::time(nullptr);
    std::tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    std::ostringstream oss;
    oss << prefix << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".txt";
    return oss.str();
}

#if OKAY_LOGGER_ENABLED

void OkayLogger::_startLine(Severity sev, Verbosity, const char* file, int line) {
    _active = true;
    _toErr = (sev == Severity::Warning || sev == Severity::Error);

    _buffer.str(std::string());
    _buffer.clear();

    const char* color = _options.UseColor ? _severityColor(sev) : "";
    const char* reset = _options.UseColor ? _colorReset() : "";

    _buffer << color << '[' << _severityString(sev) << ']' << '[' << _basename(file) << ':' << line
            << "] " << reset;
}

void OkayLogger::_flush() {
    if (!_active) return;
    std::string text = _buffer.str();

    std::lock_guard<std::mutex> guard(_mtx);

    std::ostream& os = _toErr ? std::cerr : std::cout;
    os << text << '\n';
    if (_file.is_open()) {
        _file << text << '\n';
        _file.flush();
    }
    _active = false;
}

#else

void Logger::_startLine(Severity, Verbosity, const char*, int) {}
void Logger::_flush() {}

#endif  // OKAY_LOGGER_ENABLED

}  // namespace okay
