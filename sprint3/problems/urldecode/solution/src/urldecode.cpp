#include "urldecode.h"

#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>

std::string UrlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        char current_char = str[i];

        if (current_char == '+') {
            // Пробел кодируется как '+'
            result += ' ';
        } else if (current_char == '%') {
            // Обработка %-последовательности
            if (i + 2 >= str.size()) {
                throw std::invalid_argument("Incomplete percent encoding");
            }

            char hex[3] = {str[i + 1], str[i + 2], '\0'};
            
            // Проверка что символы hex-цифры
            if (!isxdigit(hex[0]) || !isxdigit(hex[1])) {
                throw std::invalid_argument("Invalid percent encoding");
            }

            unsigned char decoded_char;
            auto [ptr, ec] = std::from_chars(hex, hex + 2, decoded_char, 16);
            
            if (ec != std::errc()) {
                throw std::invalid_argument("Invalid hex digits in percent encoding");
            }

            result += decoded_char;
            i += 2; // Пропускаем два hex символа
        } else {
            // Обычный символ
            result += current_char;
        }
    }

    return result;
}