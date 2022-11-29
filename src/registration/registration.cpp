#include "registration.hpp"

#include <userver/utest/using_namespace_userver.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/async.hpp>
#include <userver/engine/sleep.hpp>

#include <atomic>

#include <cors.hpp>

namespace battleship {

static std::atomic_size_t kLastRegId = 0;
static const std::string kRegQueue = "reg-queue";

class GameMatcher {
public:
    GameMatcher(storages::redis::ClientPtr redis_client,
                engine::TaskProcessor& task_processor);
    
    ~GameMatcher();
private:
    static void MatchLoop(storages::redis::ClientPtr redis_client, std::atomic_bool* is_stop);
    static void CleanLoop(storages::redis::ClientPtr redis_client, std::atomic_bool* is_stop);
private:
    engine::TaskWithResult<void> match_loop_;
    engine::TaskWithResult<void> clean_loop_;
    std::atomic_bool is_stop_{false};
};

GameMatcher::GameMatcher(storages::redis::ClientPtr redis_client,
                         engine::TaskProcessor& task_processor) {
    match_loop_ = utils::CriticalAsync(task_processor,
                                       "matcher_loop",
                                       this->MatchLoop,
                                       redis_client,
                                       &is_stop_);
    clean_loop_ = utils::CriticalAsync(task_processor,
                                       "matcher_loop",
                                       this->CleanLoop,
                                       redis_client,
                                       &is_stop_);
}

GameMatcher::~GameMatcher() {
    is_stop_ = true;
}

void GameMatcher::MatchLoop(storages::redis::ClientPtr redis_client,
                            std::atomic_bool* is_stop) {
    storages::redis::CommandControl redis_cc;
    while (!is_stop->load()) {
        const auto reg_id = redis_client->Lpop(kRegQueue, redis_cc).Get();
        if (reg_id.has_value()) {
            auto reg_id_2 = redis_client->Lpop(kRegQueue, redis_cc).Get();
            while (!reg_id_2.has_value()) {
                engine::SleepFor(std::chrono::seconds(3));
                reg_id_2 = redis_client->Lpop(kRegQueue, redis_cc).Get();
            }

            redis_client->Hset("turn", reg_id.value(), "0", redis_cc);
            redis_client->Hset("turn", reg_id_2.value(), "1", redis_cc);
            redis_client->Hset("game_matcher", reg_id.value(), reg_id_2.value(), redis_cc);
            redis_client->Hset("game_matcher", reg_id_2.value(), reg_id.value(), redis_cc);
        }
        engine::SleepFor(std::chrono::seconds(3));
    }
}

static constexpr size_t kHourSeconds = 3600;

void GameMatcher::CleanLoop(storages::redis::ClientPtr redis_client,
                            std::atomic_bool* is_stop) {
    storages::redis::CommandControl redis_cc;
    while (!is_stop->load()) {
        engine::SleepFor(std::chrono::hours(1));
        const auto& ids = redis_client->Hkeys("time", redis_cc).Get();
        const auto now = std::time(nullptr);
        std::vector<std::string> ids_to_remove;
        for (const auto id : ids) {
            const auto& last_acess_time = redis_client->Hget("time", id, redis_cc).Get().value_or("0");
            if ((now - std::stol(last_acess_time)) > kHourSeconds) {
                ids_to_remove.push_back(id);
            }
            redis_client->Hdel("time", ids_to_remove, redis_cc);
            redis_client->Hdel("turn", ids_to_remove, redis_cc);
            redis_client->Hdel("game", ids_to_remove, redis_cc);
            redis_client->Hdel("game_matcher", ids_to_remove, redis_cc);
        }
    }
}

class Registrator final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-registration";

    using HttpHandlerBase::HttpHandlerBase;

    Registrator(const components::ComponentConfig& config,
                const components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    storages::redis::ClientPtr redis_client_;
    storages::redis::CommandControl redis_cc_;
    GameMatcher game_matcher_;
};

Registrator::Registrator(const components::ComponentConfig& config,
                         const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
            .GetClient("main-kv")},
      game_matcher_(redis_client_, context.GetTaskProcessor("main-task-processor")) { }

std::string Registrator::HandleRequestThrow(const server::http::HttpRequest& request,
                                            server::request::RequestContext& /*context*/) const {
    SetCors(request);
    const auto reg_id = std::to_string(kLastRegId++);
    redis_client_->Rpush(kRegQueue, reg_id, redis_cc_);
    redis_client_->Hset("time", reg_id, std::to_string(std::time(nullptr)), redis_cc_);

    return reg_id;
}

class RegStatus final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-regstatus";

    using HttpHandlerBase::HttpHandlerBase;

    RegStatus(const components::ComponentConfig& config,
              const components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    storages::redis::ClientPtr redis_client_;
    storages::redis::CommandControl redis_cc_;
};

RegStatus::RegStatus(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
            .GetClient("main-kv")} { }

std::string RegStatus::HandleRequestThrow(const server::http::HttpRequest& request,
                                          server::request::RequestContext& /*context*/) const {
    SetCors(request);
    const auto& reg_id = request.GetArg("reg_id");
    if (reg_id.empty()) {
        return "Can't find reg_id arg";
    }
    const auto& player_id = redis_client_->Hget("game_matcher", reg_id, redis_cc_).Get();
    redis_client_->Hset("time", reg_id, std::to_string(std::time(nullptr)), redis_cc_);

    return player_id.value_or("Wait");
}

void AppendRegistrator(userver::components::ComponentList& component_list) {
    component_list.Append<Registrator>()
                  .Append<RegStatus>();
}

}
