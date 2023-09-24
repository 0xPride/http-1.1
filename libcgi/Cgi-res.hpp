#pragma once

// #include "libhttp/Request-target.hpp"
#include "libhttp/Headers.hpp"
#include <vector>

namespace libcgi {
  struct Respons {
    std::string       statusLine;
    std::vector<char> cgiHeader;
    std::vector<char> httpHeaders;
    std::vector<char> body;

    void build();
    void cgiHeaderToHttpHeader(const std::string &h);
  };
} // namespace libcgi
