#include "libhttp/Multiplexer.hpp"
#include "libhttp/Headers.hpp"
#include "libnet/Session.hpp"
#include "libparse/Config.hpp"
#include <string>

static const libparse::Domain *matchDomain(const libparse::Domains &domains,
                                           const std::string       &domain) {
  libparse::Domains::const_iterator iter = domains.find(domain);

  if (iter != domains.end())
    return &iter->second;
  else
    return &domains.begin()->second;
}

static const libparse::RouteProps *matchRoute(const libparse::Domain &domain,
                                              const std::string      &path) {
  std::string matchedRoute = libparse::matching(domain, path);

  if (matchedRoute.length())
    return &domain.routes.find(matchedRoute)->second;

  return NULL;
}

static std::string extractHost(libhttp::HeadersMap &headers) {
  libhttp::HeadersMap::iterator hostIter = headers.find("Host");

  if (hostIter != headers.end())
    return hostIter->second;

  return "";
}

static bool isRequestHandlerCgi(const libparse::RouteProps *route) {
  if (route->cgi.first.length() > 0 && route->cgi.second.length() > 0)
    return true;
  return false;
}

static bool isMethodAllowedOnRoute(const libparse::RouteProps *route, const std::string &method) {
  std::vector<std::string>::const_iterator begin = route->methods.begin();
  std::vector<std::string>::const_iterator end = route->methods.end();

  while (begin != end) {
    if (*begin == method)
      return true;
    begin++;
  }

  return false;
}

void libhttp::multiplexer(libnet::Session *session, const libparse::Domains &domains) {
  libhttp::Request           *req = session->reader.requests.front();
  std::string                 host = extractHost(req->headers.headers);
  const libparse::Domain     *domain = matchDomain(domains, host);
  const libparse::RouteProps *route = matchRoute(*domain, req->reqTarget.path);

  if (isMethodAllowedOnRoute(route, req->method) == false) {
    // Method not allowed
    // 405 Method Not Allowed
    return;
  }

  if (isRequestHandlerCgi(route)) {
    // Cgi
  }

  else if (req->method == "GET" || req->method == "DELETE") {

  }

  else if (req->method == "POST") {
  }
}
