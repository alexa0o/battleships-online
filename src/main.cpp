#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>


#include "hello.hpp"
#include "registration/registration.hpp"

int main(int argc, char *argv[]) {
  auto component_list = userver::components::MinimalServerComponentList()
                            .Append<userver::server::handlers::Ping>()
                            .Append<userver::components::HttpClient>()
                            .Append<userver::components::Secdist>()
                            .Append<userver::components::Redis>("key-value-database")
                            .Append<userver::server::handlers::TestsControl>()
                            .Append<userver::components::TestsuiteSupport>();

  service_template::AppendHello(component_list);
  battleship::AppendRegistrator(component_list);

  return userver::utils::DaemonMain(argc, argv, component_list);
}
