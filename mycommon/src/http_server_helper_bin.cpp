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
#include "http_request_bin.h"
#include "http_response_bin.h"
#include "http_server_helper_bin.h"
#include "http_helper.h"
#include <iostream>
#include <string>
#define HEADER_BUFFERSIZE 1000
#define BODY_BUFFERSIZE 10000

// HttpRequestJson parseHttpRequest(char buffer[], size_t buffer_size, HttpRequestJson request);

void HttpServerHelperBin::handleClient(int socket, int request_id)
{
    HttpRequestJson httpRequest;
    char buffer[HEADER_BUFFERSIZE];
    httpRequest.clientSocket = socket;
    httpRequest.request_id = request_id;
    size_t index = 0;
    char char1 = 0, char2 = 0, char3 = 0, char4 = 0;
    HttpResponseJson response;
    int flag;
    while (index < HEADER_BUFFERSIZE)
    {
        char1 = char2;
        char2 = char3;
        char3 = char4;
        flag = read(httpRequest.clientSocket, &char4, 1);
        if (!flag)
        {
            break;
        }
        buffer[index] = char4;
        if (char1 == '\r' && char2 == '\n' && char3 == '\r' && char4 == '\n')
        {
            flag = -1;
            break;
        }
        index++;
    }
    buffer[++index] = '\0';
    if (flag != -1)
    {
        response.addHeader("Content-Type", "META/JSON");
        response.addHeader("status", "500");
        response.addHeader("status-msg", "Invalid Request");
        send(httpRequest.clientSocket, response.getResponceString().c_str(), response.getResponceString().size(), 0);
        close(httpRequest.clientSocket);
        return;
    }

    std::string header_string(buffer);
    parseHeaders(header_string, httpRequest);

    if (httpRequest.headers["Content-Type"] == "META/JSON")
    {
        char body_buffer[BODY_BUFFERSIZE];
        size_t read_bytes = read(httpRequest.clientSocket, body_buffer, BODY_BUFFERSIZE - 1);
        if (read_bytes == BODY_BUFFERSIZE - 1)
        {
            response.addHeader("Content-Type", "META/JSON");
            response.addHeader("status", "500");
            response.addHeader("status-msg", "Too large  json data");
            send(httpRequest.clientSocket, response.getResponceString().c_str(), response.getResponceString().size(), 0);
            close(httpRequest.clientSocket);
            return;
        }
        body_buffer[read_bytes] = '\0';
        std::string body_string(buffer);
        httpRequest.body = body_string;
    }

    if (config.getBool("log-full-req"))
    {
        cout << header_string << endl;
        if (httpRequest.headers["Content-Type"] == "META/JSON")
        {
            cout << httpRequest.body << endl;
        }
    }
    
    // parseHttpRequest(buffer, read_bytes, httpRequest);
    if (config.getBool("log-partial-req"))
    {
        cout << httpRequest.method << " " << httpRequest.path << endl;
    }
    if (httpRequest.method == "GET")
    {
        try
        {
            response = get_routes.at(httpRequest.path)(httpRequest);
        }
        catch (const std::exception &e)
        {
            response.status_code = 404;
            response.body = "{\"Error message\":\"Not found\"}";
            std::cerr << e.what() << '\n';
        }
    }
    else if (httpRequest.method == "POST")
    {
        try
        {
            response = post_routes.at(httpRequest.path)(httpRequest);
        }
        catch (const std::exception &e)
        {
            response.status_code = 404;
            response.body = "{\"Error message\":\"Not found\"}";
            std::cerr << e.what() << '\n';
        }
    }
    else
    {
        response.status_code = 404;
        response.body = "{\"Error message\":\"Not found\"}";
    }
    if (response.status_code != -1)
    {
        cout << "response: " << response.status_code << " - " << getHttpStatusMessage(response.status_code) << endl;
        cout << endl
             << endl
             << endl;
        send(httpRequest.clientSocket, response.getResponceString().c_str(), response.getResponceString().size(), 0);
        close(httpRequest.clientSocket);
    }
}

void handleClient1(int socket_addr, int request_id)
{
    HttpServerHelperBin httpServerHelper;
    httpServerHelper.handleClient(socket_addr, request_id);
}

int HttpServerHelperBin::runserver()
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

// void HttpServerHelperBin::parseHttpRequest(char *buffer, size_t buffer_size, HttpRequestJson &request)
// {
//     char *ptr = buffer;
//     request.current_buffer_data_size = buffer_size;
//     char *header_end = NULL;
//     while (ptr < buffer + buffer_size - 3)
//     {
//         if (ptr[0] == '\r' && ptr[1] == '\n' && ptr[2] == '\r' && ptr[3] == '\n')
//         {
//             header_end = ptr;
//             break;
//         }
//         ++ptr;
//     }

//     if (header_end)
//     {
//         request.data_start_pos = header_end + 4 - buffer;
//         buffer[request.data_start_pos - 1] = '\0';
//         std::string header_string(buffer);
//         header_string += "\n";
//         parseHeaders(header_string, request);
//     }
//     else
//     {

//         throw std::runtime_error("Invalid Request");
//     }
// }
void HttpServerHelperBin::parseHeaders(string request, HttpRequestJson &httpRequest)
{
    istringstream requestStream(request);
    string line;
    cout << "headers in parse headers: \n"
         << request << endl;
    // Parse request line
    getline(requestStream, line);
    istringstream requestLine(line);
    requestLine >> httpRequest.method; // GET, POST, etc.
    requestLine >> httpRequest.path;
    // Parse headers
    while (getline(requestStream, line) && line != "\r")
    {
        if (line.back() == '\r')
            line.pop_back();
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
}
