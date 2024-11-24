#include "FileUtils.h"
#include <stdexcept>
#include "custom_exceptions.h"
#include <string>
#include <algorithm>
#include "models.h"
#include "dbhelper.h"
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <sys/types.h>

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
        parentId = directories.front().directory_id;
    }

    return parentId;
}

std::string FileUtils::findPathToNumberInNaryTree(int target, int n)
{
    std::vector<int> path;
    int targetIndex = target; // Since we're starting from index 0, target is the index of the number.

    // Step 1: Trace the path from the target index back to the root using the parent index
    while (targetIndex >= 0)
    {
        path.push_back(targetIndex); // Add the current node to the path
        if (targetIndex == 0)
            break;                           // Root node reached
        targetIndex = (targetIndex - 1) / n; // Move to the parent node
    }

    // Step 2: Reverse the path to show it from root to target
    std::reverse(path.begin(), path.end());
    std::string string_path;
    for (int &index : path)
    {
        string_path = string_path + "/dir" + std::to_string(index);
        index++;
    }

    return string_path;
}

void FileUtils::createDirectories(const std::string path)
{
    if (mkdir(path.c_str(), 0777) == 0)
    {
        std::cout << "Directories created: " << path << std::endl;
    }
    else if (errno == EEXIST)
    {
        std::cout << "Directories already exist: " << path << std::endl;
    }
    else
    {
        perror("Error creating directories: ");
    }
}

bool FileUtils::writeToFile(std::ofstream &file, const char *buffer, size_t bufferSize)
{
    file.write(buffer, bufferSize);

    // Check for write errors
    if (!file)
    {
        std::cerr << "Failed to write." << std::endl;
        return false;
    }
    return true;
}

std::string FileUtils::getForattedString(ChunkReplicasSlaveModel chunkReplicasSlaveModel)
{

    std::time_t now = std::time(nullptr);
    std::tm* utc_tm = std::gmtime(&now);
    std::ostringstream oss;
    oss << std::put_time(utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    std::string time = oss.str();

    std::stringstream formatted;
    formatted << "000000000" << std::endl;
    formatted << std::setw(29) << std::right << "REPLICA_ID"
              << std::setw(30) << std::right << chunkReplicasSlaveModel.replica_id << std::endl;

    formatted << std::setw(29) << std::right << "CHUNK_ID"
              << std::setw(30) << std::right << chunkReplicasSlaveModel.chunk_id << std::endl;

    formatted << std::setw(29) << std::right << "CHUNK_ORDER"
              << std::setw(30) << std::right << chunkReplicasSlaveModel.chunk_order << std::endl;

    formatted << std::setw(29) << std::right << "CHUNK_SIZE"
              << std::setw(30) << std::right << chunkReplicasSlaveModel.chunk_size << std::endl;

    formatted << std::setw(29) << std::right << "FILE_ID"
              << std::setw(30) << std::right << chunkReplicasSlaveModel.file_id << std::endl;

    formatted << std::setw(29) << std::right << "CREATED_AT"
              << std::setw(30) << std::right << time << std::endl;

    formatted << std::setw(29) << std::right << "CHUNK_HASH"
              << std::setw(30) << std::right << chunkReplicasSlaveModel.data_hash << std::endl;

    formatted << std::setw(29) << std::right << "FILE_PATH"
              << std::setw(2540) << std::right << chunkReplicasSlaveModel.file_path << std::endl;
    return formatted.str();
}