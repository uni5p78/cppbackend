#include "model.h"

#include <stdexcept>
#include <algorithm>
#include <cmath>


namespace model {
using namespace std::literals;

Road::Road(HorizontalTag, Point start, Coord end_x) noexcept
: start_{start}
, end_{end_x, start.y} {
}

Road::Road(VerticalTag, Point start, Coord end_y) noexcept
: start_{start}
, end_{start.x, end_y} {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

Building::Building(Rectangle bounds) noexcept
: bounds_{bounds} {
}

const Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

Office::Office(Id id, Point position, Offset offset) noexcept
: id_{std::move(id)}
, position_{position}
, offset_{offset} {
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

Point Office::GetPosition() const noexcept {
    return position_;
}

Offset Office::GetOffset() const noexcept {
    return offset_;
}


Map::Map(Id id, std::string name) noexcept
: id_(std::move(id))
, name_(std::move(name)) {
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}

void Map::AddRoad(const Road&& road) {
    bool vertical = road.IsVertical();
    auto start = road.GetStart();
    auto end = road.GetEnd();
    if(vertical){
        if(start.y > end.y){
            std::swap(start,end); 
        }
        v_paths_.AddPath(start.x, start.y, end.y);
    } else {
        if(start.x > end.x){
            std::swap(start,end); 
        }
        h_paths_.AddPath(start.y, start.x, end.x);
        // h_roads_.push_back(&roads_.back());
    }
    roads_.emplace_back(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}


void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

Dog::Pos Map::GetRandomPos() const {
    auto roads = GetRoads();
    if(roads.empty()){
        return {0.0, 0.0};
    }
    std::random_device random_device_;
    std::uniform_int_distribution<int> dist_roads(0, roads.size()-1);
    int random_road = dist_roads(random_device_);
    auto road = roads[random_road];
    auto start = road.GetStart();
    auto end = road.GetEnd();
    if(road.IsHorizontal()){
        std::uniform_real_distribution<Dog::Coord> dist_x(start.x, end.x);
        return {dist_x(random_device_), static_cast<Dog::Coord>(start.y)};
    } else {
        std::uniform_real_distribution<Dog::Coord> dist_y(start.y, end.y);
        return {static_cast<Dog::Coord>(start.x), dist_y(random_device_)};
    };
}

Dog::Pos Map::GetStartPos() const {
    auto roads = GetRoads();
    if(roads.empty()){
        return {0.0, 0.0};
    }
    auto road = roads[0];
    auto start = road.GetStart();
    return {static_cast<Dog::Coord>(start.x), static_cast<Dog::Coord>(start.y)};
}

void Map::SetDogSpeed(Dog::Dimension dog_speed){
    dog_speed_ = dog_speed;
}

Dog::Dimension Map::GetDogSpeed() const {
    return dog_speed_;
}

void Map::OrderedListPaths::BildListOderedPath(){ 
    auto compare_h = [](const Path left, const Path right){
        return left.level < right.level;
    };
    std::sort(paths_.begin(), paths_.end(), compare_h);
    auto it1 = paths_.begin();
    auto it2 = it1+1;
    while (it2 != paths_.end()) {
        // Если второя дорога продолжает первую
        if ((*it1).level==(*it2).level && (*it1).point2>=(*it2).point1) {
            (*it1).point2 = (*it2).point2; //то первую продолжаем на длину второй
            paths_.erase(it2); // вторую удаляем
            it2 = it1+1;
        } else { // если нет - переходим  к сравнению следующей пары
            it1 += 1;
            it2 += 1;
        }
    }
}

void Map::BildListOderedPath(){ 
    h_paths_.BildListOderedPath();
    v_paths_.BildListOderedPath();
}

Dimension Map::GetEndOfPath(bool IsHorizontal, bool to_right, Dimension level_dog, Dimension point_dog) const {
    if (IsHorizontal) {
        return h_paths_.GetEndOfPath(level_dog, point_dog, to_right);
    } 
    return v_paths_.GetEndOfPath(level_dog, point_dog, to_right);
}

Dimension Map::GetEndOfPathV(Dimension level_dog, Dimension point_dog, bool to_up) const {
    return v_paths_.GetEndOfPath(level_dog, point_dog, to_up);
}


Dimension Map::OrderedListPaths::GetEndOfPath(Dimension level_dog, Dimension point_dog, bool to_upward_direct) const{
    auto compare_coord = [](Path road, Dimension coord_find){ return road.level < coord_find; };
    auto it = std::lower_bound(paths_.begin(), paths_.end(), level_dog, compare_coord);
    Dimension res = point_dog; // если не найдем дорогу в нужном направлении, то передадим точку собаки

    while (it!=paths_.end() && (*it).level==level_dog) { //Если нашли дорогу с нужным уровнем
        auto path = *it;
        if (point_dog>=path.point1 && point_dog<=path.point2) { // если собака на этой дороге
            res =  to_upward_direct ? path.point2 : path.point1; // передаем предел в нужном направлении
            break;
        }
        it+=1;  // переходим на следующую (список отсортирован по возрастаню уровня
                // дороги стоящие "паровозиком" объеденены в один путь)
    }
    return res;
}

// Dimension Map::OrderedListPaths::GetEndOfPath(Dimension level_dog, Dimension point_dog, bool to_upward_direct) const{
//     auto compare_coord = [](Dimension coord_find, Path road){ return coord_find < road.level; };
//     auto it = std::lower_bound(paths_.begin(), paths_.end(), level_dog, compare_coord);
//     Dimension res = point_dog;

//     while (it!=paths_.end() && (*it).level==level_dog) { //Если нашли дорогу с нужным уровнем
//         auto path = *it;
//         if (point_dog<path.point1 && point_dog>path.point2) { // если собака не на этой дороге
//             it+=1;  // переходим на следующую
//             continue; 
//         }
//         // return to_upward_direct ? path.point2 : path.point1;
//         // res = to_right ? std::max(res, road.x2) : std::min(res, road.x1);
//         res =  to_upward_direct ? path.point2 : path.point1;
//         break;
//     }
//     return res;
// }

void Map::OrderedListPaths::AddPath(Coord coord, Coord mark1, Coord mark2){
    paths_.emplace_back(coord, mark1, mark2);
}


Dog::Dog(std::string user_name, Id id) noexcept
: name_{std::move(user_name)} 
    , id_{std::move(id)} {
}

const Dog::Id& Dog::GetId() const noexcept {
    return id_;
}

const std::string& Dog::GetName() const noexcept {
    return name_;
}

void Dog::SetPos(Pos pos){
    pos_ = pos;
}  

void Dog::SetPos(bool IsHorizontal, Pos pos){
    if (IsHorizontal) {
        pos_ = pos;
    } else {
        pos_ = {pos.y, pos.x};
    }
}  

Dog::Pos Dog::GetPos() const {
    return pos_;
}  

Dog::Speed Dog::GetSpeed() const {
    return speed_;
}  

void Dog::SetSpeed(Speed speed){
    speed_ = speed;
}

void Dog::Stop(){
    speed_ = {};
}

void Dog::SetSpeed(Dir dir, Dimension dog_speed){
    dog_speed *= dir==Dir::Left || dir==Dir::Down ? -1.0 : 1.0;
    speed_ = dir==Dir::Left || dir==Dir::Right ? Speed{dog_speed, 0.0} : Speed{0.0, dog_speed};
}




char Dog::GetDirSymbol() const {
    switch (dir_)
    {
    case Dir::Left:
                    return 'L';
    case Dir::Right:
                    return 'R';
    case Dir::Up:
                    return 'U';
    case Dir::Down:
                    return 'D';
    default:
        throw std::out_of_range("Unknown direction of movement"); 
    } 
}  

void Dog::SetDirSpeed(Dir dir, Dimension dog_speed){
    dir_ = dir;
    dog_speed *= dir==Dir::Left || dir==Dir::Up ? -1.0 : 1.0;
    speed_ = dir==Dir::Left || dir==Dir::Right ? Speed{dog_speed, 0.0} : Speed{0.0, dog_speed};
    // if(dir == dir_){
    //     return;
    // }

    // if(dir==Dir::None){
    //     SetSpeed(Speed{0.0f, 0.0f});
    // } else {
    //     dir_ = dir;
    //     SetSpeed(dir, dog_speed);
    // }
}

char Dog::CheckDirSymbol(char dir){
    if(dir!=static_cast<char>(Dir::Down) 
    && dir!=static_cast<char>(Dir::Left)
    && dir!=static_cast<char>(Dir::Right)
    && dir!=static_cast<char>(Dir::Up)){
        throw std::invalid_argument("Invalid syntax dog direct.");
    }
    return dir;
}

void Dog::CalcNewPosOnRoad(const Map& map, const int time_delta){
    auto speed_dir = GetSpeed();  
    if (speed_dir.dir_x==0.0 && speed_dir.dir_y==0.0 ) { // Если собака стоит
        return; // то мы ее не трогаем)
    };
    const Dog::Coord HalfWideRoad = 0.4;
    auto pos_1 = GetPos();
    auto speed = speed_dir.dir_x;
    bool IsHorizontal = speed != 0.e0;
    Dog::Coord level, point1, point2;
    if (IsHorizontal) { // собака движется горизонтально
        level = pos_1.y;
        point1 = pos_1.x;
    } else {  // вертикально)
        speed = speed_dir.dir_y;
        level = pos_1.x;
        point1 = pos_1.y;
    }
    point2 = point1 + speed * time_delta / 1000.0;  

    int level_int = round(level); 
    int point1_int = round(point1);               
    int point2_int = round(point2); 
    bool to_right = speed>0;
    // Проверка наличия препятствий на пути собаки
    if ((point1_int != point2_int) || (std::abs(point2-point2_int)>=HalfWideRoad)){
        // Если собака вышла из начаьного кадрата дороги
        // Проверим - есть дальше дорога?)
        auto point_end = map.GetEndOfPath(IsHorizontal, to_right, level_int, point1_int);

        if ((to_right && point2 >= HalfWideRoad + point_end) //если прошли край дороги
        ||   (!to_right && point2 <= -HalfWideRoad + point_end)) {
            bool minus = to_right ? 1.0 : -1.0;
            point2 = HalfWideRoad*minus+point_end; 
            Stop();  // Останавливаем собаку на краю дороги
        }
    }    
    SetPos(IsHorizontal, {point2, level});
}

// if (!((point1_int == point2_int) && (abs(point2-point2_int)<HalfWideRoad))){
//     // Проверим - есть дальше дорога?)
//     auto point_end = map.GetEndOfPath(IsHorizontal, to_right, level_int, point1_int);

//     if (!((to_right && point2 < HalfWideRoad + point_end) //если прошли край дороги
//     ||   (!to_right && point2 > -HalfWideRoad + point_end))) {
//         point2 = HalfWideRoad*znak()+point_end; 
//         Stop();  // Останавливаем собаку на краю дороги
//     }
//     bool d = (point1_int != point2_int) || (abs(point2-point2_int)>=HalfWideRoad);
//     bool f = (!to_right || point2 >= HalfWideRoad + point_end) //если прошли край дороги
//     &&   (to_right || point2 <= -HalfWideRoad + point_end);

//     bool f = (to_right && point2 >= HalfWideRoad + point_end) //если прошли край дороги
//     ||   (!to_right && point2 <= -HalfWideRoad + point_end);

// }    


// if ((to_right && point2_int>point_end) //если прошли последнюю точку дороги
// || (!to_right && point2_int<point_end)) {  
//     point2 = HalfWideRoad*znak()+point_end; 
//     Stop();  // Останавливаем собаку на краю в конце дороги
// } else if (point2_int=point_end) { // если  дошли до последней точки дороги
//     // нужно проверить вышла собака за границы дороги или нет
//     if ((to_right && (point2 - point_end > HalfWideRoad))
//     || (!to_right && (point_end - point2 > HalfWideRoad))) {
//         point2 = HalfWideRoad*znak()+point_end;
//         Stop(); // Останавливаем собаку на краю в конце дороги если прошли край
//     }
// } 


// void Dog::CalcNewPosOnRoad(const Map& map, const int time_delta){
//     const Dog::Coord HalfWideRoad = 0.4;
//     auto speed = GetSpeed();
//     auto pos_1 = GetPos();
//     if (speed.dir_x==0.0 && speed.dir_y==0.0 ) {
//         return;
//     };
//     const auto x1 = pos_1.x;
//     const auto y1 = pos_1.y;
//     bool HorizontalDirect = speed.dir_x != 0.0;
//     if (HorizontalDirect) { // собака движется горизонтально
//         auto x2 = x1 + speed.dir_x * time_delta / 1000.0;   
//         int x1_int = round(x1);               
//         int x2_int = round(x2); 
//         int y1_int = round(y1); 
//         if (x1_int == x2_int && abs(x2-x2_int)<HalfWideRoad){
//             SetPos({x2, y1}); // Если собака не дошла до возможной границы дороги
//         } else {
//             // Проверим - есть дальше дорога?)
//             bool to_right = speed.dir_x>0;
//             auto x_end = map.GetEndOfHorozontalPath(y1_int, x1_int, to_right);
//             if (to_right) {
//                 if ((to_right && x2_int<x_end) 
//                 || (!to_right && x2_int>x_end)) {  // если не дошли до последней точки дороги
//                     SetPos({x2, y1}); 
//                 } else if (x2_int=x_end) { // если  дошли до последней точки дороги
//                     // нужно проверить вышла собака за границы дороги или нет
//                     if (to_right && x2-x2_int>HalfWideRoad) {
//                         SetPos({HalfWideRoad+x2_int, y1});
//                         Stop();
//                     } else if (!to_right && x2-x2_int>-HalfWideRoad){
//                         SetPos({-HalfWideRoad+x2_int, y1});
//                         Stop();
//                     } else {
//                         SetPos({x2, y1}); // Если собака не дошла до возможной границы дороги
//                     }
//                 } else if (x2_int>x_end) { //если прошли последнюю точку дороги
//                     SetPos({HalfWideRoad+x_end, y1});
//                     Stop();  
//                 }  
//             } else {
//                 if (x2_int<x_end) {  // если не дошли до последней точки дороги
//                     SetPos({x2, y1}); 
//                 } else if (x2_int=x_end) { // если  дошли до последней точки дороги
//                     // нужно проверить вышла собака за границы дороги или нет
//                     if (abs(x2-x2_int)<HalfWideRoad) {
//                         SetPos({x2, y1}); // Если собака не дошла до возможной границы дороги
//                     } else {
//                         SetPos({HalfWideRoad+x2_int, y1});
//                         Stop();      
//                     }
//                 } else if (x2_int>x_end) { //если прошли последнюю точку дороги
//                     SetPos({HalfWideRoad+x_end, y1});
//                     Stop();  
//                 }  
//             }

//         }              
//     }
// }

GameSession::GameSession(const Map* map) noexcept
: map_(map) {
}

void GameSession::AddDog(Dog* dog){
    dogs_.push_back(dog);
    // dog->SetPos(map_->GetRandomPos());
    dog->SetPos(map_->GetStartPos());
    // dog->SetSpeed({0.0, 0.0});
    dog->SetDirSpeed(Dog::Dir::Up, 0.0);

}

const GameSession::Dogs& GameSession::GetDogs() const {
    return dogs_;
}

const Map& GameSession::GetMap() const{
    return *map_;
}


void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

GameSession* Game::GetSession(const Map* map){
    auto sessions_map = &sessions_[map];
    if(sessions_map->empty()){ // если пока нет сессий для этой карты
        sessions_map->emplace_back(map);
    }
    return &(*sessions_map)[0]; // возвращаем "нулевую" сессию для карты, пока она одна 
}

void Game::SetDefaultDogSpeed(Dog::Dimension dog_speed) {
    default_dog_speed_ = dog_speed;
}

Dog::Dimension Game::GetDefaultDogSpeed() const {
    return default_dog_speed_;
}

void Game::ChangeGameSate(int time_delta){
    // у всех собак изменить координаты и скорость в соответствии с движением во времени
    // поменять время игры на time_delta
    for (auto & [ map, sessions ] : sessions_) { // цикл по списку наборов сессий для конкретных карт
        for (auto & session : sessions) {  // цикл по списку сессий для карты
            auto& dogs = session.GetDogs();
            for (auto& dog : dogs) {  // цикл по собакам сессии
                dog->CalcNewPosOnRoad(*map, time_delta);
            }
        }
    }
}

// void Game::ChangeGameSate(int time_delta){
//     // у всех собак изменить координаты и скорость в соответствии с движением во времени
//     // поменять время игры на time_delta
//     const Dog::Coord HalfWideRoad = 0.4;
//     for (auto & [ map, sessions ] : sessions_) { // цикл по списку наборов сессий для конкретных карт
//         for (auto & session : sessions) {  // цикл по списку сессий для карты
//             auto& dogs = session.GetDogs();
//             for (auto& dog : dogs) {  // цикл по собакам сессии
//                 auto pos_1 = dog->GetPos();
//                 const auto x1 = pos_1.x;
//                 const auto y1 = pos_1.y;
//                 auto speed = dog->GetSpeed();
//                 if (speed.dir_x != 0.0) { // собака движется горизонтально
//                     auto x2 = pos_1.x + speed.dir_x * time_delta / 1000.0;   
//                     int x1_int = round(x1);               
//                     int x2_int = round(x2); 
//                     int y1_int = round(y1); 
//                     if (x1_int == x2_int && abs(x2-x2_int)<HalfWideRoad){
//                         dog->SetPos({x2, y1}); // Если собака не дошла до возможной границы дороги
//                         // continue; // переходим к следующей собаке по списку в сессии
//                     } else {
//                         // Проверим - есть дальше дорога?)
//                         bool to_right = speed.dir_x>0;
//                         auto x_end = map->GetEndOfHorozontalPath(y1_int, x1_int, to_right);
//                         if (to_right) {
//                             if (x2_int<x_end) {  // если не дошли до последней точки дороги
//                                 dog->SetPos({x2, y1}); 
//                                 // continue; 
//                             } else if (x2_int=x_end) { // если  дошли до последней точки дороги
//                                 // нужно проверить вышла собака за границы дороги или нет
//                                 if (abs(x2-x2_int)<HalfWideRoad) {
//                                     dog->SetPos({x2, y1}); // Если собака не дошла до возможной границы дороги
//                                     // continue; // переходим к следующей собаке по списку в сессии
//                                 } else {
//                                     dog->SetPos({HalfWideRoad+x2_int, y1});
//                                     dog->Stop();      
//                                 }
//                             } else if (x2_int>x_end) {
//                                 dog->SetPos({HalfWideRoad+x_end, y1});
//                                 dog->Stop();  
//                             }
//                             // x2 = x2_int<=x_end ? x2_int-HalfWideRoad : x2_int+HalfWideRoad;
//                             // x2 = x2<x2_int ? x2_int-HalfWideRoad : x2_int+HalfWideRoad;
//                             // dog->SetPos({x2, y1});
//                             // dog->Stop();  
//                         }

//                     }              
//                 }
//                 auto y1 = pos_1.y + speed.dir_y * time_delta / 1000.0;

//             }
//         }
//     }
// }

Game::MapToSessions& Game::GetSessions(){
    return sessions_;
}



const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

Dog* Game::AddDog(std::string name){
    Dog::Id id(dogs_.size()+1);
    dogs_.push_back(std::make_unique<Dog>(name, id));
    return dogs_.back().get();
}


const Dog& Game::GetDog(Dog::Id id) const {
    if(*id < dogs_.size()){
        return *dogs_[*id];
    }
    throw;
}

}  // namespace model
