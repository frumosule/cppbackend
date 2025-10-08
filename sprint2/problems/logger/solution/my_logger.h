#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>
#include <filesystem>
#include <iostream>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    mutable std::mutex mutex_;
    std::ofstream log_file_;
    std::string current_date_;
    
    // Внутренний метод без блокировки - для использования внутри уже заблокированных методов
    auto GetTimeUnsafe() const {
        if (manual_ts_) {
            return *manual_ts_;
        }
        return std::chrono::system_clock::now();
    }

    std::string GetTimeStampUnsafe() const {
        const auto now = GetTimeUnsafe();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        
        // Безопасная версия localtime
        std::tm tm;
    #ifdef _WIN32
        localtime_s(&tm, &t_c);
    #else
        localtime_r(&t_c, &tm);
    #endif
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string GetFileTimeStampUnsafe() const {
        const auto now = GetTimeUnsafe();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        
        // Безопасная версия localtime
        std::tm tm;
    #ifdef _WIN32
        localtime_s(&tm, &t_c);
    #else
        localtime_r(&t_c, &tm);
    #endif
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y_%m_%d");
        return oss.str();
    }

    std::string GetLogFileNameUnsafe() const {
        return "/var/log/sample_log_" + GetFileTimeStampUnsafe() + ".log";
    }

    bool EnsureLogFileUnsafe() {
        try {
            std::string new_date = GetFileTimeStampUnsafe();
            
            if (!log_file_.is_open() || current_date_ != new_date) {
                if (log_file_.is_open()) {
                    log_file_.close();
                }
                
                std::string filename = GetLogFileNameUnsafe();
                std::filesystem::path path(filename);
                
                // Создаем директорию если не существует
                std::filesystem::create_directories(path.parent_path());
                
                log_file_.open(filename, std::ios::app);
                if (!log_file_.is_open()) {
                    std::cerr << "Failed to open log file: " << filename << std::endl;
                    return false;
                }
                current_date_ = new_date;
                std::cout << "Opened log file: " << filename << std::endl;
            }
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error ensuring log file: " << e.what() << std::endl;
            return false;
        }
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard lock(mutex_);
        
        if (!EnsureLogFileUnsafe()) {
            return;
        }
        
        if (log_file_.is_open()) {
            log_file_ << GetTimeStampUnsafe() << ": ";
            // Fold expression для записи всех аргументов
            (log_file_ << ... << args);
            log_file_ << std::endl;
            log_file_.flush(); // Гарантируем запись
        }
    }

    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard lock(mutex_);
        manual_ts_ = ts;
    }
    
    ~Logger() {
        std::lock_guard lock(mutex_);
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }
};