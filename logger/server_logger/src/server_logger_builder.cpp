#include "../include/server_logger_builder.h"

#include <not_implemented.h>

#include <fstream>
#include <utility>

using namespace nlohmann;

logger_builder& server_logger_builder::add_file_stream(std::string const& stream_file_path, logger::severity severity) & {
    auto current_stream = _output_streams.find(severity);
    if (current_stream == _output_streams.end()){
        current_stream = _output_streams.emplace(severity, std::make_pair(std::string(), false)).first;
    }
    current_stream->second.first = stream_file_path;
    return *this;
}

logger_builder& server_logger_builder::add_console_stream(logger::severity severity) & {
    auto current_stream = _output_streams.find(severity);
    if (current_stream == _output_streams.end()){
        current_stream = _output_streams.emplace(severity, std::make_pair(std::string(), false)).first;
    }
    current_stream->second.second = true;
    return *this;
}

logger_builder& server_logger_builder::transform_with_configuration(std::string const& configuration_file_path, std::string const& configuration_path) & {
    json js_file;
    std::ifstream stream(configuration_file_path);
    if (!stream.is_open()) {
        return *this;
    }
    json::parser_callback_t check_function = [&configuration_path](int depth, json::parse_event_t event, json& file) {
        if (event == json::parse_event_t::key && depth == 1 && file != json(configuration_path)) {
            return false;
        }
        return true;
    };
    js_file = json::parse(stream, check_function);
    stream.close();
    if (!js_file.contains(configuration_path)) {
        return *this;
    }
    js_file = js_file[configuration_path];
    if (js_file.contains("format")) {
        set_format(js_file["format"]);
    }
    if (!js_file.contains("files")) {
        return *this;
    }
    for (auto& item : js_file["files"]) {
        std::string type = item["type"];
        if (type == "file" && item.contains("path") && item.contains("sevs")) {
            std::string path = item["path"];
            for (auto& sev : item["sevs"]) {
                add_file_stream(path, string_to_severity(sev.get<std::string>()));
            }
        } else if (type == "cons" && item.contains("sevs")) {
            for (auto& sev : item["sevs"]) {
                add_console_stream(string_to_severity(sev.get<std::string>()));
            }
        }
    }
    return *this;
}

logger_builder& server_logger_builder::clear() & {
    _output_streams.clear();
    _destination = "http://127.0.0.1:9200";
    return *this;
}

logger* server_logger_builder::build() const {
    return new server_logger(_destination, _output_streams, _format);
}

logger_builder& server_logger_builder::set_destination(const std::string& dest) & {
    _destination = dest;
    return *this;
}

logger_builder& server_logger_builder::set_format(const std::string& format) & {
    return *this;
}