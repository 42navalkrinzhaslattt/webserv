#include "catch.hpp"
#include "Utils.hpp"
#include <string>

TEST_CASE("Utils Tests") {
    SECTION("Test Utils::convertSizeToBytes") {
        // Test with bytes (no suffix)
        REQUIRE(Utils::convertSizeToBytes("1024") == 1024);

        // Test with kilobytes (k or K suffix)
        REQUIRE(Utils::convertSizeToBytes("1k") == 1024);
        REQUIRE(Utils::convertSizeToBytes("1K") == 1024);

        // Test with megabytes (m or M suffix)
        REQUIRE(Utils::convertSizeToBytes("1m") == 1024 * 1024);
        REQUIRE(Utils::convertSizeToBytes("1M") == 1024 * 1024);

        // Test with gigabytes (g or G suffix)
        REQUIRE(Utils::convertSizeToBytes("1g") == 1024 * 1024 * 1024);
        REQUIRE(Utils::convertSizeToBytes("1G") == 1024 * 1024 * 1024);
    }

    SECTION("Test Utils::isPrefix") {
        // Test with prefix
        REQUIRE(Utils::isPrefix("/test", "/test/file.html") == true);
        REQUIRE(Utils::isPrefix("/", "/test/file.html") == true);
        REQUIRE(Utils::isPrefix("/test", "/test") == true);

        // Test with non-prefix
        REQUIRE(Utils::isPrefix("/test2", "/test/file.html") == false);
        REQUIRE(Utils::isPrefix("/test/file", "/test") == false);
        REQUIRE(Utils::isPrefix("/test/", "/test") == false);
    }
}
