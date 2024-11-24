#include <iostream>
#include "json.hpp"
#include "json_helper.h"
using json = nlohmann::json;
using namespace std;

json convertToJson(string input)
{
    json j = json::parse(input);
    return j;
}
string convertToString(json j)
{
    string jsonString = j.dump();
    return jsonString;
}