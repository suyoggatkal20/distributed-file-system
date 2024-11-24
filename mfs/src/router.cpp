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
    post_routes["/ls"] = HttpController::listing;
    post_routes["/mkdir"] = HttpController::mkdir;
    post_routes["/put/req"] = HttpController::put_file_req;
    post_routes["/get/req"] = HttpController::get_file_req;
}