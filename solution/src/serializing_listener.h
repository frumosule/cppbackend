#pragma once

#include "application_listener.h"
#include "game_session_manager.h"
#include <filesystem>
#include <chrono>
#include <fstream>
#include <mutex>

namespace infrastructure {

class SerializingListener : public app::ApplicationListener {
public:
    SerializingListener(model::GameSessionManager& session_manager, 
                       const std::filesystem::path& state_file,
                       std::chrono::milliseconds save_period = std::chrono::milliseconds(0));
    
    void OnTick(std::chrono::milliseconds delta) override;
    void OnShutdown() override;

private:
    void SaveState();
    bool ShouldSave() const;
    
    model::GameSessionManager& session_manager_;
    std::filesystem::path state_file_;
    std::chrono::milliseconds save_period_;
    std::chrono::milliseconds time_since_last_save_{0};
    mutable std::mutex save_mutex_;
};

} // namespace infrastructure