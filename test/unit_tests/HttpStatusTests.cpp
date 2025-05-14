#include "catch.hpp"
#include <string>
#include <map>

TEST_CASE("HTTP Status Codes Tests") {
    // Create a map of status codes to status texts
    std::map<int, std::string> statusTexts;

    // 1xx - Informational
    statusTexts[100] = "Continue";
    statusTexts[101] = "Switching Protocols";

    // 2xx - Success
    statusTexts[200] = "OK";
    statusTexts[201] = "Created";
    statusTexts[204] = "No Content";

    // 3xx - Redirection
    statusTexts[301] = "Moved Permanently";
    statusTexts[302] = "Found";
    statusTexts[304] = "Not Modified";

    // 4xx - Client Error
    statusTexts[400] = "Bad Request";
    statusTexts[401] = "Unauthorized";
    statusTexts[403] = "Forbidden";
    statusTexts[404] = "Not Found";
    statusTexts[405] = "Method Not Allowed";
    statusTexts[413] = "Payload Too Large";

    // 5xx - Server Error
    statusTexts[500] = "Internal Server Error";
    statusTexts[501] = "Not Implemented";
    statusTexts[502] = "Bad Gateway";
    statusTexts[503] = "Service Unavailable";
    statusTexts[504] = "Gateway Timeout";

    SECTION("Test Common Status Codes") {
        // 1xx - Informational
        REQUIRE(statusTexts[100] == "Continue");
        REQUIRE(statusTexts[101] == "Switching Protocols");

        // 2xx - Success
        REQUIRE(statusTexts[200] == "OK");
        REQUIRE(statusTexts[201] == "Created");
        REQUIRE(statusTexts[204] == "No Content");

        // 3xx - Redirection
        REQUIRE(statusTexts[301] == "Moved Permanently");
        REQUIRE(statusTexts[302] == "Found");
        REQUIRE(statusTexts[304] == "Not Modified");

        // 4xx - Client Error
        REQUIRE(statusTexts[400] == "Bad Request");
        REQUIRE(statusTexts[401] == "Unauthorized");
        REQUIRE(statusTexts[403] == "Forbidden");
        REQUIRE(statusTexts[404] == "Not Found");
        REQUIRE(statusTexts[405] == "Method Not Allowed");
        REQUIRE(statusTexts[413] == "Payload Too Large");

        // 5xx - Server Error
        REQUIRE(statusTexts[500] == "Internal Server Error");
        REQUIRE(statusTexts[501] == "Not Implemented");
        REQUIRE(statusTexts[502] == "Bad Gateway");
        REQUIRE(statusTexts[503] == "Service Unavailable");
        REQUIRE(statusTexts[504] == "Gateway Timeout");
    }

    SECTION("Test Status Codes Count") {
        // Check that we have a reasonable number of status codes
        REQUIRE(statusTexts.size() >= 19); // We defined at least 19 status codes
    }
}
