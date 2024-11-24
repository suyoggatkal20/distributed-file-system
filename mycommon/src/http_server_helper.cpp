#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <functional>
#include "config.h"
#include "http_request_json.h"
#include "http_response_json.h"
#include "http_server_helper.h"
#include "http_helper.h"

HttpRequestJson HttpServerHelper::parseHttpRequest(const string &request, HttpRequestJson httpRequest)
{
    istringstream requestStream(request);
    string line;

    // Parse request line
    getline(requestStream, line);
    istringstream requestLine(line);
    requestLine >> httpRequest.method; // GET, POST, etc.
    requestLine >> httpRequest.path;
    // Parse headers
    while (getline(requestStream, line) && line != "\r")
    {
        size_t delimiterPos = line.find(": ");
        if (delimiterPos != string::npos)
        {
            string headerName = line.substr(0, delimiterPos);
            string headerValue = line.substr(delimiterPos + 2);
            httpRequest.headers[headerName] = headerValue;
        }
    }

    // Check for body (only in POST requests, for example)
    if (httpRequest.headers.find("Content-Length") != httpRequest.headers.end())
    {
        int contentLength = stoi(httpRequest.headers["Content-Length"]);
        httpRequest.body.resize(contentLength);
        requestStream.read(&httpRequest.body[0], contentLength);
    }

    // Parse query parameters
    size_t questionMarkPos = httpRequest.path.find('?');
    if (questionMarkPos != string::npos)
    {
        string queryString = httpRequest.path.substr(questionMarkPos + 1);
        httpRequest.path = httpRequest.path.substr(0, questionMarkPos);

        istringstream queryStream(queryString);
        string queryParam;
        while (getline(queryStream, queryParam, '&'))
        {
            size_t equalPos = queryParam.find('=');
            if (equalPos != string::npos)
            {
                string key = queryParam.substr(0, equalPos);
                string value = queryParam.substr(equalPos + 1);
                httpRequest.query_params[key] = value;
            }
        }
    }

    return httpRequest;
}

// Function to handle each client connection
void HttpServerHelper::handleClient(int socket, int request_id)
{
    HttpRequestJson httpRequest;
    char buffer[1024] = {0};
    httpRequest.clientSocket = socket;
    httpRequest.request_id = request_id;

    int read_bytes = read(httpRequest.clientSocket, buffer, sizeof(buffer));
    buffer[read_bytes] = '\0';
    if (config.getBool("log-full-req"))
    {
        cout << "Full request:\n"
             << buffer << "\n\n";
    }
    string request(buffer);
    httpRequest = parseHttpRequest(request, httpRequest);
    if (config.getBool("log-partial-req"))
    {
        cout << httpRequest.method << " " << httpRequest.path << endl;
    }
    HttpResponseJson responce;
    if (httpRequest.method == "GET")
    {
        try
        {
            responce = get_routes.at(httpRequest.path)(httpRequest);
        }
        catch (const std::exception &e)
        {
            responce.status_code = 404;
            responce.body = "{\"Error message\":\"Not found\"}";
            std::cerr << e.what() << '\n';
        }
    }
    else if (httpRequest.method == "POST")
    {
        try
        {
            responce = post_routes.at(httpRequest.path)(httpRequest);
        }
        catch (const std::exception &e)
        {
            responce.status_code = 404;
            responce.body = "{\"Error message\":\"Not found\"}";
            std::cerr << e.what() << '\n';
        }
    }
    else
    {
        responce.status_code = 404;
        responce.body = "{\"Error message\":\"Not found\"}";
    }
    cout << "Responce: " << responce.status_code << " - " << getHttpStatusMessage(responce.status_code) << endl;
    cout << endl
         << endl
         << endl;
    send(httpRequest.clientSocket, responce.getResponceString().c_str(), responce.getResponceString().size(), 0);
    close(httpRequest.clientSocket);
}

void handleClient1(int socket_addr, int request_id)
{
    HttpServerHelper httpServerHelper;
    httpServerHelper.handleClient(socket_addr, request_id);
}

int HttpServerHelper::runserver()
{
    setbuf(stdout, NULL);
    // long request_seq = 0;
    int server_fd;
    int new_socket, request_id;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        cerr << "Socket failed" << endl;
        return -1;
    }

    // Attach socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        cerr << "setsockopt failed" << endl;
        return -1;
    }

    address.sin_family = AF_INET;                    // IPv4
    address.sin_addr.s_addr = INADDR_ANY;            // Bind to all interfaces
    address.sin_port = htons(config.getInt("PORT")); // Convert to network byte order

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        cerr << "Bind failed" << endl;
        return -1;
    }

    // Start listening for incoming connections
    if (listen(server_fd, config.getInt("MAX_CONNECTION")) < 0)
    {
        cerr << "Listen failed" << endl;
        return -1;
    }

    cout << "Server is listening on port " << config.getInt("PORT") << endl;

    // Main loop to accept and handle client connections

    while (true)
    {
        // new_socket = (int *)malloc(sizeof(int));
        // request_id = (int *)malloc(sizeof(int));
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            cerr << "Accept failed" << endl;
            continue; // Go to next iteration if accept fails
        }
        request_id = (request_id) + 1;
        // Create a new thread to handle the client

        // auto handler = std::bind(&HttpServerHelper::handleClient, &httpServerHelper);
        // thread(handler, new_socket, request_id).detach();
        std::thread t(handleClient1, new_socket, request_id);
        t.detach();
    }
    return 0;
}
