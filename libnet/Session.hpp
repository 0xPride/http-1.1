#pragma once

#include "libhttp/Reader.hpp"

namespace libnet {
  enum sessionStat {
    READING_HEADERS,
    READING_BODY,
    READING_ERR,
    READING_FIN,

    CGI_WAIT,
    CGI_TIMEOUT,
    CGI_FIN,
    CGI_ERR,

    WRITTING_TO_CLIENT,
    WRTTING_ERR,
    FIN
  };

  struct Session {
    Session(int fd);

    int fd;

    sessionStat status;
    libhttp::Reader Reader;
  };

  bool operator<(const libnet::Session &, const libnet::Session &);
  bool operator==(std::pair<const int, libnet::Session> pair, const libnet::Session &);
} // namespace libnet
