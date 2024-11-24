#ifndef HTTP_CONTROLLER
#define HTTP_CONTROLLER
#include <string>
#include <iostream>
#include <map>
#include "http_response_json.h"
#include "http_request_json.h"
// #include "context.h"
using namespace std;

class HttpController
{
public:
    static HttpResponseJson put_block(HttpRequestJson request);
    static HttpResponseJson get_block(HttpRequestJson request);
};

#endif // MESSAGE_H