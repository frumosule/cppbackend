#pragma once
#include "model.h"
#include <filesystem>
#include <boost/json.hpp>

namespace json_loader {

class JsonLoaderException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

model::Game LoadGame(const std::filesystem::path& json_path);

namespace detail {
    std::string LoadFileContent(const std::filesystem::path& path);
    boost::json::value ParseJson(const std::string& content);
    model::Map ParseMap(const boost::json::object& map_obj);
    model::Road ParseRoad(const boost::json::object& road_obj);
    model::Building ParseBuilding(const boost::json::object& building_obj);
    model::Office ParseOffice(const boost::json::object& office_obj);
} // namespace detail

} // namespace json_loader