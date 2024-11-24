// multi_threaded_server.cpp

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include "http_server_helper_bin.h"
#include "config.h"
#include "router.h"
using namespace std;
Properties config;
int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    config.loadProperties("config.properties");
    if (argc == 2)
    {
        string port = argv[1];
        config.properties_["port"] = port;
    }

    register_urls();
    HttpServerHelperBin httpServerHelper;
    httpServerHelper.runserver();
    return 0;
}
