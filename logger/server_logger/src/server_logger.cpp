#include "../include/server_logger.h"

#include <httplib.h>
#include <not_implemented.h>

#ifdef _WIN32
#include <process.h>
#include <fstream>
#include <sstream>
#include <utility>
#else

#include <unistd.h>
#endif

server_logger::~server_logger() noexcept {
    std::string current_pid = std::to_string(inner_getpid());
    auto result = _client.Get("/stop?pid=" + current_pid);
}

logger &server_logger::log(const std::string &text, logger::severity severity) & {
    std::string current_pid = std::to_string(inner_getpid());
    std::string message = make_format(text, severity);
    std::string url = "/log?pid=" + current_pid + "&sev=" + severity_to_string(severity) + "&message=" + message;
    auto result = _client.Get(url);
    return *this;
}

std::string server_logger::make_format(const std::string &message, severity sev) const {
    std::stringstream result;
    for (size_t i = 0; i < _format.length(); ++i) {
        if (_format[i] == '%' && i + 1 < _format.length()) {
            switch (char_to_flag(_format[i + 1])) {
                case flag::DATE:
                    result << current_date_to_string();
                    break;
                case flag::TIME:
                    result << current_time_to_string();
                    break;
                case flag::SEVERITY:
                    result << severity_to_string(sev);
                    break;
                case flag::MESSAGE:
                    result << message;
                    break;
                default:
                    result << '%' << _format[i + 1];
            }
            ++i;
        } else {
            result << _format[i];
        }
    }
    return result.str();
}

server_logger::flag server_logger::char_to_flag(char c) noexcept {
    switch (c) {
        case 'd':
            return flag::DATE;
        case 't':
            return flag::TIME;
        case 's':
            return flag::SEVERITY;
        case 'm':
            return flag::MESSAGE;
        default:
            return flag::NO_FLAG;
    }
}

server_logger::server_logger(const std::string &dest, const std::unordered_map<logger::severity, std::pair<std::string, bool> > &streams,
                             std::string format) : _client(dest), _streams(streams), _format(std::move(format)) {
    for (const auto &[cur_sev, info]: streams) {
        auto url = "/create?pid=" + std::to_string(inner_getpid()) + "&sev=" +
                severity_to_string(cur_sev) + "&path=" + info.first + "&console=" + std::to_string(+info.second);
        auto result = _client.Get(url);
    }
}

int server_logger::inner_getpid() {
#ifdef _WIN32
    return ::_getpid();
#elif
    return getpid();
#endif
}

server_logger::server_logger(server_logger &&other) noexcept : _client(std::move(other._client)), _format(std::move(other._format)) {}

server_logger &server_logger::operator=(server_logger &&other) noexcept {
    if (this == &other) {
        return *this;
    }
    _client = std::move(other._client);
    _format = std::move(other._format);
    return *this;
}