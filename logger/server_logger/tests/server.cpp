#include "server.h"

#include <logger_builder.h>

#include <fstream>
#include <iostream>


server::server(uint16_t port) {
    CROW_ROUTE(app, "/create")([&](const crow::request &req) {
        std::string string_pid = req.url_params.get("pid");
        std::string string_severity = req.url_params.get("sev");
        std::string string_path = req.url_params.get("path");
        std::string use_console = req.url_params.get("console");
        int pid = std::stoi(string_pid);
        logger::severity sev = logger_builder::string_to_severity(string_severity);
        bool console = (use_console == "1");
        std::lock_guard lock(_mut);
        auto iter = _streams.find(pid);
        if (iter == _streams.end()) {
            iter = _streams.emplace(pid, std::unordered_map<logger::severity, std::pair<std::string, bool>>()).first;
        }
        auto sev_iter = iter->second.find(sev);
        if (sev_iter == iter->second.end()) {
            sev_iter = iter->second.emplace(sev, std::make_pair(std::string(), false)).first;
        }
        sev_iter->second.first = std::move(string_path);
        sev_iter->second.second = console;
        return crow::response(204);
    });

    CROW_ROUTE(app, "/log")([&](const crow::request &req) {
        std::string string_pid = req.url_params.get("pid");
        std::string string_severity = req.url_params.get("sev");
        std::string message = req.url_params.get("message");
        int pid = std::stoi(string_pid);
        logger::severity sev = logger_builder::string_to_severity(string_severity);
        std::shared_lock lock(_mut);
        auto iter = _streams.find(pid);
        if (iter == _streams.end()) {
            return crow::response(204);
        }
        auto sev_iter = iter->second.find(sev);
        if (sev_iter == iter->second.end()) {
            return crow::response(204);
        }
        const std::string &path = sev_iter->second.first;
        if (!path.empty()) {
            std::ofstream stream(path, std::ios_base::app);
            if (stream.is_open()) {
                stream << message << std::endl;
            }
        }
        if (sev_iter->second.second) {
            std::cout << message << std::endl;
        }
        return crow::response(204);
    });

    CROW_ROUTE(app, "/stop")([&](const crow::request &req) {
        std::string string_pid = req.url_params.get("pid");
        int current_pid = std::stoi(string_pid);
        std::lock_guard lock(_mut);
        _streams.erase(current_pid);
        return crow::response(204);
    });

    app.port(port).loglevel(crow::LogLevel::Warning).multithreaded();
    app.run();
}