#include <string>
#include <iostream>
#include <map>
#include "http_response_json.h"
#include "http_helper.h"
#include "json_helper.h"
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

HttpResponseJson::HttpResponseJson()
{
    headers = {
        {"Content-Type", "application/json"},
        {"Cache-Control", "no-cache"},
        {"Expires", "Wed, 21 Oct 2023 07:28:00 GMT"},
        {"Last-Modified", "Wed, 21 Oct 2023 07:28:00 GMT"}};
}

string HttpResponseJson::getResponceString() const
{
    std::string response = "HTTP/1.1 " + std::to_string(status_code) + " " + getHttpStatusMessage(status_code) + "\r\n";
    for (const auto &header : headers)
    {
        response += header.first + ": " + header.second + "\r\n";
    }
    response += "\r\n\r\n";
    response += body;
    return response;
};
const char *HttpResponseJson::getResponceBytes() const
{
    return getResponceString().c_str();
};
json HttpResponseJson::getJsonBody() const
{
    return convertToJson(body);
};

void HttpResponseJson::setJsonBody(json json1)
{
    body = convertToString(json1);
};

void HttpResponseJson::addHeader(string key, string value){
    headers[key]=value;
}
