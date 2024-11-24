#ifndef ROUTER_H
#define ROUTER_H
#include <map>
#include "http_request_json.h"
#include "http_response_json.h"
#include "http_controller.h"
#include <functional>
extern std::map<std::string, std::function<HttpResponseJson(HttpRequestJson)>> get_routes;
extern std::map<std::string, std::function<HttpResponseJson(HttpRequestJson)>> post_routes;
void register_urls();
#endif