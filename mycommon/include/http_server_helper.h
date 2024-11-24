#ifndef HTTP_SERVER_HELPER
#define HTTP_SERVER_HELPER

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include "config.h"
#include "http_request_json.h"
#include "http_response_json.h"
#include <functional>

extern std::map<std::string, std::function<HttpResponseJson(HttpRequestJson)>> get_routes;
extern std::map<std::string, std::function<HttpResponseJson(HttpRequestJson)>> post_routes;

class HttpServerHelper
{
private:
    HttpRequestJson parseHttpRequest(const string &request, HttpRequestJson httpRequest);

public:
    void handleClient(int socket, int request_id);
    int runserver();
};

#endif // MESSAGE_H