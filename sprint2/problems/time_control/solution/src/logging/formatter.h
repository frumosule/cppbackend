#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>

namespace logging = boost::log;
namespace json = boost::json;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

class JsonFormatter {
public:
    void operator()(logging::record_view const& rec, logging::formatting_ostream& strm) const;
};

void setup_logging();