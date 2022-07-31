#include "HTTPrequest.h"
#include <regex>
#include <iostream>
#include "buffer.h"

const std::unordered_set<std::string> HTTPRequest::DEFAULT_HTML{
            "/index", "/welcome", "/video", "/picture"};

HTTPRequest::HTTPRequest()
    : m_state(PARSE_STATE::REQUEST_LINE)
    , m_method("")
    , m_path("")
    , m_version("")
    , m_body("")
    , m_header()
    , m_post()
{
}

void HTTPRequest::init()
{
    m_state = PARSE_STATE::REQUEST_LINE;
    m_method = "";
    m_path = "";
    m_version = "";
    m_body = "";
    m_header.clear();
    m_post.clear();
}

bool HTTPRequest::parse(Buffer& buff)
{   
    if (buff.readableBytes() <= 0)
        return false;
    const char CRLF[] = "\r\n";
    while (buff.readableBytes() > 0 && PARSE_STATE::FINISH != m_state)
    {   
        const char* lineEnd = std::search(buff.getReadCP(), buff.getWriteCP(), CRLF, CRLF+2);
        std::string line(buff.getReadCP(), lineEnd);
        if (lineEnd == buff.getWriteCP())
            break;
        switch (m_state)
        {
            case PARSE_STATE::REQUEST_LINE:
            {   
                if(!__parseRequestLine(line))
                    return false;
                break;
            }
            case PARSE_STATE::HEADERS:
            {   
                __parseRequestHeader(line);
                if(buff.readableBytes() <= 2)
                    m_state = PARSE_STATE::FINISH;
                break;
            }
            case PARSE_STATE::BODY:
            {   
                __parseRequestBody(line);
                break;
            }
            default:
                break;
        }
        
        buff.updateReadPosUntil(lineEnd + 2);
    }
    return true;
}

bool HTTPRequest::isKeepAlive() const
{
    auto it = m_post.find("Connection");
    if (it == m_post.end())
        return false;
    return (it->second == "keep-alive") && (m_version == "1.1");
}

std::string HTTPRequest::getPost(const std::string& key) const
{   
    auto it = m_post.find(key);
    if (it == m_post.end())
        return "";
    else 
        return it->second;
}

std::string HTTPRequest::getPost(const char* key) const
{
    return getPost(std::string(key));
}

int HTTPRequest::convertToHex(char ch)
{
    if (ch >= 'A' && ch <= 'F')
        return (ch - 'A' + 10);
    if (ch >= 'a' && ch <= 'f')
        return (ch - 'a' + 10);
    return ch;
}

std::string HTTPRequest::__getFormatPath(const std::string& rawPath)
{
    if (rawPath == "/")
        return "/index.html";
    else
    {
        for (const auto& item : DEFAULT_HTML)
        {
            if (item == rawPath)
                return (rawPath + ".html");
        }
        return rawPath;
    }
}

bool HTTPRequest::__parseRequestLine(const std::string& line)
{
    const std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) 
    {   
        m_method = subMatch[1];
        m_path = __getFormatPath(subMatch[2]);
        m_version = subMatch[3];
        m_state = PARSE_STATE::HEADERS;
        return true;
    }
    return false;
}

void HTTPRequest::__parseRequestHeader(const std::string& line)
{
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten))
        m_header[subMatch[1]] = subMatch[2];
    else
        m_state = PARSE_STATE::BODY;
}

void HTTPRequest::__parseRequestBody(const std::string& line)
{
    m_body = line;
    if (m_body.empty())
        return;
    if (m_method != "POST")
        return;
    auto it = m_header.find("Content-Type");
    if (it == m_header.end() || it->second != "application/x-www-form-urlencoded")
        return;
    std::string key, value;
    size_t i = 0, j = 0;
    while (j < m_body.size())
    {
        switch (m_body[j])
        {
            case '=':
                key = m_body.substr(i, j - i);
                i = j + 1;
                break;
            case '+':
                m_body[j] = ' ';
                break;
            case '%':
	    {
                int hexNum = convertToHex(m_body[j+1]) * 16 + convertToHex(m_body[j+2]);
                m_body[j+1] = hexNum / 10 + '0';
                m_body[j+2] = hexNum % 10 + '0';
                j += 2;
                break;
	    }
            case '&':
                value = m_body.substr(i, j - i);
                i = j + 1;
                m_post[key] = value;
                break;
            default:
                break;
        }
        j++;
    }
    // solve last param
    if (m_post.find(key) == m_post.end() && i < j)
    {
        value = m_body.substr(i, j - i);
        m_post[key] = value;
    }
    m_state = PARSE_STATE::FINISH;
    return;
}
