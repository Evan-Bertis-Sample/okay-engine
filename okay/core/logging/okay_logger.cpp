#include "okay_logger.hpp"

#include <iomanip>
#include <sstream>
#include <chrono>
#include <filesystem>

using namespace okay;

std::string OkayLogger::makeStartFilename(const std::string& prefix) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time);
    std::stringstream ss;
    // prepend cwd / logs
    ss << std::filesystem::current_path() / "logs" / prefix
       << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".log";
    return ss.str();
}

void OkayLogger::openFileIfNeeded() {
    if (!_options.ToFile) return;
    if (_logFileName.empty()) _logFileName = makeStartFilename(_options.FilePrefix);
    // make the file if it doesn't exist
    std::lock_guard<std::mutex> g(_mtx);
    _file.open(_logFileName, std::ios::out | std::ios::app);
    if (!_file.is_open()) {
        auto log = OkayLog("Failed to open log file: {}");
        log.invoke<Severity::ERROR, Verbosity::NORMAL>(std::cerr, true, _logFileName);
    }
}

void OkayLogger::setOptions(const OkayLoggerOptions& options) {
    std::lock_guard<std::mutex> g(_mtx);
    _options = options;
    if (_file.is_open()) {
        _file.flush();
        _file.close();
    }
    _logFileName.clear();
    openFileIfNeeded();
}

OkayLogger::OkayLogger(const OkayLoggerOptions& options) : _options(options) {
    openFileIfNeeded();
}
