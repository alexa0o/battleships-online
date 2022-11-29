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

#include "field_stat.hpp"

namespace battleship {

class FieldResultJsonBuilder {
public:
    FieldResultJsonBuilder(const FieldHelper& field_helper);
    std::string GetString() const;

private:
    const FieldHelper& field_helper_;
};

FieldResultJsonBuilder::FieldResultJsonBuilder(const FieldHelper& field_helper)
    : field_helper_(field_helper) { }

std::string FieldResultJsonBuilder::GetString() const {
    formats::json::ValueBuilder builder;
    builder["status"] = field_helper_.IsValid();
    const auto ships = field_helper_.CountShips();
    builder["ships"]["1"] = ships.ship_1;
    builder["ships"]["2"] = ships.ship_2;
    builder["ships"]["3"] = ships.ship_3;
    builder["ships"]["4"] = ships.ship_4;

    return ToString(builder.ExtractValue());
}

class FieldHandler : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-field";

    FieldHandler(const components::ComponentConfig& config,
          const components::ComponentContext& context);

    std::string HandleRequestThrow(const server::http::HttpRequest& request,
                                   server::request::RequestContext&) const override;

private:
    storages::redis::ClientPtr redis_client_;
    storages::redis::CommandControl redis_cc_;
};

FieldHandler::FieldHandler(const components::ComponentConfig& config,
             const components::ComponentContext& context) 
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetClient("main-kv")} { }

std::string FieldHandler::HandleRequestThrow(const server::http::HttpRequest& request,
                                      server::request::RequestContext&) const {
    const auto player_id = request.GetArg("player_id");
    if (player_id.empty()) {
        return "Wrong player_id";
    }
    const auto& body = request.RequestBody();
    formats::json::Value json = formats::json::FromString(body)["left_field"];
    FieldHelper field(json["field"].As<Field>());
    FieldResultJsonBuilder field_json(field);
    if (field.IsValid()) {
        redis_client_->Hset("game", player_id, body, redis_cc_);
    }
    return field_json.GetString();
}

Field Parse(const formats::json::Value& json,
            formats::parse::To<Field>) {
    Field field;
    for (size_t x = 0; x < json.GetSize(); ++x) {
        for (size_t y = 0; y < json[x].GetSize(); ++y) {
            field[x][y] = static_cast<FieldPoint>(json[x][y].As<size_t>());
        }
    }
    return field;
}

FieldHelper::FieldHelper(const Field& field)
    : field_(field),
      is_valid_(CountShipsAndCheckValid()) { }

bool FieldHelper::CountShipsAndCheckValid() {
    struct FieldTest {
        bool IsShip = false;
        bool IsHor = false;
        bool IsVer = false;
    };

    std::array<std::array<FieldTest, kFieldSize>, kFieldSize> test_field;
    for (size_t x = 0; x < kFieldSize; ++x) {
        bool is_ship = false;
        for (size_t y = 0; y < kFieldSize; ++y) {
            if (field_[x][y] == FieldPoint::Empty) {
                is_ship = false;
            } else if (field_[x][y] == FieldPoint::Ship) {
                test_field[x][y].IsShip = true;
                if (is_ship) {
                    if (test_field[x][y - 1].IsVer) {
                        return false;
                    }
                    test_field[x][y - 1].IsHor = true;
                    test_field[x][y].IsHor = true;
                    
                    if (x > 0) {
                        if (test_field[x - 1][y].IsShip) {
                            return false;
                        }
                    }
                }
                is_ship = true;
                if (x > 0) {
                    if (test_field[x - 1][y].IsShip) {
                        if (test_field[x - 1][y].IsHor) {
                            return false;
                        }
                        test_field[x - 1][y].IsVer = true;
                        test_field[x][y].IsVer = true;
                    }
                }
            } else {
                return false;
            }
        }
    }

    for (size_t x = 0; x < kFieldSize; ++x) {
        for (size_t y = 0; y < kFieldSize; ++y) {
            if (test_field[x][y].IsShip) {
                size_t ship_size = 0;
                if (test_field[x][y].IsVer) {
                    size_t current_x = x;
                    while (test_field[current_x][y].IsShip) {
                        ++ship_size;
                        test_field[current_x][y].IsShip = false;
                        if (current_x + 1< kFieldSize) {
                            ++current_x;
                        }
                    }
                } else if (test_field[x][y].IsHor) {
                    size_t current_y = y;
                    while (test_field[x][current_y].IsShip) {
                        ++ship_size;
                        test_field[x][current_y].IsShip = false;
                        if (current_y + 1 < kFieldSize) {
                            ++current_y;
                        }
                    }
                } else {
                    ship_size = 1;
                }
                if (ship_size == 1) {
                    ++ships_.ship_1;
                } else if (ship_size == 2) {
                    ++ships_.ship_2;
                } else if (ship_size == 3) {
                    ++ships_.ship_3;
                } else if (ship_size == 4) {
                    ++ships_.ship_4;
                } else {
                    return false;
                }
            }
        }
    }

    const bool is_ships_count_ok = ships_.ship_1 == 4 && ships_.ship_2 == 3 && ships_.ship_3 == 2 && ships_.ship_4 == 1;
    return is_ships_count_ok;
}

bool FieldHelper::IsValid() const {
    return is_valid_;
}

FieldShips FieldHelper::CountShips() const {
    return ships_;
}

bool FieldHelper::IsAllShipsDead(const Field& field) {
    for (const auto& line : field) {
        for (const auto point : line) {
            if (point == FieldPoint::Ship) {
                return false;
            }
        }
    }
    return true;
}

bool FieldHelper::IsKilled(const Field& field, size_t x, size_t y) {
    size_t current_x = x;
    size_t current_y = y;

    while (current_x > 0) {
        const auto point = field[current_x - 1][y];
        if (point == FieldPoint::Ship) {
            return false;
        } else if (point == FieldPoint::X_Ship) {
            --current_x;
        } else {
            break;
        }
    }

    current_x = x;

    while (current_x < kFieldSize - 1) {
        const auto point = field[current_x + 1][y];
        if (point == FieldPoint::Ship) {
            return false;
        } else if (point == FieldPoint::X_Ship) {
            ++current_x;
        } else {
            break;
        }
    }

    while (current_y > 0) {
        const auto point = field[x][current_y - 1];
        if (point == FieldPoint::Ship) {
            return false;
        } else if (point == FieldPoint::X_Ship) {
            --current_y;
        } else {
            break;
        }
    }

    current_y = y;
    
    while (current_y < kFieldSize - 1) {
        const auto point = field[x][current_y + 1];
        if (point == FieldPoint::Ship) {
            return false;
        } else if (point == FieldPoint::X_Ship) {
            ++current_y;
        } else {
            break;
        }
    }

    return true;
}

void AppendField(userver::components::ComponentList& component_list) {
    component_list.Append<FieldHandler>();
}

}
