/**
 * @class   HTTPResponse
 * @author  github.com/translee
 * @date    2022/06/10
 * @brief   
 */

#pragma once
#include <string>
#include <unordered_map>
#include <sys/stat.h>
class Buffer;

class HTTPResponse
{
public:
    HTTPResponse();
    ~HTTPResponse();
    void init(const std::string& srcDir, const std::string& path, 
        bool isKeepAlive=false, int code=-1);
    void makeResponse(Buffer& buff);
    char* getFile() const { return m_fileAddr; }
    size_t fileLen() const { return m_fileStat.st_size; }
    void unMapFile();
private:
    void __addStateLine(Buffer& buff);
    void __addResponseHeader(Buffer& buff) const;
    void __addResponseContent(Buffer& buff);
    std::string __getFileType() const;
    void __errorContent(Buffer& buff, std::string message);
private:
    int m_nCode;
    bool m_bKeepAlive;
    std::string m_path;
    std::string m_srcDir;
    struct stat m_fileStat;
    char* m_fileAddr;
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
};
