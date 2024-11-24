#include <iostream>
#include <string>
#include <map>
#include <curl/curl.h>
#include "http_client_helper.h"
#include "http_request_json.h"
#include "http_response_json.h"

using namespace std;
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, string *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

// Function to perform a GET request
HttpResponseJson HttpClientHelper::httpGet(string url, HttpRequestJson request)
{
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (res != CURLE_OK)
        {
            cerr << "Failed to set URL: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            throw std::runtime_error("Request failed: invalid URL");
        }

        struct curl_slist *headers_final = NULL;
        for (const auto &pair : request.headers)
        {
            string header = pair.first + ": " + pair.second;
            headers_final = curl_slist_append(headers_final, header.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_final);
        if (res != CURLE_OK)
        {
            cerr << "Failed to set Headers: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            throw std::runtime_error("Request failed: invalid headers");
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        if (res != CURLE_OK)
        {
            cerr << "Failed to set callback: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            throw std::runtime_error("Request failed: invalid callback");
        }
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        if (res != CURLE_OK)
        {
            cerr << "Failed to set Buffer: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            throw std::runtime_error("Request failed: invalid Buffer");
        }
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            cerr << "GET request failed: " << curl_easy_strerror(res) << endl;
        }
        else
        {
            cout << "Response data: " << readBuffer << endl;
        }
        curl_slist_free_all(headers_final);
        curl_easy_cleanup(curl);
    }
    HttpResponseJson httpResponce;
    return httpResponce;
}

// Function to perform a POST request
HttpResponseJson HttpClientHelper::httpPost(string url, HttpRequestJson request)
{
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (res != CURLE_OK)
        {
            cerr << "Failed to set URL: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            throw std::runtime_error("Request failed: invalid URL");
        }

        struct curl_slist *headers_final = NULL;
        for (const auto &pair : request.headers)
        {
            string header = pair.first + ": " + pair.second;
            headers_final = curl_slist_append(headers_final, header.c_str());
        }
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_final);
        if (res != CURLE_OK)
        {
            cerr << "Failed to set Headers: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            throw std::runtime_error("Request failed: invalid headers");
        }
        printf("my body: %s \n", request.body.c_str());
        res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.c_str());
        if (res != CURLE_OK)
        {
            cerr << "Failed to set body: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            throw std::runtime_error("Request failed: invalid body");
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        if (res != CURLE_OK)
        {
            cerr << "Failed to set callback: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            throw std::runtime_error("Request failed: invalid callback");
        }
        res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        if (res != CURLE_OK)
        {
            cerr << "Failed to set Buffer: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            throw std::runtime_error("Request failed: invalid Buffer");
        }
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            cerr << "POST request failed: " << curl_easy_strerror(res) << endl;
        }
        else
        {
            cout << "Response data: " << readBuffer << endl;
        }
        curl_slist_free_all(headers_final);
        curl_easy_cleanup(curl);
    }
    HttpResponseJson httpResponce;
    return httpResponce;
}
