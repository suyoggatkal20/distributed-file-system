#ifndef HTTP_RESPONSE
#define HTTP_RESPONSE
#include <string>
#include <iostream>
#include <map>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

class HttpResponseJson
{

public:
    HttpResponseJson();
    map<string, string> headers;
    string body;
    int status_code;
    void addHeader(string key, string value);
    string getResponceString() const;
    const char *getResponceBytes() const;
    json getJsonBody() const;
    void setJsonBody(json json);
};

#endif // MESSAGE_H