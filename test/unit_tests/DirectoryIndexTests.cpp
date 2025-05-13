#include "catch.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "DirectoryIndexer.hpp"
#include <sstream>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

// Helper function to create a temporary directory
void createTempDirectory(const std::string& path) {
    mkdir(path.c_str(), 0755);
}

// Helper function to create a temporary file
void createTempFile(const std::string& path, const std::string& content = "") {
    std::ofstream file(path.c_str());
    file << content;
    file.close();
}

// Helper function to delete a temporary file or directory
void deleteTempPath(const std::string& path) {
    unlink(path.c_str());
}

TEST_CASE("Directory Index Tests") {
    std::stringstream logStream;
    Logger log(logStream, "DEBUG");
    
    // Create a temporary test directory
    std::string testDir = "test_dir";
    createTempDirectory(testDir);
    
    SECTION("Test DirectoryIndexer::generateDirectoryIndex") {
        // Create some test files
        createTempFile(testDir + "/file1.txt");
        createTempFile(testDir + "/file2.html");
        createTempDirectory(testDir + "/subdir");
        
        // Generate directory index
        std::string uri = "/test/";
        std::string html = DirectoryIndexer::generateDirectoryIndex(testDir, uri);
        
        // Check that the HTML contains the directory name
        REQUIRE(html.find("<title>Index of /test/</title>") != std::string::npos);
        
        // Check that the HTML contains the files
        REQUIRE(html.find("file1.txt") != std::string::npos);
        REQUIRE(html.find("file2.html") != std::string::npos);
        REQUIRE(html.find("subdir/") != std::string::npos);
    }
    
    SECTION("Test DirectoryIndexer::generateDirectoryIndex with empty directory") {
        // Generate directory index for an empty directory
        std::string uri = "/empty/";
        std::string html = DirectoryIndexer::generateDirectoryIndex(testDir, uri);
        
        // Check that the HTML contains the directory name
        REQUIRE(html.find("<title>Index of /empty/</title>") != std::string::npos);
        
        // Check that the HTML contains the parent directory link
        REQUIRE(html.find("../") != std::string::npos);
    }
    
    // Clean up
    deleteTempPath(testDir + "/file1.txt");
    deleteTempPath(testDir + "/file2.html");
    deleteTempPath(testDir + "/subdir");
    deleteTempPath(testDir);
}
