#include <utility>
#include <not_implemented.h>
#include "../include/client_logger_builder.h"

using namespace nlohmann;

logger_builder &client_logger_builder::add_file_stream(
        std::string const &stream_file_path,
        logger::severity severity) &{
    auto &[streams, flag] = _output_streams[severity];
    bool is_existing = std::any_of(
            streams.begin(),
            streams.end(),
            [&](const client_logger::refcounted_stream &s) {
                return std::filesystem::weakly_canonical(s._stream.first) ==
                       std::filesystem::weakly_canonical(stream_file_path);;
            });

    if (!is_existing) {
        streams.emplace_front(stream_file_path);
    }
    return *this;
}

logger_builder &client_logger_builder::add_console_stream(
        logger::severity severity) &{
    auto &[streams, flag] = _output_streams[severity];
    flag = true;
    return *this;
}

logger_builder &client_logger_builder::transform_with_configuration(
        std::string const &configuration_file_path,
        std::string const &configuration_path) &{
    std::ifstream file(configuration_file_path);
    json data = json::parse(file);
    json::json_pointer ptr(configuration_path);
    json config = data[ptr];
    if (config.contains("format")) {
        _format = config["format"].get<std::string>();
    }
    if (config.contains("streams")) {
        for (auto &[sev, dest]: config["streams"].items()) {
            logger::severity severity = string_to_severity(sev);
            parse_severity(severity, dest);
        }
    }
    return *this;
}

logger_builder &client_logger_builder::clear() &{
    _output_streams.clear();
    _format.clear();
    return *this;
}

logger *client_logger_builder::build() const {
    return new client_logger(_output_streams, _format);
}

logger_builder &client_logger_builder::set_format(const std::string &format) &{
    _format = format;
    return *this;
}

void client_logger_builder::parse_severity(logger::severity sev, nlohmann::json &j) {
    if (j.contains("file")) {
        std::string path = j["file"].get<std::string>();
        add_file_stream(path, sev);
    }
    if (j.contains("console") && j["console"].get<bool>()) {
        add_console_stream(sev);
    }
}

logger_builder &client_logger_builder::set_destination(const std::string &format) &{
    return *this;
}
