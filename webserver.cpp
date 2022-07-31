#include "webserver.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "timer.h"
#include "threadpool.h"
#include "epoller.h"

constexpr int MAX_FD = 65536;

WebServer::WebServer(int port, int trigMode, int timeout, 
        bool optLinger, int threadNum)
    : m_nPort(port)
    , m_timeout(timeout)
    , m_bClosed(false)
    , m_listenFd(0)
    , m_bOpenLinger(optLinger)
    , m_listenEvent(0)
    , m_connectionEvent(0)
    , m_timer(new TimerManager())
    , m_threadPool(new ThreadPool(threadNum))
    , m_epoller(new Epoller)
    , m_users()
{	
    char* temp = getcwd(nullptr, 256);
    std::string tempStr(temp);
    tempStr += "/resources/";
    free(temp);
    HTTPConnection::userCount = 0;
    HTTPConnection::srcDir = tempStr;
    __initEventMode(trigMode);
    if (!__initSocket())
        m_bClosed = true;
}

WebServer::~WebServer()
{
    close(m_listenFd);
    m_bClosed = true;
}

void WebServer::start()
{
    int timeout = -1;
    std::cout << "================";
    std::cout << "Server Start";
    std::cout << "================" << std::endl;
    while (!m_bClosed)
    {
        if (timeout > 0)
        {
            timeout = m_timer->getNextHandle();
        }
        int eventCnt = m_epoller->wait(timeout);
        for (int i = 0; i < eventCnt; i++)
        {
            int fd = m_epoller->getEventFd(static_cast<size_t>(i));
            uint32_t events = m_epoller->getEvents(static_cast<size_t>(i));
            if (fd == m_listenFd)
            {
                __handleNewConn();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                __handleCloseConn(&m_users[fd]);
            }
            else if (events & EPOLLIN)
            {   
                __handleRead(&m_users[fd]);
            }
            else if (events & EPOLLOUT)
            {  
                __handleWrite(&m_users[fd]);
            }
            else
            {
                std::cout << "Unexpected event" << std::endl;
            }
        }
    }
    return;
}

void WebServer::__initEventMode(int trigMode)
{
    m_listenEvent = EPOLLRDHUP;
    m_connectionEvent = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        m_connectionEvent |= EPOLLET;
        break;
    case 2:
        m_listenEvent |= EPOLLET;
        break;
    case 3:
        m_listenEvent |= EPOLLET;
        m_connectionEvent |= EPOLLET;
        break;
    default:
        m_listenEvent |= EPOLLET;
        m_connectionEvent |= EPOLLET;
        break;
    }
    HTTPConnection::isET = (m_connectionEvent & EPOLLET);
    return;
}

bool WebServer::__initSocket()
{
    struct sockaddr_in addr;
    if (m_nPort > MAX_FD || m_nPort < 1024)
        return false;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_nPort);
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenFd < 0)
        return false;
    struct linger optLinger = {0};
    if (m_bOpenLinger)
    {
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }
    int ret;
    ret = setsockopt(m_listenFd, SOL_SOCKET, SO_LINGER, 
        &optLinger, sizeof(optLinger));
    if (ret < 0)
    {
        close(m_listenFd);
        return false;
    }
    int optVal = 1;
    ret = setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, 
        (const void*)&optVal, sizeof(int));
    if (ret < 0)
    {
        close(m_listenFd);
        return false;
    }
    ret = bind(m_listenFd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0)
    {
        close(m_listenFd);
        return false;
    }
    ret = listen(m_listenFd, 6);
    if (ret < 0)
    {
        close(m_listenFd);
        return false;
    }
    ret = m_epoller->addFd(m_listenFd, m_listenEvent | EPOLLIN);
    if (ret == 0)
    {
        close(m_listenFd);
        return false;
    }
    fcntl(m_listenFd, F_SETFL, fcntl(m_listenFd, F_GETFD, 0) | O_NONBLOCK);
    return true;
}

void WebServer::__handleNewConn()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do 
    {
        int fd = accept(m_listenFd, (struct sockaddr *)&addr, &len);
        if(fd <= 0) 
            return;
        else if(HTTPConnection::userCount >= MAX_FD) 
        {   
            const char* msg = "Server busy!";
            send(fd, msg, strlen(msg), 0);
            close(fd);
            std::cout << "Clients is full!" <<std::endl;
            return;
        }
        __addClientConn(fd, addr);
    } while(m_listenEvent & EPOLLET);
    return;
}

void WebServer::__handleCloseConn(HTTPConnection* client)
{
    m_epoller->delFd(client->getFd());
    client->closeHTTPConn();
    return;
}

void WebServer::__handleWrite(HTTPConnection* client)
{
    if (m_timeout > 0)
        m_timer->updateTimer(client->getFd(), m_timeout);
    auto onWrite = [this, client]()
    {   
        int writeErrorNo = 0;
        ssize_t ret = client->writeBuffer(&writeErrorNo);
        if (client->writeBytes() == 0)
        {
            if (client->isKeepAlive())
            {
                __onProcess(client);
                return;
            }
        }
        else if (ret < 0)
        {
            if (writeErrorNo == EAGAIN)
            {
                m_epoller->modFd(client->getFd(), m_connectionEvent | EPOLLOUT);
                return;
            }
        }
        __closeConn(client);
        return;
    };
    m_threadPool->addTask(onWrite);
    return;
}

void WebServer::__handleRead(HTTPConnection* client)
{   
    if (m_timeout > 0)
        m_timer->updateTimer(client->getFd(), m_timeout);
    auto onRead = [this, client]()
    {  
        int readErrorNo = 0;
        int ret = client->readBuffer(&readErrorNo);
        if (ret <= 0 && readErrorNo != EAGAIN)
        {
            __closeConn(client);
            return;
        }
        __onProcess(client);
        return;
    };
    m_threadPool->addTask(onRead);
    return;
}

void WebServer::__addClientConn(int fd, sockaddr_in addr)
{
    m_users[fd].initHTTPConn(fd, addr);
    auto closeTimeoutConn = [this, fd]()
    {
        __closeConn(&m_users[fd]);
        return;
    };
    if (m_timeout > 0)
        m_timer->addTimer(fd, m_timeout, closeTimeoutConn);
    m_epoller->addFd(fd, m_connectionEvent | EPOLLIN);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    return;
}

void WebServer::__closeConn(HTTPConnection* client)
{
    m_epoller->delFd(client->getFd());
    client->closeHTTPConn();
    return;
}

void WebServer::__onProcess(HTTPConnection* client)
{   
    bool ret = client->handleHTTPConn();
    if (ret)
    {
        m_epoller->modFd(client->getFd(), m_connectionEvent | EPOLLOUT);
    }
    else
    {
        m_epoller->modFd(client->getFd(), m_connectionEvent | EPOLLIN);
    }
    return;
}
