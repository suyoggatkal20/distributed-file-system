#include <map>
#include "http_request_json.h"
#include "http_response_json.h"
#include "http_controller.h"
#include <functional>
#include "router.h"

std::map<std::string, std::function<HttpResponseJson(HttpRequestJson)>> get_routes;
std::map<std::string, std::function<HttpResponseJson(HttpRequestJson)>> post_routes;

void register_urls()
{
    post_routes["/put"] = HttpController::put_block;
    post_routes["/get"] = HttpController::get_block;
}