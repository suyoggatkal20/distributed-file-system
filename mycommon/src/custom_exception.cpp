#include "custom_exceptions.h"
// Implementation for DirectoryNotFoundException
DirectoryNotFoundException::DirectoryNotFoundException(const std::string &dirName)
    : std::runtime_error("Directory not found: " + dirName), directoryName(dirName) {}

std::string DirectoryNotFoundException::getDirectoryName() const
{
    return directoryName;
}

// Implementation for DatabaseConnectionException
DatabaseConnectionException::DatabaseConnectionException(const std::string &message)
    : std::runtime_error("Database connection failed: " + message) {}

// Implementation for MySQLQueryException
MySQLQueryException::MySQLQueryException(const std::string &query, const std::string &errorMessage)
    : std::runtime_error("MySQL query error: " + errorMessage), query(query), errorMessage(errorMessage) {}

std::string MySQLQueryException::getQuery() const
{
    return query;
}

std::string MySQLQueryException::getErrorMessage() const
{
    return errorMessage;
}

DuplicateDbEntry::DuplicateDbEntry(const std::string &errorMessage)
    : std::runtime_error("MySQL query error: " + errorMessage), errorMessage(errorMessage) {}
