#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// Function to load properties from a file
bool Properties::loadProperties(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        // Ignore comments and empty lines
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value))
        {
            // Trim whitespace
            key.erase(key.find_last_not_of(" \n\r\t") + 1);
            value.erase(0, value.find_first_not_of(" \n\r\t"));
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            properties_[key] = value;
        }
    }

    file.close();
    return true;
}

// Function to get a property value by key
std::string Properties::getString(const std::string &key) const
{
    std::string new_key = key;
    std::transform(new_key.begin(), new_key.end(), new_key.begin(), ::tolower);
    auto it = properties_.find(new_key);
    if (it != properties_.end())
    {
        return it->second;
    }

    throw std::runtime_error("Can not find key in properties. key: " + new_key);
}

int Properties::getInt(const std::string &key) const
{
    std::string str_value = Properties::getString(key);
    int value;
    try
    {
        value = std::stoi(str_value);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Can not convert property value to Long key:" + key + " value: " + str_value);
    }
    return value;
}

long Properties::getLong(const std::string &key) const
{
    std::string str_value = Properties::getString(key);
    long value;
    try
    {
        value = std::stol(str_value);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Can not convert property value to Long key:" + key + " value: " + str_value);
    }

    return value;
}

long long Properties::getLongLong(const std::string &key) const
{
    std::string str_value = Properties::getString(key);
    long long value;
    try
    {
        value = std::atoll(str_value.c_str());
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Can not convert property value to Long key:" + key + " value: " + str_value);
    }

    return value;
}

bool Properties::getBool(const std::string &key) const
{
    bool res;
    std::string str_value = Properties::getString(key);
    std::transform(str_value.begin(), str_value.end(), str_value.begin(), ::tolower);
    if (str_value == "true" || str_value == "1")
    {
        res = true;
    }
    else if (str_value == "false" || str_value == "0")
    {
        res = false;
    }
    else
    {
        throw std::runtime_error("Can not convert property value to bool. key:" + key + " value: " + str_value);
    }
    return res;
}