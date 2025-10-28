#ifndef OKAY_LOGGER_HPP
#define OKAY_LOGGER_HPP

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <source_location>

// 0=Debug, 1=Info, 2=Warning, 3=Error
#ifndef OKAY_COMPILED_MIN_SEVERITY
#define OKAY_COMPILED_MIN_SEVERITY 0
#endif
// 0=Silent, 1=Quiet, 2=Normal, 3=Verbose, 4=VeryVerbose
#ifndef OKAY_COMPILED_MIN_VERBOSITY
#define OKAY_COMPILED_MIN_VERBOSITY 0
#endif
// Global on/off switch
#ifndef OKAY_LOGGER_ENABLED
#define OKAY_LOGGER_ENABLED 1
#endif

namespace okay {

enum class Severity : std::uint8_t { Debug, Info, Warning, Error };
enum class Verbosity : std::uint8_t { Silent, Quiet, Normal, Verbose, VeryVerbose };

struct OkayLoggerOptions {
    bool ToFile = true;
    std::string FilePrefix = "okay_log_";
    bool UseColor = true;
};

// Compile-time filter trait
template <Severity S, Verbosity V>
struct LogEnabled : std::bool_constant<(OKAY_LOGGER_ENABLED != 0) &&
                                       (static_cast<int>(S) >= OKAY_COMPILED_MIN_SEVERITY) &&
                                       (static_cast<int>(V) >= OKAY_COMPILED_MIN_VERBOSITY)> {};

template <Severity S, Verbosity V>
inline constexpr bool LogEnabled_v = LogEnabled<S, V>::value;

class OkayLogger {
   public:
    OkayLogger() {}
    OkayLogger(const OkayLoggerOptions& options);

    template <Verbosity Verb = Verbosity::VeryVerbose,
              typename std::enable_if<LogEnabled_v<Severity::Debug, Verb>, int>::type = 0>
    OkayLogger& debug(const char* file = __FILE__, int line = __LINE__) {
        _startLine(Severity::Debug, Verb, file, line);
        return *this;
    }
    template <Verbosity Verb = Verbosity::VeryVerbose,
              typename std::enable_if<LogEnabled_v<Severity::Info, Verb>, int>::type = 0>
    OkayLogger& info(const char* file = __FILE__, int line = __LINE__) {
        _startLine(Severity::Info, Verb, file, line);
        return *this;
    }
    template <Verbosity Verb = Verbosity::VeryVerbose,
              typename std::enable_if<LogEnabled_v<Severity::Warning, Verb>, int>::type = 0>
    OkayLogger& warn(const char* file = __FILE__, int line = __LINE__) {
        _startLine(Severity::Warning, Verb, file, line);
        return *this;
    }
    template <Verbosity Verb = Verbosity::VeryVerbose,
              typename std::enable_if<LogEnabled_v<Severity::Error, Verb>, int>::type = 0>
    OkayLogger& error(const char* file = __FILE__, int line = __LINE__) {
        _startLine(Severity::Error, Verb, file, line);
        return *this;
    }

    // Disabled overloads (optimized away)
    template <Verbosity Verb = Verbosity::VeryVerbose,
              typename std::enable_if<!LogEnabled_v<Severity::Debug, Verb>, int>::type = 0>
    OkayLogger& debug(const char* = nullptr, int = 0) {
        _active = false;
        return *this;
    }
    template <Verbosity Verb = Verbosity::VeryVerbose,
              typename std::enable_if<!LogEnabled_v<Severity::Info, Verb>, int>::type = 0>
    OkayLogger& info(const char* = nullptr, int = 0) {
        _active = false;
        return *this;
    }
    template <Verbosity Verb = Verbosity::Quiet,
              typename std::enable_if<!LogEnabled_v<Severity::Warning, Verb>, int>::type = 0>
    OkayLogger& warn(const char* = nullptr, int = 0) {
        _active = false;
        return *this;
    }
    template <Verbosity Verb = Verbosity::Normal,
              typename std::enable_if<!LogEnabled_v<Severity::Error, Verb>, int>::type = 0>
    OkayLogger& error(const char* = nullptr, int = 0) {
        _active = false;
        return *this;
    }

    // Stream body
    template <typename T>
    OkayLogger& operator<<(const T& value) {
        if (_active) _buffer << value;
        return *this;
    }

    OkayLogger& operator<<(std::ostream& (*manip)(std::ostream&)) {
        if (_active) {
            // Detect std::endl and flush the log line instead of writing to the buffer
            if (manip == static_cast<std::ostream& (*)(std::ostream&)>(
                             std::endl<char, std::char_traits<char>>)) {
                _flush();
            } else {
                manip(_buffer);
            }
        }
        return *this;
    }

    OkayLogger& operator<<(std::ios_base& (*manip)(std::ios_base&)) {
        if (_active) manip(_buffer);
        return *this;
    }

    OkayLogger& endl() {
        _flush();
        return *this;
    }

    // One-shot helpers
    template <Verbosity Verb = Verbosity::VeryVerbose, typename T>
    void infoln(const T& v, const char* file = __FILE__, int line = __LINE__) {
        info<Verb>(file, line) << v;
        endl();
    }
    template <Verbosity Verb = Verbosity::VeryVerbose, typename T>
    void debugln(const T& v, const char* file = __FILE__, int line = __LINE__) {
        debug<Verb>(file, line) << v;
        endl();
    }
    template <Verbosity Verb = Verbosity::Quiet, typename T>
    void warnln(const T& v, const char* file = __FILE__, int line = __LINE__) {
        warn<Verb>(file, line) << v;
        endl();
    }
    template <Verbosity Verb = Verbosity::Normal, typename T>
    void errorln(const T& v, const char* file = __FILE__, int line = __LINE__) {
        error<Verb>(file, line) << v;
        endl();
    }

   private:
    OkayLoggerOptions _options;
    std::ostringstream _buffer;
    std::ofstream _file;
    std::mutex _mtx;
    bool _active = false;
    bool _toErr = false;
    std::string _logFileName;

    // helpers
    static const char* _severityString(Severity s);
    static const char* _severityColor(Severity s);
    static const char* _colorReset();
    static const char* _basename(const char* path);
    static std::string _makeStartFilename(const std::string& prefix);

    void _startLine(Severity sev, Verbosity verb, const char* file, int line);
    void _flush();
};

}  // namespace okay

#endif  // OKAY_LOGGER_HPP
