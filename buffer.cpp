#include "buffer.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <sys/uio.h>

Buffer::Buffer(int initBuffersize)
    : m_vecBuff(initBuffersize)
    , m_readPos(0)
    , m_writePos(0)
{}

void Buffer::updateReadPos(size_t len)
{
    assert(len<=readableBytes());
    m_readPos+=len;
    return;
}

void Buffer::updateReadPosUntil(const char* end)
{
    //assert(end>=curReadPtr());
    updateReadPos(end-getReadCP());
    return;
}

void Buffer::init()
{
    bzero(m_vecBuff.data(), m_vecBuff.size());
    m_readPos = 0;
    m_writePos = 0;
    return;
}

void Buffer::__allocateSpace(size_t len)
{
    if(writeableBytes() + readBytes() < len)
    {
        m_vecBuff.resize(m_writePos + len + 1);
    }
    else
    {
        size_t readable = readableBytes();
        std::copy(getReadCP(), getWriteCP(), __getBeginP());
        m_readPos = 0;
        m_writePos = readable;
        assert(readable==readableBytes());
    }
    return;
}

void Buffer::append(const char* str,size_t len)
{
    assert(str);
    if(writeableBytes() < len)
    {
        __allocateSpace(len);
    }
    std::copy(str, str+len, getWriteP());
    m_writePos += len;
    return;
}

void Buffer::append(const std::string& str)
{
    append(str.data(),str.length());
    return;
}

ssize_t Buffer::readFd(int fd,int* err)
{   
    // temp buff
    char buff[65535];
    const size_t writable = writeableBytes();
    struct iovec iov[2];
    iov[0].iov_base = getWriteP();
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if(len < 0)
    {
        //std::cout<<"从fd读取数据失败！"<<std::endl;
        *err = errno;
    }
    else if(static_cast<size_t>(len) <= writable)
    {
        m_writePos += len;
    }
    else
    {
        m_writePos = m_vecBuff.size();
        append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::writeFd(int fd,int* err)
{
    ssize_t len = write(fd, getReadCP(), readableBytes());
    if(len < 0)
    {
        //std::cout<<"往fd写入数据失败！"<<std::endl;
        *err = errno;
        return len;
    }
    m_readPos += len;
    return len;
}