#include "json_loader.h"
#include "tagged.h"
#include "boost_json.h"


#include <filesystem>
#include <fstream>

using namespace std;
using namespace std::literals;

namespace json_loader {

void AddRoads(model::Map& map, const boost_json::ArrayJsonValue& roads){
    for(const auto& road : roads){
        bool horizontal_road = road.ContainsParam(RoadCoord::END_X);
        if (horizontal_road){
            model::Road road_model(model::Road::HORIZONTAL
            , {road.GetParamAsInt(RoadCoord::START_X), road.GetParamAsInt(RoadCoord::START_Y)}
            , road.GetParamAsInt(RoadCoord::END_X));
            map.AddRoad(move(road_model));
        } else {
            model::Road road_model(model::Road::VERTICAL
            , {road.GetParamAsInt(RoadCoord::START_X), road.GetParamAsInt(RoadCoord::START_Y)}
            , road.GetParamAsInt(RoadCoord::END_Y));
            map.AddRoad(move(road_model));
        }
    }
    map.BildListOderedPath();
}

void AddBuildings(model::Map& map, const boost_json::ArrayJsonValue& buildings){
    for(const auto& building : buildings){
        model::Building building_model(
            { {building.GetParamAsInt(BuildingFields::POSITION_X), building.GetParamAsInt(BuildingFields::POSITION_Y)}
            , {building.GetParamAsInt(BuildingFields::WIDTH), building.GetParamAsInt(BuildingFields::HEIGHT)}}
        );
        map.AddBuilding(move(building_model));
    }
}

void AddOffices(model::Map& map, const boost_json::ArrayJsonValue& offices){
    using IdOffice = util::Tagged<std::string, model::Office>;
    for(const auto& office : offices){
        std::string id =  office.GetParamAsString(OfficeFields::ID);
        model::Office office_model(IdOffice(id)
            , {office.GetParamAsInt(OfficeFields::POSITION_X), office.GetParamAsInt(OfficeFields::POSITION_Y)}
            , {office.GetParamAsInt(OfficeFields::OFFSET_X), office.GetParamAsInt(OfficeFields::OFFSET_Y)}
        );
        map.AddOffice(move(office_model));
    }
}

void LoadGame(model::Game& game,const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    using IdMap = util::Tagged<std::string, model::Map>;

    const auto json_obj = boost_json::ParseFile(json_path);
    // Устанавливаем скорость собак по умолчаниюю для всех карт
    if(json_obj.ContainsParam(ConfigFields::DEFAULT_DOG_SPEED)){ //если есть
        game.SetDefaultDogSpeed(json_obj.GetParamAsDouble(ConfigFields::DEFAULT_DOG_SPEED));
    } else { // если нет - записываем скорость по умолчанию 1.0
        game.SetDefaultDogSpeed(1.0);
    };
    
    const auto jmaps = json_obj.GetParamAsArray(ConfigFields::MAPS);
    for(const auto& jmap : jmaps){
        std::string id =  jmap.GetParamAsString(MapFields::ID);
        std::string name =  jmap.GetParamAsString(MapFields::NAME);
        
        model::Map map(IdMap(id), name);

        AddRoads(map, jmap.GetParamAsArray(MapFields::ROADS));
        AddBuildings(map, jmap.GetParamAsArray(MapFields::BILDINGS));
        AddOffices(map, jmap.GetParamAsArray(MapFields::OFFICES));
        // Если для карты есть скорость записываем ее в модель
        if(jmap.ContainsParam(MapFields::DOG_SPEED)){
            map.SetDogSpeed(jmap.GetParamAsDouble(MapFields::DOG_SPEED));
        } else { // если нет - записываем скорость по умолчанию для всех карт
            map.SetDogSpeed(game.GetDefaultDogSpeed());
        };

        game.AddMap(move(map));
    }
}

}  // namespace json_loader

