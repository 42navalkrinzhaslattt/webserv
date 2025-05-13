#include "HttpServer.hpp"

void HttpServer::initStatusTexts() {
    // 1xx - Informational
    _statusTexts[100] = "Continue";
    _statusTexts[101] = "Switching Protocols";
    _statusTexts[102] = "Processing";
    _statusTexts[103] = "Early Hints";

    // 2xx - Success
    _statusTexts[200] = "OK";
    _statusTexts[201] = "Created";
    _statusTexts[202] = "Accepted";
    _statusTexts[203] = "Non-Authoritative Information";
    _statusTexts[204] = "No Content";
    _statusTexts[205] = "Reset Content";
    _statusTexts[206] = "Partial Content";
    _statusTexts[207] = "Multi-Status";
    _statusTexts[208] = "Already Reported";
    _statusTexts[226] = "IM Used";

    // 3xx - Redirection
    _statusTexts[300] = "Multiple Choices";
    _statusTexts[301] = "Moved Permanently";
    _statusTexts[302] = "Found";
    _statusTexts[303] = "See Other";
    _statusTexts[304] = "Not Modified";
    _statusTexts[305] = "Use Proxy";
    _statusTexts[307] = "Temporary Redirect";
    _statusTexts[308] = "Permanent Redirect";

    // 4xx - Client Error
    _statusTexts[400] = "Bad Request";
    _statusTexts[401] = "Unauthorized";
    _statusTexts[402] = "Payment Required";
    _statusTexts[403] = "Forbidden";
    _statusTexts[404] = "Not Found";
    _statusTexts[405] = "Method Not Allowed";
    _statusTexts[406] = "Not Acceptable";
    _statusTexts[407] = "Proxy Authentication Required";
    _statusTexts[408] = "Request Timeout";
    _statusTexts[409] = "Conflict";
    _statusTexts[410] = "Gone";
    _statusTexts[411] = "Length Required";
    _statusTexts[412] = "Precondition Failed";
    _statusTexts[413] = "Payload Too Large";
    _statusTexts[414] = "URI Too Long";
    _statusTexts[415] = "Unsupported Media Type";
    _statusTexts[416] = "Range Not Satisfiable";
    _statusTexts[417] = "Expectation Failed";
    _statusTexts[418] = "I'm a teapot";
    _statusTexts[421] = "Misdirected Request";
    _statusTexts[422] = "Unprocessable Entity";
    _statusTexts[423] = "Locked";
    _statusTexts[424] = "Failed Dependency";
    _statusTexts[425] = "Too Early";
    _statusTexts[426] = "Upgrade Required";
    _statusTexts[428] = "Precondition Required";
    _statusTexts[429] = "Too Many Requests";
    _statusTexts[431] = "Request Header Fields Too Large";
    _statusTexts[451] = "Unavailable For Legal Reasons";

    // 5xx - Server Error
    _statusTexts[500] = "Internal Server Error";
    _statusTexts[501] = "Not Implemented";
    _statusTexts[502] = "Bad Gateway";
    _statusTexts[503] = "Service Unavailable";
    _statusTexts[504] = "Gateway Timeout";
    _statusTexts[505] = "HTTP Version Not Supported";
    _statusTexts[506] = "Variant Also Negotiates";
    _statusTexts[507] = "Insufficient Storage";
    _statusTexts[508] = "Loop Detected";
    _statusTexts[510] = "Not Extended";
    _statusTexts[511] = "Network Authentication Required";

    log.debug() << "Initialized " << _statusTexts.size() << " HTTP status texts" << std::endl;
}

std::string HttpServer::getStatusText(int statusCode) {
    std::map<int, std::string>::const_iterator it = _statusTexts.find(statusCode);

    if (it != _statusTexts.end()) {
        return it->second;
    }

    return "Unknown Status";
}
