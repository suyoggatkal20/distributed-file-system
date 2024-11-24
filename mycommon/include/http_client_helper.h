#ifndef HTTP_CLIENT_HELPER
#define HTTP_CLIENT_HELPER
#include <fstream>
#include <map>
#include "http_response_json.h"
#include "http_request_json.h"
using namespace std;
class HttpClientHelper
{
    // size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* userp);
public:
    HttpResponseJson httpGet(string url, HttpRequestJson request);
    HttpResponseJson httpPost(string url, HttpRequestJson request);
};
#endif