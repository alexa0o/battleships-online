#include <userver/utest/using_namespace_userver.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/json.hpp>

#include <cors.hpp>

#include <options.hpp>

class ImplicitOptions final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-implicit-options";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(const server::http::HttpRequest& request,
                                   server::request::RequestContext&) const override {
        SetCors(request);
        return "";
    }
};

void AppendOptions(userver::components::ComponentList &component_list) {
    component_list.Append<ImplicitOptions>();
}
