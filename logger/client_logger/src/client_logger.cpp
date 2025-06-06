#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
#include "../include/client_logger.h"
#include <not_implemented.h>

std::unordered_map<std::string, std::pair<size_t, std::ofstream>> client_logger::refcounted_stream::_global_streams;


logger& client_logger::log(
    const std::string &text,
    logger::severity severity) &
{
    std::string message = make_format(text, severity);
    auto it = _output_streams.find(severity);
    if (it == _output_streams.end()) {
        return *this;
    }
    for (auto& stream: it->second.first) {
        if (stream._stream.second && stream._stream.second->is_open())
            *(stream._stream.second) << message << "\n";
    }
    if (it->second.second) {
        std::cout << message << "\n";
    }
    return *this;
}

std::string client_logger::make_format(const std::string &message, severity sev) const
{
    std::string res;
    size_t last_pos = 0;
    size_t pos = 0;
    const std::string datetime_str = current_datetime_to_string();
    const std::string time_str = current_time_to_string();
    const std::string severity_str = severity_to_string(sev);
    while ((pos = _format.find('%', last_pos)) != std::string::npos) {
        res.append(_format.substr(last_pos, pos - last_pos));
        if (pos + 1 >= _format.size()) {
            res.append("%");
            break;
        }
        flag spec = char_to_flag(_format[pos + 1]);
        switch (spec) {
            case flag::DATE:
                res.append(datetime_str);
                break;
            case flag::TIME:
                res.append(time_str);
                break;
            case flag::SEVERITY:
                res.append(severity_str);
                break;
            case flag::MESSAGE:
                res.append(message);
                break;
            default:
                res.append(std::string("%") + _format[pos + 1]);
        }
        last_pos = pos + 2;
    }
    res.append(_format.substr(last_pos));
    return res;
}

client_logger::client_logger(
        const std::unordered_map<logger::severity, std::pair<std::forward_list<refcounted_stream>, bool>> &streams,
        std::string format): _output_streams(streams), _format(std::move(format))
{
}

client_logger::flag client_logger::char_to_flag(char c) noexcept
{
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

client_logger::client_logger(const client_logger &other): _output_streams(other._output_streams), _format(other._format)
{
}

client_logger &client_logger::operator=(const client_logger &other)
{
    client_logger client(other);
    std::swap(*this, client);
    return *this;
}

client_logger::client_logger(client_logger &&other) noexcept:
                                                    _format(std::move(other._format)),
                                                    _output_streams(std::move(other._output_streams))
{
}

client_logger& client_logger::operator=(client_logger&& other) noexcept {
    if (this == &other) return *this;
    _format.clear();
    _output_streams.clear();
    _format = std::move(other._format);
    _output_streams = std::move(other._output_streams);
    return *this;
}

client_logger::~client_logger() noexcept
{

}


client_logger::refcounted_stream::refcounted_stream(const std::string &path)
{
    auto [it, inserted] = _global_streams.try_emplace(path, 0, std::ofstream(path, std::ios::out));
    if (!it->second.second.is_open()) {
        if (inserted) {
            _global_streams.erase(it);
        }
        throw std::runtime_error("error while opening stream: " + path);
    }
    it->second.first++;
    _stream = {it->first, &it->second.second};
}


client_logger::refcounted_stream::refcounted_stream(const client_logger::refcounted_stream &oth)
{
    auto it = _global_streams.find(oth._stream.first);
    if (it == _global_streams.end()) {
        throw std::runtime_error("error while finding stream in map in copy constructor");
    }
    it->second.first++;
    _stream.first = oth._stream.first;
    _stream.second = oth._stream.second;
}

client_logger::refcounted_stream&
client_logger::refcounted_stream::operator=(const refcounted_stream& oth) {
    if (this == &oth) return *this;

    auto it = _global_streams.find(oth._stream.first);
    if (it == _global_streams.end()) {
        throw std::runtime_error("error while finding stream");
    }
    if (!it->second.second.is_open()) {
        throw std::runtime_error("stream is closed");
    }

    ++it->second.first;
    const std::string new_key = it->first;

    try {
        if (!_stream.first.empty()) {
            auto old = _global_streams.find(_stream.first);
            if (old == _global_streams.end()) {
                throw std::runtime_error("error while finding stream");
            }
            if (--old->second.first == 0) {
                old->second.second.close();
                _global_streams.erase(old);
            }
        }
        _stream.first = new_key;
        _stream.second = &it->second.second;
    } catch (...) {
        --it->second.first;
        throw;
    }
    return *this;
}


client_logger::refcounted_stream::refcounted_stream(client_logger::refcounted_stream&& oth) noexcept: _stream(std::move(oth._stream)){
    oth._stream.first.clear();
    oth._stream.second = nullptr;
}

client_logger::refcounted_stream &client_logger::refcounted_stream::operator=(client_logger::refcounted_stream &&oth) noexcept
{
    if (this == &oth) {
        return *this;
    }
    if (!_stream.first.empty()) {
        auto old_it = _global_streams.find(_stream.first);
        if (old_it != _global_streams.end()) {
            if (--old_it->second.first == 0) {
                old_it->second.second.close();
                _global_streams.erase(old_it);
            }
        }
    }
    _stream.first = std::move(oth._stream.first);
    _stream.second = oth._stream.second;
    oth._stream.first.clear();
    oth._stream.second = nullptr;
    return *this;
}

client_logger::refcounted_stream::~refcounted_stream()
{
    auto it = _global_streams.find(_stream.first);
    if (it == _global_streams.end()) {
        return;
    }
    it->second.first--;
    if (it->second.first == 0) {
        it->second.second.close();
        _global_streams.erase(it);
    }
}
