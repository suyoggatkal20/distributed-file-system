#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "dbhelper.h"
#include "models.h"

class FileUtils
{
public:
    // Constructor that accepts an existing MySQL connection object
    FileUtils(MySQLDatabase &database);
    FileUtils();

    // Function to get the directory_id by path
    int getDirectoryIdByPath(const std::string &path);
    std::string findPathToNumberInNaryTree(int target, int n);

    void createDirectories(const std::string path);

    bool writeToFile(std::ofstream &file, const char *buffer, size_t bufferSize);

    std::string getForattedString(ChunkReplicasSlaveModel chunkReplicasSlaveModel);

private:
    // MySQL connection object
    MySQLDatabase &db;

    // Helper function to split a path into directories
    std::vector<std::string> splitPath(const std::string &path);
};

#endif
