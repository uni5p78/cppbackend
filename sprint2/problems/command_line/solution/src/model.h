#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <sstream>
#include <memory>
#include <iomanip>
#include <chrono>

#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept;
    Road(VerticalTag, Point start, Coord end_y) noexcept;
    bool IsHorizontal() const noexcept ;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept ;
private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept;
    const Rectangle& GetBounds() const noexcept ;
private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;
    Office(Id id, Point position, Offset offset) noexcept;
    const Id& GetId() const noexcept ;
    Point GetPosition() const noexcept ;
    Offset GetOffset() const noexcept ;
private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map;

class Dog {
    public:
        using Id = util::Tagged<int, Dog>;
        Dog(std::string user_name, Id id) noexcept;
        const Id& GetId() const noexcept ;
        const std::string& GetName() const noexcept ;
    
        using Dimension = double;
        using Coord = Dimension;
        struct Pos {
            Coord x{}, y{};
        };         
        struct Speed {
            // Dimension dir_x, dir_y;
            Dimension dir_x{}, dir_y{};
        };    
        enum class Dir: char {Left = 'L', Right = 'R', Up = 'U', Down = 'D', None = 0}; 
    
        void SetPos(Pos pos);
        void SetPos(bool IsHorizontal, Pos pos);
        Pos GetPos() const;
        Speed GetSpeed() const;
        void SetSpeed(Speed speed);
        void Stop();
        void SetSpeed(Dir dir, Dimension dog_speed);
        char GetDirSymbol() const;
        void SetDirSpeed(Dir dir, Dimension dog_speed);
        static char CheckDirSymbol(char dir);
        void CalcNewPosOnRoad(const Map& map, const std::chrono::milliseconds time_delta);      
    private:
        Id id_;
        std::string name_;
        Pos pos_;
        Speed speed_;
        Dir dir_ = Dir::Up;
    };
        
class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept;
    const Id& GetId() const noexcept ;
    const std::string& GetName() const noexcept ;
    const Buildings& GetBuildings() const noexcept ;
    const Roads& GetRoads() const noexcept ;
    const Offices& GetOffices() const noexcept ;
    void AddRoad(const Road&& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);
    Dog::Pos GetRandomPos() const;
    Dog::Pos GetStartPos() const;
    void SetDogSpeed(Dog::Dimension dog_speed);
    Dog::Dimension GetDogSpeed() const;
    void BildListOderedPath();
    Dimension GetEndOfPath(bool IsHorizontal, bool to_right, Dimension level_dog, Dimension point_dog) const ;
    Dimension GetEndOfPathV(Dimension level_dog, Dimension point_dog, bool to_right) const ;
private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    struct Path {Coord level, point1, point2;};
    // Coord level - это координа общая для обеих точек отрезка дороги 
    // для горизонтальных дорог это "y", для вертикальных - "x"
    // point1, point2 - это координаты точек на оставшейся оси, отличной от level
    class OrderedListPaths {
    public:
        void BildListOderedPath();
        Dimension GetEndOfPath(Dimension level_dog, Dimension point_dog, bool to_right) const ;
        void AddPath(Coord base, Coord point1, Coord point2);
    private:
        std::vector<Path> paths_;
    };
    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    Dog::Dimension dog_speed_;
    OrderedListPaths h_paths_;
    OrderedListPaths v_paths_;
    // void BildListOderedPath(OrderedListPaths paths);
};

class GameSession {
public:
    using Dogs = std::vector<Dog*>;
    GameSession(const Map* map, bool randomize_spawn_points) noexcept;
    void AddDog(Dog* dog);
    const Dogs& GetDogs() const;
    const Map& GetMap() const;
private:
    Dogs dogs_;
    const Map* map_;
    bool randomize_spawn_points_;
};

class Game {
public:
    using Maps = std::vector<Map>;
    Game(bool randomize_spawn_points);
    void AddMap(Map map);
    const Maps& GetMaps() const noexcept ;
    const Map* FindMap(const Map::Id& id) const noexcept ;
    // No copy functions.
    Game(const Game&) = delete;
    void operator=(const Game&) = delete;
    Dog* AddDog(std::string name);
    const Dog& GetDog(Dog::Id id) const ;
    GameSession* GetSession(const Map* map);
    void SetDefaultDogSpeed(Dog::Dimension dog_speed);
    Dog::Dimension GetDefaultDogSpeed() const;
    void ChangeGameSate(std::chrono::milliseconds time_delta);

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapToSessions = std::unordered_map<const Map*, std::vector<GameSession>>;
    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    std::vector<std::unique_ptr<Dog>> dogs_;
    MapToSessions sessions_;
    Dog::Dimension default_dog_speed_ = 1.0;
    bool randomize_spawn_points_;

    MapToSessions& GetSessions();
};

}  // namespace model
