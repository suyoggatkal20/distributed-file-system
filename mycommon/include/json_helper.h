#ifndef JSON_HELPER
#define JSON_HELPER

#include <iostream>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;
json convertToJson(string input);
string convertToString(json j);
#endif