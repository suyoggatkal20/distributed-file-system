#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "dbhelper.h"

class FileUtils
{
public:
    // Constructor that accepts an existing MySQL connection object
    FileUtils(MySQLDatabase &database);

    // Function to get the directory_id by path
    int getDirectoryIdByPath(const std::string &path);

private:
    // MySQL connection object
    MySQLDatabase& db;

    // Helper function to split a path into directories
    std::vector<std::string> splitPath(const std::string &path);
};

#endif 
