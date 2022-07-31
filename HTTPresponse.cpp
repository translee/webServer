#include "HTTPresponse.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "buffer.h"

const std::unordered_map<std::string, std::string> HTTPResponse::SUFFIX_TYPE = 
{
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HTTPResponse::CODE_STATUS =
{
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbbidden"},
    {404, "Not Found"},
};

HTTPResponse::HTTPResponse()
    : m_nCode(-1)
    , m_bKeepAlive(false)
    , m_path()
    , m_srcDir()
    , m_fileStat{0}
    , m_fileAddr(nullptr)
{
}

HTTPResponse::~HTTPResponse()
{
    unMapFile();
}

void HTTPResponse::init(const std::string& srcDir, const std::string& path, 
        bool isKeepAlive, int code)
{   
    if (nullptr != m_fileAddr)
        unMapFile();
    m_nCode = code;
    m_bKeepAlive = isKeepAlive;
    m_path = path;
    m_srcDir = srcDir;
    m_fileAddr = nullptr;
    m_fileStat = {0};
    return;
}

void HTTPResponse::makeResponse(Buffer& buff)
{
    if(stat((m_srcDir + m_path).c_str(), &m_fileStat) != 0 || 
        S_ISDIR(m_fileStat.st_mode))
        m_nCode = 404;
    else if (!(m_fileStat.st_mode & S_IROTH))
        m_nCode = 403;
    else if (m_nCode == -1)
        m_nCode = 200;

    if (m_nCode == 400 || m_nCode == 403 || m_nCode == 404)
    {
        m_path = std::to_string(m_nCode) + ".html";
        stat((m_srcDir + m_path).c_str(), &m_fileStat);
    }
    __addStateLine(buff);
    __addResponseHeader(buff);
    __addResponseContent(buff);
    return;
}

void HTTPResponse::unMapFile()
{
    if (nullptr != m_fileAddr)
    {
        munmap(m_fileAddr, m_fileStat.st_size);
        m_fileAddr = nullptr;
    }
    return;
}

void HTTPResponse::__addStateLine(Buffer& buff)
{
    std::string status;
    auto it = CODE_STATUS.find(m_nCode);
    if (it == CODE_STATUS.end())
    {
        m_nCode = 400;
        status = "Bad Request";
    }
    else
    {
        status = it->second;
    }
    buff.append("HTTP/1.1 " + std::to_string(m_nCode)
        + " " + status + "\r\n");
    return;
}

void HTTPResponse::__addResponseHeader(Buffer& buff) const
{
    buff.append("Connection: ");
    if(m_bKeepAlive) 
    {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    } 
    else
    {
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + __getFileType() + "\r\n");
    return;
}

void HTTPResponse::__addResponseContent(Buffer& buff)
{
    int srcFd = open((m_srcDir+m_path).c_str(), O_RDONLY);
    if (srcFd < 0)
    {
        __errorContent(buff, "File Not Found");
        return;
    }
    int* mmRet = reinterpret_cast<int*>(mmap(0, m_fileStat.st_size, 
        PROT_READ, MAP_PRIVATE, srcFd, 0));
    if (*mmRet == -1)
    {
        __errorContent(buff, "File Not Found");
        return;
    }
    m_fileAddr = reinterpret_cast<char*>(mmRet);
    close(srcFd);
    buff.append("Content-length: " + std::to_string(m_fileStat.st_size)
        + "\r\n\r\n");
    return;
}

std::string HTTPResponse::__getFileType() const
{
    auto idx = m_path.find_last_of('.');
    if(idx == std::string::npos)
        return "text/plain";
    std::string suffix = m_path.substr(idx);
    auto it = SUFFIX_TYPE.find(suffix);
    if(it != SUFFIX_TYPE.end())
        return it->second;
    return "text/plain";
}

void HTTPResponse::__errorContent(Buffer& buff, std::string message)
{
    std::string body;
    std::string status;
    body += "<html><title>Error</title><body bgcolor=\"ffffff\">";
    auto it = CODE_STATUS.find(m_nCode);
    if (it != CODE_STATUS.end())
        status = it->second;
    else 
        status = "Bad Request";
    body += std::to_string(m_nCode) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";
    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
    return;
}
