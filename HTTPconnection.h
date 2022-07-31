/**
 * @class   HTTPConnection
 * @author  github.com/translee
 * @date    2022/06/11
 * @brief   
 */

#pragma once
#include <string>
#include <atomic>
#include <arpa/inet.h>
#include <sys/uio.h>
#include "buffer.h"
#include "HTTPrequest.h"
#include "HTTPresponse.h"

class HTTPConnection{
public:
    HTTPConnection();
    ~HTTPConnection();
    void initHTTPConn(int socketFd,const sockaddr_in& addr);
    void closeHTTPConn();
    bool handleHTTPConn();
    ssize_t readBuffer(int* saveErrno);
    ssize_t writeBuffer(int* saveErrno);
    int writeBytes() { return m_iov[0].iov_len + m_iov[1].iov_len; }
    std::string getIP() const { return std::string(inet_ntoa(m_addr.sin_addr)); }
    unsigned short getPort() const { return m_addr.sin_port; }
    int getFd() const { return m_fd; }
    sockaddr_in getAddr() const { return m_addr; }
    bool isKeepAlive() const { return m_request.isKeepAlive(); }
public:
    static bool isET;
    static std::string srcDir;
    static std::atomic<int> userCount;
private:
    int m_fd;
    struct sockaddr_in m_addr;
    bool m_bClosed;
    int m_iovCnt;
    struct iovec m_iov[2];
    Buffer m_readBuffer;
    Buffer m_writeBuffer;
    HTTPRequest m_request;    
    HTTPResponse m_response;
};