#ifndef PROPERTIES_LOADER_H
#define PROPERTIES_LOADER_H

#include <string>
#include <map>

class Properties
{
public:
    bool loadProperties(const std::string &filename);
    std::string getString(const std::string &key) const;
    int getInt(const std::string &key) const;
    bool getBool(const std::string &key) const;
    long getLong(const std::string &key) const;
    long long getLongLong(const std::string &key) const;

    std::map<std::string, std::string> properties_; // To store key-value pairs
};
extern Properties config;
#endif // PROPERTIES_LOADER_H
