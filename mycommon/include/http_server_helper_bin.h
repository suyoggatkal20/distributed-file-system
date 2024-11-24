#ifndef HTTP_SERVER_HELPER_BIN
#define HTTP_SERVER_HELPER_BIN

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

class HttpServerHelperBin
{
private:
    // void parseHttpRequest(char *buffer, size_t buffer_size, HttpRequestJson &request);
    void parseHeaders(string request, HttpRequestJson &httpRequest);

public:
    void handleClient(int socket, int request_id);
    int runserver();
};


#endif // MESSAGE_H