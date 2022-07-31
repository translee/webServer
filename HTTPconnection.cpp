#include "HTTPconnection.h"
#include <cassert>
#include <iostream>
#include "unistd.h"

std::string HTTPConnection::srcDir;
std::atomic<int> HTTPConnection::userCount(0);
bool HTTPConnection::isET = true;

HTTPConnection::HTTPConnection()
    : m_fd(-1)
    , m_addr({0})
    , m_iov({{0}, {0}})
    , m_iovCnt(1)
    , m_bClosed(true)
    , m_readBuffer()
    , m_writeBuffer()
    , m_request()
    , m_response()
{
}

HTTPConnection::~HTTPConnection() 
{ 
    closeHTTPConn(); 
}

void HTTPConnection::initHTTPConn(int fd, const sockaddr_in& addr) 
{
    assert(fd > 0);
    userCount++;
    m_addr = addr;
    m_fd = fd;
    m_writeBuffer.init();
    m_readBuffer.init();
    m_bClosed = false;
    return;
}

void HTTPConnection::closeHTTPConn() 
{
    m_response.unMapFile();
    if(m_bClosed == false)
    {
        m_bClosed = true; 
        userCount--;
        close(m_fd);
    }
    return;
}

ssize_t HTTPConnection::readBuffer(int* saveErrno) 
{
    ssize_t len = -1;
    do {
        len = m_readBuffer.readFd(m_fd, saveErrno);
        //std::cout<<m_fd<<" read bytes:"<<len<<std::endl;
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HTTPConnection::writeBuffer(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(m_fd, m_iov, m_iovCnt);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(m_iov[0].iov_len + m_iov[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > m_iov[0].iov_len) {
            m_iov[1].iov_base = (uint8_t*) m_iov[1].iov_base + (len - m_iov[0].iov_len);
            m_iov[1].iov_len -= (len - m_iov[0].iov_len);
            if(m_iov[0].iov_len) {
                m_writeBuffer.init();
                m_iov[0].iov_len = 0;
            }
        }
        else {
            m_iov[0].iov_base = (uint8_t*)m_iov[0].iov_base + len; 
            m_iov[0].iov_len -= len; 
            m_writeBuffer.updateReadPos(len);
        }
    } while(isET || writeBytes() > 10240);
    return len;
}

bool HTTPConnection::handleHTTPConn() {
    m_request.init();
    if(m_readBuffer.readableBytes() <= 0) {
        //std::cout<<"readBuffer is empty!"<<std::endl;
        return false;
    }
    else if(m_request.parse(m_readBuffer)) {
        m_response.init(srcDir, m_request.path(), m_request.isKeepAlive(), 200);
    }else {
        std::cout<<"400!"<<std::endl;
        //m_readBuffer.printContent();
        m_response.init(srcDir, m_request.path(), false, 400);
    }

    m_response.makeResponse(m_writeBuffer);
    /* 响应头 */
    m_iov[0].iov_base = m_writeBuffer.getReadP();
    m_iov[0].iov_len = m_writeBuffer.readableBytes();
    m_iovCnt = 1;

    /* 文件 */
    if(m_response.fileLen() > 0  && m_response.getFile()) {
        m_iov[1].iov_base = m_response.getFile();
        m_iov[1].iov_len = m_response.fileLen();
        m_iovCnt = 2;
    }
    return true;
}
