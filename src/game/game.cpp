#include "game.hpp"

#include <userver/utest/using_namespace_userver.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/serialize/common_containers.hpp>

#include <iostream>
#include <sstream>
#include <string>

#include <field/field_stat.hpp>
#include <cors.hpp>

namespace battleship {

class GameHandler final : public userver::server::handlers::HttpHandlerBase {
public:

    static constexpr std::string_view kName = "handler-game";

    using HttpHandlerBase::HttpHandlerBase;

    GameHandler(const components::ComponentConfig& config,
              const components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    storages::redis::ClientPtr redis_client_;
    storages::redis::CommandControl redis_cc_;
};

GameHandler::GameHandler(const components::ComponentConfig& config,
                         const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
            .GetClient("main-kv")} { }

template <class Value>
Value Serialize(const Field& field, formats::serialize::To<Value>) {
    typename Value::Builder builder;
    std::array<std::array<size_t, kFieldSize>, kFieldSize> field_array;

    for (size_t x = 0; x < kFieldSize; ++x) {
        for (size_t y = 0; y < kFieldSize; ++y) {
            field_array[x][y] = static_cast<size_t>(field[x][y]);
        }
    }
    builder["field"] = field_array;

    return builder.ExtractValue();
}

std::string GameHandler::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                            userver::server::request::RequestContext&) const {
    SetCors(request);
    const auto& player_id = request.GetArg("player_id");
    const auto& x_str = request.GetArg("x");
    const auto& y_str = request.GetArg("y");
    
    if (player_id.empty() || x_str.empty() || y_str.empty()) {
        return "Wrong params";
    }
    redis_client_->Hset("time", player_id, std::to_string(std::time(nullptr)), redis_cc_);

    const auto my_field_str = redis_client_->Hget("game", player_id, redis_cc_).Get().value_or("");
    if (my_field_str.empty()) {
        return "your field is broken";
    }

    formats::json::Value my_field_json = formats::json::FromString(my_field_str);
    auto my_field = my_field_json["left_field"]["field"].As<Field>();
    if (FieldHelper::IsAllShipsDead(my_field)) {
        return "You lose";
    }

    const auto str_to_size_t = [](const std::string& str) {
        std::stringstream iss(str);
        size_t result = 0;
        iss >> result;
        return result;
    };

    const auto x = str_to_size_t(x_str);
    const auto y = str_to_size_t(y_str);
    if (x >= kFieldSize || y >= kFieldSize) {
        return "wrong coords";
    }

    const auto enemy_id = redis_client_->Hget("game_matcher", player_id, redis_cc_).Get().value_or("");
    if (enemy_id.empty()) {
        return "player_id is broken";
    }
    redis_client_->Hset("time", enemy_id, std::to_string(std::time(nullptr)), redis_cc_);

    const auto field_str = redis_client_->Hget("game", enemy_id, redis_cc_).Get().value_or("");
    if (field_str.empty()) {
        return "enemy field is broken";
    }

    formats::json::Value field_json = formats::json::FromString(field_str);
    auto field = field_json["left_field"]["field"].As<Field>();
    if (FieldHelper::IsAllShipsDead(field)) {
        return "You win";
    }
    const auto& is_player_turn = redis_client_->Hget("turn", player_id, redis_cc_).Get().value_or("0");
    if (is_player_turn != "1") {
        return "Not your turn";
    }
    redis_client_->Hset("turn", player_id, "0", redis_cc_);
    redis_client_->Hset("turn", enemy_id, "1", redis_cc_);
    if (field[x][y] == FieldPoint::Ship) {
        field[x][y] = FieldPoint::X_Ship;
        formats::json::ValueBuilder builder;
        builder["left_field"] = field;

        redis_client_->Hset("game", enemy_id, ToString(builder.ExtractValue()), redis_cc_);
        if (FieldHelper::IsKilled(field, x, y)) {
            return "Kill";
        }
        return "Damage";
    }
    return "Miss";
}

void AppendGame(userver::components::ComponentList& component_list) {
    component_list.Append<GameHandler>();
}

}
