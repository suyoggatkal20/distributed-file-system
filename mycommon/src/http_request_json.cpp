#include "http_request_json.h"
#include "json.hpp"
#include "json_helper.h"
using json = nlohmann::json;

string HttpRequestJson::getRequestString() const
{
    std::string response = method + " " + path + " HTTP/1.1\r\n";
    for (const auto &header : headers)
    {
        response += header.first + ": " + header.second + "\r\n";
    }
    response += "\r\n\r\n";
    if (body.empty() || body == "")
    {
        response += body;
    }

    return response;
};
const char *HttpRequestJson::getRequestBytes() const
{
    return getRequestString().c_str();
};

json HttpRequestJson::getJsonBody() const
{
    return convertToJson(body);
};

void HttpRequestJson::setJsonBody(json json)
{
    body = convertToString(json);
};