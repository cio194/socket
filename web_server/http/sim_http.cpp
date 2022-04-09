#include "sim_http.h"
#include "sim_utils.h"

using std::string;
using std::unordered_map;
using std::unordered_set;

#include <iostream>
using std::cout;
using std::endl;

const unordered_set<string> HttpRequest::METHODS = {
    "CONNECT", "DELETE", "GET", "HEAD", "OPTIONS", "PATCH", "POST", "PUT",
    "TRACE"
};

const unordered_set<string> HttpRequest::VERSIONS = {
    "HTTP/1.1"
};

void HttpRequest::init() {
    headers.clear();
    parse_idx = 0;
    nread = 0;
}

void HttpRequest::ParseRequestLine(HTTP_CODE& hcode, REQUEST_LINE& lstate) {
    string method, uri, version;
    while (true) {
        nread = Read(clifd, buf, BUF_SIZE);
        if (nread == 0) {
            ErrMsg("请求行解析错误：连接提前关闭");
            hcode = HTTP_CODE::CONN_CLOSED;
            return;
        }
        parse_idx = 0;
        for (; parse_idx < nread; parse_idx++) {
            char c = buf[parse_idx];
            switch (lstate) {
                case REQUEST_LINE::METHOD:
                    {
                        if (isupper(c)) {
                            method.push_back(c);
                            if (method.size() > METHOD_MAX_LEN) {
                                hcode = HTTP_CODE::BAD_REQUEST;
                            }
                        } else if (c == ' ') {
                            if (!HandleMethod(method)) {
                                hcode = HTTP_CODE::BAD_REQUEST;
                            } else {
                                lstate = REQUEST_LINE::URI;
                            }
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                case REQUEST_LINE::URI:
                    {
                        if (isprint(c) && c != ' ') {
                            uri.push_back(c);
                            if (uri.size() > URI_MAX_LEN) {
                                hcode = HTTP_CODE::BAD_REQUEST;
                            }
                        } else if (c == ' ') {
                            if (!HandleUri(uri)) {
                                hcode = HTTP_CODE::BAD_REQUEST;
                            } else {
                                lstate = REQUEST_LINE::VERSION;
                            }
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                case REQUEST_LINE::VERSION:
                    {
                        if (isprint(c) && c != ' ') {
                            version.push_back(c);
                            if (version.size() > VERSION_LEN) {
                                hcode = HTTP_CODE::BAD_REQUEST;
                            }
                        } else if (c == '\r') {
                            if (!HandleVersion(version)) {
                                hcode = HTTP_CODE::BAD_REQUEST;
                            } else {
                                lstate = REQUEST_LINE::RLOK1;
                            }
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                case REQUEST_LINE::RLOK1:
                    {
                        if (c == '\n') {
                            lstate = REQUEST_LINE::RLOK2;
                            parse_idx++;
                            return;
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                default:
                    hcode = HTTP_CODE::BAD_REQUEST;
                    break;
            }
            if (hcode != HTTP_CODE::REQUEST_OK) {
                ErrMsg("请求行解析错误");
                return;
            }
        }
    }
}

bool HttpRequest::HandleMethod(std::string& method) {
    if (METHODS.count(method)) {
        this->method = std::move(method);
        return true;
    }
    return false;
}

bool HttpRequest::HandleUri(std::string& uri) {
    if (uri.size()) {
        this->uri = std::move(uri);
        return true;
    }
    return false;
}

bool HttpRequest::HandleVersion(std::string& version) {
    if (VERSIONS.count(version)) {
        this->version = std::move(version);
        return true;
    }
    return false;
}

void HttpRequest::ParseHeaderLine(HTTP_CODE& hcode, HEADER_LINE& hlstate, int& headers_len) {
    string key, value;
    while (true) {
        if (parse_idx >= nread) {
            parse_idx = 0;
            nread = Read(clifd, buf, BUF_SIZE);
            if (nread == 0) {
                ErrMsg("请求行头解析错误：连接提前关闭");
                hcode = HTTP_CODE::CONN_CLOSED;
                return;
            }
        }
        for (; parse_idx < nread; parse_idx++) {
            char c = buf[parse_idx];
            switch (hlstate) {
                case HEADER_LINE::START:
                    {
                        if (isalpha(c) || c == '-') {
                            key.push_back(c);
                            hlstate = HEADER_LINE::KEY;
                        } else if (c == '\r') {
                            hlstate = HEADER_LINE::EMPTY1;
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                case HEADER_LINE::KEY:
                    {
                        if (isalpha(c) || c == '-') {
                            key.push_back(c);
                            if (key.size() > HEADER_KEY_MAX_LEN) {
                                hcode = HTTP_CODE::BAD_REQUEST;
                            }
                        } else if (c == ':') {
                            hlstate = HEADER_LINE::SPLIT1;
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                case HEADER_LINE::SPLIT1:
                    {
                        if (c == ' ') {
                            hlstate = HEADER_LINE::SPLIT2;
                        } else if (isprint(c)) {
                            value.push_back(c);
                            hlstate = HEADER_LINE::VALUE;
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                case HEADER_LINE::SPLIT2:
                    {
                        if (isprint(c) && c != ' ') {
                            value.push_back(c);
                            hlstate = HEADER_LINE::VALUE;
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                case HEADER_LINE::VALUE:
                    {
                        if (isprint(c)) {
                            value.push_back(c);
                            if (value.size() > HEADER_VALUE_MAX_LEN) {
                                hcode = HTTP_CODE::BAD_REQUEST;
                            }
                        } else if (c == '\r') {
                            hlstate = HEADER_LINE::HLOK1;
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                case HEADER_LINE::HLOK1:
                    {
                        if (c == '\n') {
                            headers_len += key.size() + value.size();
                            if (headers_len > HEADERS_MAX_LEN) {
                                hcode = HTTP_CODE::BAD_REQUEST;
                            } else {
                                if (!HandleHearderLine(key, value)) {
                                    hcode = HTTP_CODE::BAD_REQUEST;
                                } else {
                                    hlstate = HEADER_LINE::HLOK2;
                                    parse_idx++;
                                    return;
                                }
                            }
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                case HEADER_LINE::EMPTY1:
                    {
                        if (c == '\n') {
                            hlstate = HEADER_LINE::EMPTY2;
                            parse_idx++;
                            return;
                        } else {
                            hcode = HTTP_CODE::BAD_REQUEST;
                        }
                    }
                    break;
                default:
                    hcode = HTTP_CODE::BAD_REQUEST;
                    break;
            }
            if (hcode != HTTP_CODE::REQUEST_OK) {
                ErrMsg("请求头解析错误");
                return;
            }
        }
    }
}

bool HttpRequest::HandleHearderLine(string& key, string& value) {
    if (key.size() && value.size()) {
        headers.insert({ std::move(key), std::move(value) });
        return true;
    }
    return false;
}

HTTP_CODE HttpRequest::ParseRequest(int clifd) {
    this->init();
    this->clifd = clifd;
    auto hstate = HTTP_STATE::REQUEST_LINE;
    auto hcode = HTTP_CODE::REQUEST_OK;
    while (true) {
        switch (hstate) {
            case HTTP_STATE::REQUEST_LINE:
                {
                    auto lstate = REQUEST_LINE::METHOD;
                    ParseRequestLine(hcode, lstate);
                    if (hcode == HTTP_CODE::REQUEST_OK) {
                        hstate = HTTP_STATE::REQUEST_HEADERS;
                    }
                }
                break;
            case HTTP_STATE::REQUEST_HEADERS:
                {
                    int headers_len = 0;
                    while (true) {
                        auto hlstate = HEADER_LINE::START;
                        ParseHeaderLine(hcode, hlstate, headers_len);
                        if (hcode == HTTP_CODE::REQUEST_OK) {
                            if (hlstate == HEADER_LINE::EMPTY2) {
                                hstate = HTTP_STATE::REQUEST_BODY;
                                break;
                            } else continue;
                        } else {
                            break;
                        }
                    }
                }
                break;
            case HTTP_STATE::REQUEST_BODY:
                return hcode;
            default:
                hcode = HTTP_CODE::BAD_REQUEST;
                break;
        }
        if (hcode != HTTP_CODE::REQUEST_OK) {
            return hcode;
        }
    }
}