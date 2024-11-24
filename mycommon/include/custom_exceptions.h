#ifndef CUSTOM_EXCEPTIONS_H
#define CUSTOM_EXCEPTIONS_H

#include <stdexcept>
#include <string>

// Custom exception for directory not found
class DirectoryNotFoundException : public std::runtime_error {
public:
    explicit DirectoryNotFoundException(const std::string& dirName);
    std::string getDirectoryName() const;
private:
    std::string directoryName;
};
      
// Custom exception for database connection failure
class DatabaseConnectionException : public std::runtime_error {
public:
    explicit DatabaseConnectionException(const std::string& message);
};

// Custom exception for query errors in MySQL
class MySQLQueryException : public std::runtime_error {
public:
    explicit MySQLQueryException(const std::string& query, const std::string& errorMessage);
    std::string getQuery() const;
    std::string getErrorMessage() const;
private:
    std::string query;
    std::string errorMessage;
};

class DuplicateDbEntry : public std::runtime_error {
public:
    explicit DuplicateDbEntry(const std::string& errorMessage);
    std::string getErrorMessage() const;
private:
    std::string errorMessage;
};



#endif // CUSTOM_EXCEPTIONS_H
