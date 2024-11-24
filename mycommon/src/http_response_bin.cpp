#include <string>
#include <iostream>
#include <map>
#include "http_response_bin.h"
#include "http_helper.h"
#include "json_helper.h"
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

// HttpResponseBin::HttpResponseBin()
// {
//     headers = {
//         {"Content-Type", "application/json"},
//         {"Cache-Control", "no-cache"},
//         {"Expires", "Wed, 21 Oct 2023 07:28:00 GMT"},
//         {"Last-Modified", "Wed, 21 Oct 2023 07:28:00 GMT"}};
// }
