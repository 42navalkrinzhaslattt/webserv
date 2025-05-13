#include "HttpServer.hpp"

void HttpServer::initMimeTypes() {
    // Text types
    _mimeTypes[".html"] = "text/html";
    _mimeTypes[".htm"] = "text/html";
    _mimeTypes[".css"] = "text/css";
    _mimeTypes[".js"] = "application/javascript";
    _mimeTypes[".txt"] = "text/plain";
    _mimeTypes[".csv"] = "text/csv";
    _mimeTypes[".xml"] = "application/xml";
    _mimeTypes[".json"] = "application/json";

    // Image types
    _mimeTypes[".jpg"] = "image/jpeg";
    _mimeTypes[".jpeg"] = "image/jpeg";
    _mimeTypes[".png"] = "image/png";
    _mimeTypes[".gif"] = "image/gif";
    _mimeTypes[".svg"] = "image/svg+xml";
    _mimeTypes[".ico"] = "image/x-icon";
    _mimeTypes[".webp"] = "image/webp";

    // Audio types
    _mimeTypes[".mp3"] = "audio/mpeg";
    _mimeTypes[".wav"] = "audio/wav";
    _mimeTypes[".ogg"] = "audio/ogg";

    // Video types
    _mimeTypes[".mp4"] = "video/mp4";
    _mimeTypes[".webm"] = "video/webm";
    _mimeTypes[".avi"] = "video/x-msvideo";

    // Application types
    _mimeTypes[".pdf"] = "application/pdf";
    _mimeTypes[".zip"] = "application/zip";
    _mimeTypes[".gz"] = "application/gzip";
    _mimeTypes[".tar"] = "application/x-tar";
    _mimeTypes[".doc"] = "application/msword";
    _mimeTypes[".docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    _mimeTypes[".xls"] = "application/vnd.ms-excel";
    _mimeTypes[".xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    _mimeTypes[".ppt"] = "application/vnd.ms-powerpoint";
    _mimeTypes[".pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";

    // Font types
    _mimeTypes[".ttf"] = "font/ttf";
    _mimeTypes[".otf"] = "font/otf";
    _mimeTypes[".woff"] = "font/woff";
    _mimeTypes[".woff2"] = "font/woff2";

    log.debug() << "Initialized " << _mimeTypes.size() << " MIME types" << std::endl;
}

std::string HttpServer::getMimeType(const std::string &path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }

    std::string extension = path.substr(dotPos);
    std::map<std::string, std::string>::const_iterator it = _mimeTypes.find(extension);

    if (it != _mimeTypes.end()) {
        return it->second;
    }

    return "application/octet-stream";
}
