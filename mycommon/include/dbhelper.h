#ifndef DBHELPER_H
#define DBHELPER_H

#include <mysql/mysql.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>
#include <iostream>

template <typename T>
class DBEntity
{
public:
    virtual T mapRowToEntity(const MYSQL_ROW &row, MYSQL_FIELD *fields) const = 0;
    virtual ~DBEntity() = default;
};

class MySQLDatabase
{
public:
    MySQLDatabase();
    ~MySQLDatabase();

    bool connect();
    void disconnect();
    void executeQuery(const std::string &query);

    std::vector<MYSQL_ROW> executeSelectQuery(const std::string &query);

    template <typename T>
    std::vector<T> executeTemplateQuery(const std::string &query, const DBEntity<T> &entityTemplate)
    {
        std::vector<T> results;

        MYSQL_RES *rows = execSelect(query);

        MYSQL_FIELD *fields = nullptr;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(rows)))
        {
            T entity = entityTemplate.mapRowToEntity(row, fields);
            results.push_back(entity);
        }

        mysql_free_result(rows);

        return results;
    }

    MYSQL *getConnection();

private:
    MYSQL *conn;
    MYSQL_RES *execSelect(const std::string &query);
};

#endif