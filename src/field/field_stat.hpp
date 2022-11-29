#pragma once

namespace battleship {

enum class FieldPoint: size_t {
    Empty,
    Ship,
    X_Ship
};

static constexpr size_t kFieldSize = 10;
using Field = std::array<std::array<FieldPoint, kFieldSize>, kFieldSize>;

Field Parse(const formats::json::Value& json,
            formats::parse::To<Field>);

struct FieldShips {
    size_t ship_1 = 0;
    size_t ship_2 = 0;
    size_t ship_3 = 0;
    size_t ship_4 = 0;
};

class FieldHelper {
public:
    FieldHelper(const Field& field);
    bool IsValid() const;
    FieldShips CountShips() const;

    static bool IsAllShipsDead(const Field& field);
    static bool IsKilled(const Field& field, size_t x, size_t y);

private:
    bool CountShipsAndCheckValid();

private:
    Field field_;
    FieldShips ships_;
    bool is_valid_;
};

}
