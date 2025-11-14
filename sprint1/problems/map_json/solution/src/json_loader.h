#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "model.h"
//#include "boost_json.cpp"

namespace json_loader {

namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path);

std::string ReadFileToString(const std::filesystem::path& filePath);

model::Building JsonToBuilding(const json::object& obj);

model::Office JsonToOffice(const json::object& obj);

model::Road JsonToRoad(const json::object& obj);

}  // namespace json_loader