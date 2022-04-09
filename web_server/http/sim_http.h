#ifndef SIM_HTTP_H_
#define SIM_HTTP_H_

#include <cctype>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

enum class HTTP_CODE {
    REQUEST_OK, BAD_REQUEST, CONN_CLOSED, INTERNAL_ERR, METHOD_NOT_SUPPORT
};

/*
方法：暂时只支持GET、POST
版本：暂时只考虑HTTP1.1
边界考虑：URI长度、请求头每行长度、请求体长度

请求体过大怎么办，例如上传文件？
要保证不占用过多内存，同时保证能较快处理socket接收数据
如此似乎需要特殊处理？暂时只考虑较小请求体吧，都读到内存

HTTP协商是怎么回事？有哪些首部需要考虑？
*/
class HttpRequest {
public:
    std::string method;
    std::string uri;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
private:
    enum class HTTP_STATE {
        REQUEST_LINE, REQUEST_HEADERS, REQUEST_BODY
    };
    enum class REQUEST_LINE {
        METHOD, URI, VERSION, RLOK1, RLOK2
    };
    enum class HEADER_LINE {
        START, KEY, SPLIT1, SPLIT2, VALUE, HLOK1, HLOK2, EMPTY1, EMPTY2
    };
private:
    static const std::unordered_set<std::string> METHODS;
    static const std::unordered_set<std::string> VERSIONS;
    // 各种限制参考：https://blog.csdn.net/liayn523/article/details/70243694
    static const int METHOD_MAX_LEN = 7;
    static const int URI_MAX_LEN = 8096;
    static const int VERSION_LEN = 8;
    static const int HEADER_KEY_MAX_LEN = 255;
    static const int HEADER_VALUE_MAX_LEN = 737280; // 以cookie为参考，180*4096
    static const int HEADERS_MAX_LEN = 8388608; // 8MB，8*1024*1024
private:
    static constexpr int BUF_SIZE = 4096;
    char buf[BUF_SIZE];
    int parse_idx;
    int nread;
    int clifd;
private:
    void init();
    void ParseRequestLine(HTTP_CODE& hcode, REQUEST_LINE& lstate);
    bool HandleMethod(std::string& method);
    bool HandleUri(std::string& uri);
    bool HandleVersion(std::string& version);
    // 不考虑首部延续行
    void ParseHeaderLine(HTTP_CODE& hcode, HEADER_LINE& hlstate, int& headers_len);
    bool HandleHearderLine(std::string& key, std::string& value);
public:
    HTTP_CODE ParseRequest(int clifd);
};

#endif