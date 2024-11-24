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
#include "http_server_helper.h"
#include "config.h"
#include "router.h"
using namespace std;
Properties config;
int main()
{
    setbuf(stdout,NULL);
    config.loadProperties("config.properties");
    register_urls();
    HttpServerHelper httpServerHelper;
    httpServerHelper.runserver();
    return 0;
}
