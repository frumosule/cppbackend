#include "formatter.h"
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time;

void JsonFormatter::operator()(logging::record_view const& rec, logging::formatting_ostream& strm) const {
    json::object log_entry;
    
    // Timestamp
    auto timestamp = logging::extract<ptime>("TimeStamp", rec);
    if (timestamp) {
        log_entry["timestamp"] = to_iso_extended_string(timestamp.get());
    }
    
    // Message
    auto message = logging::extract<std::string>("Message", rec);
    if (message) {
        log_entry["message"] = message.get();
    }
    
    // Additional data
    auto data = logging::extract<json::value>("AdditionalData", rec);
    if (data) {
        log_entry["data"] = data.get();
    } else {
        log_entry["data"] = json::object{};
    }
    
    strm << json::serialize(log_entry) << "\n";
    strm.flush();
}

void setup_logging() {
    logging::add_common_attributes();
    
    auto sink = logging::add_console_log(
        std::cout,
        logging::keywords::auto_flush = true,
        logging::keywords::format = JsonFormatter{}  // ← ПРЯМО ЗДЕСЬ
    );
    
    // Устанавливаем уровень логирования
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );
}