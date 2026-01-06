#ifndef OKAY_LOGGER_HPP
#define OKAY_LOGGER_HPP

#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>

#include <utility>

#include <ctime>

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
    bool ToFile{true};
    std::string FilePrefix{"okay_log_"};
};

struct LogPhrases {
    static constexpr std::string_view SEVERITY_TAG[4] = {"[DEBUG]", "[INFO]", "[WARN]", "[ERROR]"};

    static constexpr std::string_view SEVERITY_COLOR[4] = {"\03[37m", "\033[32m", "\033[33m",
                                                           "\033[31m"};

    static constexpr std::string_view COLOR_RESET = "\033[0m";

    template <Severity s>
    static constexpr std::string_view severityTag() {
        return SEVERITY_TAG[static_cast<std::size_t>(s)];
    }

    template <Severity S>
    static std::string_view severityColor(bool enableColor) {
        return enableColor ? SEVERITY_COLOR[static_cast<std::size_t>(S)] : "";
    }
};

struct OkayLog final {
    std::string_view fmt;
    std::source_location loc;

    template <typename T>
    consteval OkayLog(T&& fmt, std::source_location loc = std::source_location::current())
        : fmt(std::forward<T>(fmt)), loc(loc) {}

    template <Severity S, Verbosity V>
    static consteval bool logEnabled() {
        return (OKAY_LOGGER_ENABLED != 0) && (static_cast<int>(S) >= OKAY_COMPILED_MIN_SEVERITY) &&
               (static_cast<int>(V) >= OKAY_COMPILED_MIN_VERBOSITY);
    }

    template <Severity S, Verbosity V, typename... Ts>
    void invoke(std::ostream& os, bool enableColor, Ts&&... ts) const {
        os << LogPhrases::severityColor<S>(enableColor);
        os << LogPhrases::severityTag<S>() << '[' << loc.file_name() << ':' << loc.line()
           << "] ";
        os << std::vformat(fmt, std::make_format_args(ts...));
        if (enableColor) os << LogPhrases::COLOR_RESET;
        os << '\n';
    }
};

class OkayLogger {
   public:
    OkayLogger() = default;

    explicit OkayLogger(const OkayLoggerOptions& options);

    void setOptions(const OkayLoggerOptions& options);

    template <Verbosity V = Verbosity::NORMAL, typename... Ts>
    void debug(OkayLog log, Ts&&... ts) {
        emit<Severity::DEBUG, V, Ts...>(std::cout, log, std::forward<Ts>(ts)...);
    }

    template <Verbosity V = Verbosity::NORMAL, typename... Ts>
    void info(OkayLog log, Ts&&... ts) {
        emit<Severity::INFO, V, Ts...>(std::cout, log, std::forward<Ts>(ts)...);
    }

    template <Verbosity V = Verbosity::NORMAL, typename... Ts>
    void warn(OkayLog log, Ts&&... ts) {
        emit<Severity::WARNING, V, Ts...>(std::cerr, log, std::forward<Ts>(ts)...);
    }

    template <Verbosity V = Verbosity::NORMAL, typename... Ts>
    void error(OkayLog log, Ts&&... ts) {
        emit<Severity::ERROR, V, Ts...>(std::cerr, log, std::forward<Ts>(ts)...);
    }

   private:
    OkayLoggerOptions _options{};
    std::ofstream _file{};
    std::mutex _fileMtx{};
    std::mutex _optionsMtx{};
    std::string _logFileName{};
    bool _triedToOpenFile{false};

    template <Severity S, Verbosity V, typename... Ts>
    void emit(std::ostream& os, const OkayLog& log, Ts&&... ts) {
        if constexpr (!OkayLog::logEnabled<S, V>()) return;

        log.invoke<S, V, Ts...>(os, true, std::forward<Ts>(ts)...);

        if (_options.ToFile && !_triedToOpenFile) {
            std::lock_guard g(_fileMtx);
            openFileIfNeeded();
            _triedToOpenFile = true;
        }

        if (_file.is_open()) {
            std::lock_guard g(_fileMtx);
            log.invoke<S, V, Ts...>(_file, false, std::forward<Ts>(ts)...);
            _file.flush();
        }
    }

    static std::string makeStartFilename(const std::string& prefix);

    void openFileIfNeeded();
};

}  // namespace okay

#endif  // OKAY_LOGGER_HPP
