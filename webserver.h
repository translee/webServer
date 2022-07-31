/**
 * @class   WebServer
 * @author  github.com/translee
 * @date    2022/06/14
 * @brief   
 */

#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <arpa/inet.h>
#include "HTTPconnection.h"
class TimerManager;
class ThreadPool;
class Epoller;

class WebServer
{
public:
    WebServer(int port, int trigMode, int timeout, 
        bool optLinger, int threadNum);
    ~WebServer();
    void start();
private:
    void __initEventMode(int trigMode);
    bool __initSocket();
    void __handleNewConn();
    void __handleCloseConn(HTTPConnection* client);
    void __handleWrite(HTTPConnection* client);
    void __handleRead(HTTPConnection* client);
    void __addClientConn(int fd, sockaddr_in addr);
    void __closeConn(HTTPConnection* client);
    void __onProcess(HTTPConnection* client);
private:
    int m_nPort;
    int m_timeout;
    bool m_bClosed;
    int m_listenFd;
    bool m_bOpenLinger;
    uint32_t m_listenEvent;
    uint32_t m_connectionEvent;
    std::unique_ptr<TimerManager> m_timer;
    std::unique_ptr<ThreadPool> m_threadPool;
    std::unique_ptr<Epoller> m_epoller;
    std::unordered_map<int, HTTPConnection> m_users;
};
