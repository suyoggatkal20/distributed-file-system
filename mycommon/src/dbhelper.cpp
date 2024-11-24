#include "dbhelper.h"
#include <iostream>
#include "config.h"
#include "custom_exceptions.h"

MySQLDatabase::MySQLDatabase() : conn(nullptr) {}

MySQLDatabase::~MySQLDatabase()
{
    disconnect();
}

bool MySQLDatabase::connect()
{
    conn = mysql_init(NULL);
    if (!conn)
    {
        std::cerr << "mysql_init() failed\n";
        return false;
    }

    if (!mysql_real_connect(conn, config.getString("db.host").c_str(), config.getString("db.username").c_str(), config.getString("db.password").c_str(), config.getString("db.db_name").c_str(), 0, NULL, 0))
    {
        std::cerr << "mysql_real_connect() failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return false;
    }
    return true;
}

void MySQLDatabase::executeQuery(const std::string &query)
{
    if (mysql_query(conn, query.c_str()))
    {
        unsigned int error_code = mysql_errno(conn); // Retrieve the error code
        if (error_code == 1062)                      // MySQL error code for duplicate entry
        {
            throw DuplicateDbEntry("Duplicate entry detected: " + std::string(mysql_error(conn)));
        }
        else
        {
            throw std::runtime_error("Error in db query: " + std::string(mysql_error(conn)));
        }
    }
}

MYSQL_RES *MySQLDatabase::execSelect(const std::string &query)
{
    if (!conn)
    {
        std::cerr << "Database connection is null." << std::endl;
        throw DatabaseConnectionException("Database connection not established.");
    }
    std::vector<MYSQL_ROW> rows;
    if (mysql_query(conn, query.c_str()))
    {

        throw DatabaseConnectionException("Query failed: " + std::string(mysql_error(conn)));
    }
    MYSQL_RES *res = mysql_store_result(conn);
    if (!res)
    {
        throw DatabaseConnectionException("mysql_store_result() failed: " + std::string(mysql_error(conn)));
    }
    return res;
}

std::vector<MYSQL_ROW> MySQLDatabase::executeSelectQuery(const std::string &query)
{
    std::vector<MYSQL_ROW> rows;

    MYSQL_RES *res = execSelect(query);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        rows.push_back(row);
    }

    return rows;
}

void MySQLDatabase::disconnect()
{
    if (conn)
    {
        mysql_close(conn);
        conn = nullptr;
    }
}

MYSQL *MySQLDatabase::getConnection()
{
    return conn;
}
