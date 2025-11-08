#define BOOST_TEST_MODULE UrlDecodeTests
#include <boost/test/included/unit_test.hpp>  // header-only версия

#include "../src/urldecode.h"
#include <stdexcept>

BOOST_AUTO_TEST_SUITE(UrlDecodeTests)

BOOST_AUTO_TEST_CASE(EmptyString) {
    BOOST_TEST(UrlDecode("") == "");
}

BOOST_AUTO_TEST_CASE(NoPercentEncoding) {
    BOOST_TEST(UrlDecode("Hello World") == "Hello World");
    BOOST_TEST(UrlDecode("abc123") == "abc123");
}

BOOST_AUTO_TEST_CASE(ValidPercentEncoding) {
    BOOST_TEST(UrlDecode("Hello%20World") == "Hello World");
    BOOST_TEST(UrlDecode("test%2Bcase") == "test+case");
}

BOOST_AUTO_TEST_CASE(PlusCharacter) {
    BOOST_TEST(UrlDecode("Hello+World") == "Hello World");
}

BOOST_AUTO_TEST_CASE(IncompletePercentEncoding) {
    BOOST_CHECK_THROW(UrlDecode("%"), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%1"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(InvalidPercentEncoding) {
    BOOST_CHECK_THROW(UrlDecode("%GG"), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()