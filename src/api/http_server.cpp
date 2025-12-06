#include "http_server.hpp"
#include "metrics.hpp"
#include "../common/logging.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>

namespace hft {

HttpServer::HttpServer(MatchingEngine& engine) : engine_(engine) {}

void HttpServer::start(uint16_t port) {
    running_ = true;
    
    httplib::Server svr;
    
    // Health check
    svr.Get("/healthz", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("OK", "text/plain");
    });
    
    // Readiness check
    svr.Get("/readyz", [this](const httplib::Request&, httplib::Response& res) {
        bool ready = engine_.processed_count() > 0;
        res.status = ready ? 200 : 503;
        res.set_content(ready ? "READY" : "NOT READY", "text/plain");
    });
    
    // Metrics
    svr.Get("/metrics", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(Metrics::instance().prometheus_format(), "text/plain");
    });
    
    // Order count
    svr.Get("/orders/count", [this](const httplib::Request&, httplib::Response& res) {
        nlohmann::json j;
        j["processed"] = engine_.processed_count();
        res.set_content(j.dump(), "application/json");
    });
    
    // State snapshot
    svr.Get("/state/snapshot", [this](const httplib::Request&, httplib::Response& res) {
        nlohmann::json j;
        j["processed"] = engine_.processed_count();
        j["status"] = "running";
        res.set_content(j.dump(), "application/json");
    });
    
    LOG_INFO("HTTP server starting on port: ", port);
    svr.listen("0.0.0.0", port);
}

void HttpServer::stop() {
    running_ = false;
    LOG_INFO("HTTP server stopped");
}

} // namespace hft
