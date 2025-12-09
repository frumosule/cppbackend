#include "serializing_listener.h"
#include <fstream>
#include <iostream>

namespace infrastructure
{

    SerializingListener::SerializingListener(model::GameSessionManager &session_manager,
                                             const std::filesystem::path &state_file,
                                             std::chrono::milliseconds save_period)
        : session_manager_(session_manager), state_file_(state_file), save_period_(save_period)
    {
        auto parent_path = state_file_.parent_path();
        if (!parent_path.empty())
        {
            std::filesystem::create_directories(parent_path);
        }

        std::cout << "SerializingListener initialized: " << state_file_
                  << ", save period: " << save_period_.count() << "ms" << std::endl;
    }

    void SerializingListener::OnTick(std::chrono::milliseconds delta)
    {
        std::lock_guard lock(save_mutex_);

        time_since_last_save_ += delta;

        if (ShouldSave())
        {
            SaveState();
            time_since_last_save_ = std::chrono::milliseconds(0);
        }
    }

    void SerializingListener::OnShutdown()
    {
        std::lock_guard lock(save_mutex_);
        SaveState();
    }

    void SerializingListener::SaveState()
    {
        try
        {
            auto temp_file = state_file_;
            temp_file += ".tmp";

            std::ofstream file(temp_file, std::ios::binary);
            if (!file)
            {
                throw std::runtime_error("Cannot open state file for writing: " + temp_file.string());
            }

            session_manager_.Serialize(file);

            file.close();

            std::filesystem::rename(temp_file, state_file_);

            std::cout << "Game state saved to: " << state_file_ << std::endl;
        }
        catch (const std::exception &ex)
        {
            std::cerr << "Failed to save game state: " << ex.what() << std::endl;
            throw;
        }
    }

    bool SerializingListener::ShouldSave() const
    {
        return save_period_.count() > 0 && time_since_last_save_ >= save_period_;
    }

} // namespace infrastructure