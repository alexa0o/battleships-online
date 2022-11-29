#pragma once

#include <userver/server/handlers/http_handler_base.hpp>

inline void SetCors(const userver::server::http::HttpRequest& request) {
    auto& response = request.GetHttpResponse();
    response.SetHeader("Access-Control-Allow-Origin", "http://158.160.37.44:8000");
    response.SetHeader("Access-Control-Allow-Methods", "*");
    response.SetHeader("Access-Control-Allow-Headers", "content-type");
}
