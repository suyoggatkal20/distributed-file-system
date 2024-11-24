#ifndef HTTP_REQUEST
#define HTTP_REQUEST
#include <string>
#include <map>
#include "http_response_json.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

class HttpRequestJson
{
public:
    string method;
    string path;
    map<string, string> headers;
    string body;
    map<string, string> query_params;
    map<string, string> path_params;
    long request_id;
    int clientSocket;
    int threadid;
    
    string getRequestString() const;
    const char *getRequestBytes() const;
    json getJsonBody() const;
    void setJsonBody(json json);
};

// Declare the thread-local variable

#endif
