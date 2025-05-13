#ifndef DIRECTORY_INDEXER_HPP
#define DIRECTORY_INDEXER_HPP

#include "Logger.hpp"

#include <string>
#include <vector>

class DirectoryIndexer {
public:
    DirectoryIndexer(Logger &log);
    ~DirectoryIndexer();

    std::string indexDirectory(const std::string &requestPath, const std::string &diskPath);

private:
    Logger &log;

    struct FileEntry {
        std::string name;
        std::string type;
        size_t size;
        std::string lastModified;
    };

    // Helper class for sorting file entries
    class SortFileEntries {
    public:
        bool operator()(const FileEntry &a, const FileEntry &b) const;
    };

    std::vector<FileEntry> getDirectoryContents(const std::string &path);
    std::string formatSize(size_t size);
    std::string formatTime(time_t time);
    std::string getFileType(const std::string &name);
};

#endif // DIRECTORY_INDEXER_HPP
