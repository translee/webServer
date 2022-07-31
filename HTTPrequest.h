/**
 * @class   HTTPRequest
 * @author  github.com/translee
 * @date    2022/06/08
 * @brief   
 */

#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
class Buffer;

class HTTPRequest
{
public:
    enum class PARSE_STATE : int
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
public:
    HTTPRequest();
    void init();
    bool parse(Buffer& buff);
    bool isKeepAlive() const;
    std::string path() const { return m_path; }
    std::string method() const { return m_method; }
    std::string version() const { return m_version; }
    std::string getPost(const std::string& key) const;
    std::string getPost(const char* key) const;
    static int convertToHex(char ch);
private:
    std::string __getFormatPath(const std::string& rawPath);
    bool __parseRequestLine(const std::string&);
    void __parseRequestHeader(const std::string&);
    void __parseRequestBody(const std::string&);
private:
    PARSE_STATE m_state;
    std::string m_method;
    std::string m_path;
    std::string m_version;
    std::string m_body;
    std::unordered_map<std::string, std::string> m_header;
    std::unordered_map<std::string, std::string> m_post;
    static const std::unordered_set<std::string> DEFAULT_HTML;
};
