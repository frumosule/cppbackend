#include "my_logger.h"

#include <string_view>
#include <thread>
#include <iostream>

using namespace std::literals;

int main() {

     std::cout <<"Start" << std::endl;
    // Будем устанавливать моменты времени в секундах от начала эпохи.
    // Конкретные значения не так важны, главное, что часы идут монотонно.
    Logger::GetInstance().SetTimestamp(std::chrono::system_clock::time_point{1000000s});

     std::cout <<"Time" << std::endl;

    // Логируем значения разных типов.
    LOG("Hello "sv, "world "s, 123);

    std::cout <<"First check" << std::endl;

    // Проверяем, что логер можно вызвать с очень большим количеством параметров.
    LOG(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 
        1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 
        1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0);

        std::cout <<"Second check" << std::endl;
    
    Logger::GetInstance().SetTimestamp(std::chrono::system_clock::time_point{10000000s});
    LOG("Brilliant logger.", " ", "I Love it");

    std::cout <<"Third check" << std::endl;

    // Выполним ещё 100000 логирований.
    static const int attempts = 100000;
    for(int i = 0; i < attempts; ++i) {
        std::chrono::system_clock::time_point ts(std::chrono::seconds(10000000 + i * 100));
        Logger::GetInstance().SetTimestamp(ts);

        LOG("Logging attempt ", i, ". ", "I Love it");
    }

    std::cout <<"It's okay" << std::endl;
}
