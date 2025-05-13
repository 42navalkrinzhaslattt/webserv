#include "DirectoryIndexer.hpp"
#include "Repr.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

DirectoryIndexer::DirectoryIndexer(Logger &log) : log(log) {}

DirectoryIndexer::~DirectoryIndexer() {}

std::string DirectoryIndexer::indexDirectory(const std::string &requestPath, const std::string &diskPath) {
    log.debug() << "Generating directory index for " << repr(diskPath) << std::endl;

    std::vector<FileEntry> entries = getDirectoryContents(diskPath);

    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
         << "    <title>Index of " << requestPath << "</title>\n"
         << "    <style>\n"
         << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
         << "        h1 { color: #333; }\n"
         << "        table { border-collapse: collapse; width: 100%; }\n"
         << "        th, td { text-align: left; padding: 8px; }\n"
         << "        th { background-color: #f2f2f2; }\n"
         << "        tr:nth-child(even) { background-color: #f9f9f9; }\n"
         << "        a { text-decoration: none; color: #0066cc; }\n"
         << "        a:hover { text-decoration: underline; }\n"
         << "        .size { text-align: right; }\n"
         << "        .date { white-space: nowrap; }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <h1>Index of " << requestPath << "</h1>\n"
         << "    <table>\n"
         << "        <tr>\n"
         << "            <th>Name</th>\n"
         << "            <th>Type</th>\n"
         << "            <th>Size</th>\n"
         << "            <th>Last Modified</th>\n"
         << "        </tr>\n";

    // Add parent directory link if not at root
    if (requestPath != "/") {
        html << "        <tr>\n"
             << "            <td><a href=\"..\">..</a></td>\n"
             << "            <td>Directory</td>\n"
             << "            <td class=\"size\">-</td>\n"
             << "            <td class=\"date\">-</td>\n"
             << "        </tr>\n";
    }

    // Add entries
    for (size_t i = 0; i < entries.size(); ++i) {
        const FileEntry &entry = entries[i];
        html << "        <tr>\n"
             << "            <td><a href=\"" << entry.name << "\">" << entry.name << "</a></td>\n"
             << "            <td>" << entry.type << "</td>\n"
             << "            <td class=\"size\">" << (entry.type == "Directory" ? "-" : formatSize(entry.size)) << "</td>\n"
             << "            <td class=\"date\">" << entry.lastModified << "</td>\n"
             << "        </tr>\n";
    }

    html << "    </table>\n"
         << "    <hr>\n"
         << "    <p>Webserv - HTTP/1.1 Server in C++98</p>\n"
         << "</body>\n"
         << "</html>";

    return html.str();
}

std::vector<DirectoryIndexer::FileEntry> DirectoryIndexer::getDirectoryContents(const std::string &path) {
    std::vector<FileEntry> entries;

    DIR *dir = opendir(path.c_str());
    if (!dir) {
        log.error() << "Failed to open directory " << repr(path) << std::endl;
        return entries;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;

        // Skip . directory
        if (name == ".") {
            continue;
        }

        FileEntry fileEntry;
        fileEntry.name = name;

        std::string fullPath = path + "/" + name;
        struct stat statBuf;
        if (stat(fullPath.c_str(), &statBuf) == 0) {
            if (S_ISDIR(statBuf.st_mode)) {
                fileEntry.type = "Directory";
                fileEntry.name += "/";
            } else {
                fileEntry.type = getFileType(name);
            }

            fileEntry.size = statBuf.st_size;
            fileEntry.lastModified = formatTime(statBuf.st_mtime);
        } else {
            fileEntry.type = "Unknown";
            fileEntry.size = 0;
            fileEntry.lastModified = "-";
        }

        entries.push_back(fileEntry);
    }

    closedir(dir);

    // Sort entries: directories first, then files, both alphabetically
    std::sort(entries.begin(), entries.end(),
        // C++98 compatible sort predicate
        SortFileEntries());

    return entries;
}

// Custom comparison function for sorting file entries
bool DirectoryIndexer::SortFileEntries::operator()(const FileEntry &a, const FileEntry &b) const {
    // Directories come before files
    bool aIsDir = a.type == "Directory";
    bool bIsDir = b.type == "Directory";

    if (aIsDir != bIsDir) {
        return aIsDir > bIsDir; // Directories first
    }

    // Alphabetical order within the same type
    return a.name < b.name;
}

std::string DirectoryIndexer::formatSize(size_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double fileSize = static_cast<double>(size);

    while (fileSize >= 1024.0 && unitIndex < 4) {
        fileSize /= 1024.0;
        ++unitIndex;
    }

    std::ostringstream sizeStr;
    if (unitIndex == 0) {
        sizeStr << fileSize << " " << units[unitIndex];
    } else {
        sizeStr << std::fixed << std::setprecision(2) << fileSize << " " << units[unitIndex];
    }

    return sizeStr.str();
}

std::string DirectoryIndexer::formatTime(time_t time) {
    char buffer[64];
    struct tm *timeinfo = localtime(&time);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

std::string DirectoryIndexer::getFileType(const std::string &name) {
    size_t dotPos = name.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "File";
    }

    std::string extension = name.substr(dotPos + 1);

    if (extension == "html" || extension == "htm") {
        return "HTML";
    } else if (extension == "css") {
        return "CSS";
    } else if (extension == "js") {
        return "JavaScript";
    } else if (extension == "jpg" || extension == "jpeg") {
        return "JPEG Image";
    } else if (extension == "png") {
        return "PNG Image";
    } else if (extension == "gif") {
        return "GIF Image";
    } else if (extension == "pdf") {
        return "PDF";
    } else if (extension == "txt") {
        return "Text";
    } else if (extension == "zip") {
        return "ZIP Archive";
    } else if (extension == "py") {
        return "Python Script";
    } else if (extension == "sh") {
        return "Shell Script";
    } else if (extension == "pl") {
        return "Perl Script";
    } else {
        return "File";
    }
}
