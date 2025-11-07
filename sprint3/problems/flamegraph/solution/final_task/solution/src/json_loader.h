#pragma once

#include "model.h"

#include <filesystem>

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader