// Этот файл служит для подключения реализации библиотеки Boost.Json
#include "boost_json.h"

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std::literals;

namespace boost_json {

std::string GetErrorMes(std::string_view code, std::string_view message){
    boost::json::object obj{
          {ErrorMesFields::CODE, code}
        , {ErrorMesFields::MESSAGE, message}
    };
    return serialize(obj);   
}

JoinRequest ParseJoinRequest(const std::string& object){
    try {
        boost::json::value json_object = boost::json::parse(object);
        JoinRequest res;
        res.user_name = json_object.as_object().at(JoinRequestFields::USER_NAME).get_string().c_str();
        res.map_id = json_object.as_object().at(JoinRequestFields::MAP_ID).get_string().c_str();
        return res;
    }
    catch (std::exception& ex) {
        throw std::runtime_error("ParseJoinRequest(): Error parse string in json object -> "s + ex.what());
    }
}    

std::string GetMapsJson(const app::list_maps::Result& maps){
    boost::json::array arr;
    for(const auto&map : maps){
        boost::json::object obj;
        obj[MapFields::ID] = map.id;
        obj[MapFields::NAME] = map.name;
        arr.emplace_back(std::move(obj));
    }
    return serialize(arr);
}

boost::json::array GetRoadsArr(const app::map_info::Roads & roads){
    boost::json::array res; 
    for(const auto& road : roads){
        boost::json::object obj;
        obj[RoadCoord::START_X] = road.start.x;
        obj[RoadCoord::START_Y] = road.start.y;
        if (road.start.y == road.end.y){
            obj[RoadCoord::END_X] = road.end.x;
        } else {
            obj[RoadCoord::END_Y] = road.end.y;
        }
        res.emplace_back(std::move(obj));
    }
    return res;
}

boost::json::array GetBuildingsArr(const app::map_info::Buildings & buildings){ 
    boost::json::array res; 
    for(const auto& building : buildings){
        boost::json::object obj;
        auto bounds = building.bounds;
        obj[BuildingFields::POSITION_X] = bounds.position.x;
        obj[BuildingFields::POSITION_Y] = bounds.position.y;
        obj[BuildingFields::WIDTH] = bounds.size.width;
        obj[BuildingFields::HEIGHT] = bounds.size.height;
        res.emplace_back(std::move(obj));
    }
    return res;
}

boost::json::array GetOfficesArr(const app::map_info::Offices & offices){ 
    boost::json::array res; 
    for(const auto& office : offices){
        boost::json::object obj;
        auto position = office.position;
        auto offset = office.offset;
        obj[OfficeFields::ID] = office.id;
        obj[OfficeFields::POSITION_X] = position.x;
        obj[OfficeFields::POSITION_Y] = position.y;
        obj[OfficeFields::OFFSET_X] = offset.dx;
        obj[OfficeFields::OFFSET_Y] = offset.dy;
        res.emplace_back(std::move(obj));
    }
    return res;
}

std::string GetMapJson(const app::map_info::Result& map){
    boost::json::object obj;
    obj[MapFields::ID] = map.id_map;
    obj[MapFields::NAME] = map.name_map;
    obj[MapFields::ROADS] = GetRoadsArr(map.roads_);
    obj[MapFields::BILDINGS] = GetBuildingsArr(map.buildings_);
    obj[MapFields::OFFICES] = GetOfficesArr(map.offices_);

    return serialize(obj);
}

std::string GetPlayerJsonBody(const app::join_game::Result& player_data){
    boost::json::object obj;
    obj[PlayerFields::TOKEN] = *player_data.token;
    obj[PlayerFields::PAYER_ID] = *player_data.player_id;
    return serialize(obj);
}

std::string GetGameSateJsonBody(const app::game_state::Result& dogs){
    boost::json::object res; 
    boost::json::object players;

    for(const auto& dog : dogs){
        boost::json::object player;
        player[GameSateFields::POS] = {dog.pos.x, dog.pos.y};
        player[GameSateFields::SPEED] = {dog.speed.dir_x, dog.speed.dir_y};
        player[GameSateFields::DIR] = dog.dir ;
        players[dog.id] = player;
    }
    res[GameSateFields::PLAYERS] = players;
    return serialize(res);
}

std::string GetPlayersJsonBody(const app::players_list::Result& dogs){
    boost::json::object obj;
    for(const auto& dog : dogs){
        boost::json::object player_obj;
        player_obj[DogFields::NAME] = dog.name;
        obj[dog.id] = player_obj;
    }
    return serialize(obj);
}

std::string SerializeEmptyJsonObject(){
    boost::json::object obj;
    return serialize(obj);
}

JsonValue::JsonValue(const boost::json::value& value)
: value_(value){
}

const std::vector<JsonValue> JsonValue::GetParamAsArray(const std::string& name_array) const {
    auto& json_array = value_.as_object().at(name_array).as_array();
    return {json_array.begin(), json_array.end()};
}

std::string JsonValue::GetParamAsString(const std::string& name_param) const {
    return value_.at(name_param).get_string().c_str();
}

int JsonValue::GetParamAsInt(const std::string& name_param) const {
    return static_cast<int>(value_.as_object().at(name_param).as_int64());
}

double JsonValue::GetParamAsDouble(const std::string& name_param) const {
    return static_cast<double>(value_.as_object().at(name_param).as_double());
}

bool JsonValue::ContainsParam(const std::string& name_param) const{
    return value_.as_object().contains(name_param);
}

void JsonValue::SetParentPtr(std::shared_ptr<boost::json::value> ptr){
    parent_ptr_ = ptr;
}



JsonValue ParseFile(const std::filesystem::path& json_path){
    std::ifstream in_file(json_path);
    if (!in_file){
        throw std::runtime_error("Can't open config file: "s + json_path.c_str());
    };
    std::ostringstream sstr;
    sstr << in_file.rdbuf();
    return ParseStr(sstr.str());
}

JsonValue ParseStr(const std::string& str_json){
    try {
        //Создаем в куче дерево Json, чтобы потом не него могли ссылаться объекты JsonValue
        auto val_shptr = std::make_shared<boost::json::value>(boost::json::parse(str_json)); 
        JsonValue res{*val_shptr};
        // Сохраняем shared ссылку на дерево Json в первом объекте JsonValue, чтобы продлить 
        // срок жизни дерева до завершения обработки информации
        res.SetParentPtr(val_shptr);
        return res;
    }
    catch (std::exception& ex) {
        throw std::runtime_error("ParseStr(): Error parse string in json object -> "s + ex.what());
    }
}



} // namespace boost_json 

