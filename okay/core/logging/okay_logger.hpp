#ifndef OKAY_LOGGER_HPP
#define OKAY_LOGGER_HPP

#include <array>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#ifndef OKAY_COMPILED_MIN_SEVERITY
#define OKAY_COMPILED_MIN_SEVERITY 0
#endif

#ifndef OKAY_COMPILED_MIN_VERBOSITY
#define OKAY_COMPILED_MIN_VERBOSITY 0
#endif

#ifndef OKAY_LOGGER_ENABLED
#define OKAY_LOGGER_ENABLED 1
#endif

namespace okay {

enum class Severity : std::uint8_t { DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3 };
enum class Verbosity : std::uint8_t {
    SILENT = 0,
    QUIET = 1,
    NORMAL = 2,
    VERBOSE = 3,
    VERY_VERBOSE = 4
};

struct OkayLoggerOptions {
    bool ToFile = true;
    std::string FilePrefix = "okay_log_";
};

struct LogPhrases {
    static constexpr std::array<std::string_view, 4> SEVERITY_TAG = {"[DEBUG]", "[INFO]", "[WARN]",
                                                                     "[ERROR]"};

    static constexpr std::array<std::string_view, 4> SEVERITY_COLOR = {"\033[37m", "\033[32m",
                                                                       "\033[33m", "\033[31m"};

    static constexpr std::string_view COLOR_RESET = "\033[0m";

    static constexpr std::string_view severityTag(Severity s) {
        return SEVERITY_TAG[static_cast<std::size_t>(s)];
    }

    static constexpr std::string_view severityColor(Severity s, bool enableColor) {
        return enableColor ? SEVERITY_COLOR[static_cast<std::size_t>(s)] : std::string_view{};
    }
};

struct OkayLog final {
    std::string_view fmt;
    std::source_location loc;

    template<typename T>
    consteval OkayLog(T&& fmt,
                      std::source_location loc = std::source_location::current())
        : fmt(std::forward<T>(fmt)), loc(loc) {}

    template <Severity S, Verbosity V>
    static consteval bool logEnabled() {
        return (OKAY_LOGGER_ENABLED != 0) && (static_cast<int>(S) >= OKAY_COMPILED_MIN_SEVERITY) &&
               (static_cast<int>(V) >= OKAY_COMPILED_MIN_VERBOSITY);
    }

    template <Severity S, Verbosity V, typename... Ts>
    void invoke(std::ostream& os, bool enableColor, Ts&&... ts) const {
        os << LogPhrases::severityColor(S, enableColor);
        os << LogPhrases::severityTag(S) << '[' << loc.file_name() << ':' << loc.line() << "] ";
        os << std::vformat(fmt, std::make_format_args(ts...));
        if (enableColor) os << LogPhrases::COLOR_RESET;
        os << '\n';
    }
};

class OkayLogger {
   public:
    OkayLogger() = default;
    explicit OkayLogger(const OkayLoggerOptions& options) : _options(options) {
        _openFileIfNeeded();
    }

    void setOptions(const OkayLoggerOptions& options) {
        std::lock_guard<std::mutex> g(_mtx);
        _options = options;
        if (_file.is_open()) {
            _file.flush();
            _file.close();
        }
        _logFileName.clear();
        _openFileIfNeeded();
    }

    template <Verbosity V = Verbosity::NORMAL, typename... Ts>
    void debug(OkayLog log, Ts&&... ts) {
        _emit<Severity::DEBUG, V, Ts...>(std::cout, log, std::forward<Ts>(ts)...);
    }

    template <Verbosity V = Verbosity::NORMAL, typename... Ts>
    void info(OkayLog log, Ts&&... ts) {
        _emit<Severity::INFO, V, Ts...>(std::cout, log, std::forward<Ts>(ts)...);
    }

    template <Verbosity V = Verbosity::NORMAL, typename... Ts>
    void warning(OkayLog log, Ts&&... ts) {
        _emit<Severity::WARNING, V, Ts...>(std::cerr, log, std::forward<Ts>(ts)...);
    }

    template <Verbosity V = Verbosity::NORMAL, typename... Ts>
    void error(OkayLog log, Ts&&... ts) {
        _emit<Severity::ERROR, V, Ts...>(std::cerr, log, std::forward<Ts>(ts)...);
    }

   private:
    OkayLoggerOptions _options{};
    std::ofstream _file{};
    std::mutex _mtx{};
    std::string _logFileName{};

    template <Severity S, Verbosity V, typename... Ts>
    void _emit(std::ostream& os, const OkayLog& log, Ts&&... ts) {
        if constexpr (!OkayLog::logEnabled<S, V>()) return;

        std::lock_guard<std::mutex> g(_mtx);

        log.invoke<S, V, Ts...>(os, true, std::forward<Ts>(ts)...);

        if (_file.is_open()) {
            log.invoke<S, V, Ts...>(_file, false, std::forward<Ts>(ts)...);
            _file.flush();
        }
    }

    static std::string _makeStartFilename(const std::string& prefix);

    void _openFileIfNeeded() {
        if (!_options.ToFile) return;
        if (_logFileName.empty()) _logFileName = _makeStartFilename(_options.FilePrefix);
        _file.open(_logFileName.c_str(), std::ios::out | std::ios::trunc);
        if (!_file) {
            std::cerr << "[WARN][logger] Failed to open log file: " << _logFileName << "\n";
        }
    }
};

}  // namespace okay

#endif  // OKAY_LOGGER_HPP
