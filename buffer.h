/**
 * @class   Buffer
 * @author  github.com/translee
 * @date    2022/06/06
 * @brief   
 */

#pragma once
#include <vector>
#include <string>
#include <atomic>

class Buffer
{
public:
    Buffer(int initBufferSize=1024);
    ~Buffer() = default;
    void init();
    void append(const char* str,size_t len);
    void append(const std::string& str);

    ssize_t readFd(int fd, int* err);
    ssize_t writeFd(int fd, int* err);

    size_t writeableBytes() const { return m_vecBuff.size()-m_writePos; }
    size_t readableBytes() const { return m_writePos-m_readPos; }
    size_t readBytes() const { return m_readPos; }
    
    const char* getReadCP() const { return __getBeginCP() + m_readPos; }
    const char* getWriteCP() const { return __getBeginCP() + m_writePos; }
    char* getReadP() { return __getBeginP() + m_readPos; }
    char* getWriteP() { return __getBeginP() + m_writePos; }

    void updateReadPos(size_t len);
    void updateReadPosUntil(const char* end);
private:
    char* __getBeginP() { return m_vecBuff.data(); }
    const char* __getBeginCP() const { return m_vecBuff.data(); }
    void __allocateSpace(size_t len);
private:
    std::vector<char> m_vecBuff;
    std::atomic<size_t> m_readPos;
    std::atomic<size_t> m_writePos;
};