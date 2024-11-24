#include "FileUtils.h"
#include <stdexcept>
#include "custom_exceptions.h"
#include <string>
#include "models.h"
#include "dbhelper.h"

FileUtils::FileUtils(MySQLDatabase &database) : db(database) {}

std::vector<std::string> FileUtils::splitPath(const std::string &path)
{
    std::vector<std::string> components;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '/'))
    {
        if (!item.empty())
        {
            components.push_back(item);
        }
    }

    return components;
}

int FileUtils::getDirectoryIdByPath(const std::string &path)
{
    std::vector<std::string> directories = splitPath(path);
    int parentId = 1;

    for (const auto &dirName : directories)
    {
        std::string query = "SELECT * FROM directories WHERE directory_name = '" + dirName +
                            "' AND parent_directory_id = " + std::to_string(parentId);
        DirectoryModel dirTemplate(0, 0, "", "", "");
        std::vector<DirectoryModel> directories = db.executeTemplateQuery(query, dirTemplate);
        if (directories.size() > 0)
        {
            parentId = directories.front().directory_id;
        }
        else
        {
            throw DirectoryNotFoundException(dirName);
        }
    }

    return parentId;
}