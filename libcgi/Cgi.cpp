#include "libcgi/Cgi.hpp"
#include "libnet/SessionState.hpp"
#include <assert.h>
#include <cstring>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define assertm(ex, msg)                                                                           \
  (ex == false ? ({                                                                                \
    log(msg);                                                                                      \
    assert(ex);                                                                                    \
  })                                                                                               \
               : (assert(ex)))
#define log(msg) printf("%s\n", msg);

ssize_t doesContainerHasBuff(const char *raw, size_t rLen, const char *ptr, size_t pLen) {
  for (size_t i = 0; i < rLen - pLen + 1; i++) {
    if (!strncmp(&raw[i], ptr, strlen(ptr)))
      return i;
  }
  return -1;
}

libnet::SessionState libcgi::Cgi::handleCgiBuff(char *ptr, size_t len) {
  ssize_t     idx;
  const char *del = "\n\n";

  if (this->state == libnet::CGI_READING_HEADERS) {
    idx = doesContainerHasBuff(ptr, len, del, strlen(del));
    if (idx == -1) {
      this->res.cgiHeader.insert(this->res.cgiHeader.end(), ptr, ptr + len);
      return libnet::CGI_READING_HEADERS;
    }
    this->state = libnet::CGI_READING_BODY;
    this->res.cgiHeader.insert(this->res.cgiHeader.end(), ptr, ptr + idx + 1); // plus the /n
    this->res.body.insert(this->res.body.end(), ptr + idx + 1, ptr + len);
    return libnet::CGI_READING_BODY;
  }
  this->res.body.insert(this->res.body.end(), ptr, ptr + len);
  return libnet::CGI_READING_BODY;
}

char *keyAndValAsStr(const std::string &key, const std::string &val) {
  char *keyAndVal;

  keyAndVal = new char[key.size() + val.size() + 2];
  std::memcpy(keyAndVal, key.c_str(), key.size());
  keyAndVal[key.size()] = '=';
  std::memcpy(keyAndVal + key.size() + 1, val.c_str(), val.size());
  keyAndVal[key.size() + val.size() + 1] = 0;
  return keyAndVal;
}

char **headersAsEnv(std::map<std::string, std::string> &headers) {
  char                                             **env;
  std::map<std::string, std::string>::const_iterator it;
  std::map<std::string, std::string>::const_iterator end;
  std::map<std::string, std::string>::size_type      i;

  env = new char *[headers.size() + 1];
  i = 0;
  it = headers.begin();
  end = headers.end();
  for (; i < headers.size(); i++, it++) {
    env[i] = keyAndValAsStr(it->first, it->second);
  }
  env[i] = NULL;
  return env;
}

char **getScriptArgs(std::string &scriptPath) {
  char **argv;

  argv = new char *[2];

  argv[0] = ::strdup(scriptPath.c_str());
  argv[1] = NULL;
  return argv;
}

void delete2d(char **env) {
  unsigned int i;

  for (i = 0; env[i]; i++)
    delete[] env[i];
  delete[] env;
}

libcgi::Cgi::Cgi(libhttp::Request *httpReq, std::string scriptPath, sockaddr_in *clientAddr)
    : httpReq(httpReq)
    , scriptPath(scriptPath)
    , clientAddr(clientAddr)
    , req()
    , fd{-1, -1}
    , pid(-1)
    , bodySize(0)
    , state(libnet::CGI_INIT) {}

libcgi::Cgi::error libcgi::Cgi::init(std::string serverName, std::string scriptName,
                                     std::string localReqPath, std::string serverPort,
                                     std::string protocol, std::string serverSoftware) {
  struct stat s;

  this->req.init(serverName, scriptName, localReqPath, serverPort, protocol, serverSoftware);
  if (::stat(this->scriptPath.c_str(), &s) != 0)
    return FAILED_OPEN_FILE;
  if (!(s.st_mode & S_IXUSR))
    return FAILED_EXEC_PERM;
  this->req.build(this->httpReq);

  if (::pipe(fd) < 0)
    return FAILED_OPEN_PIPE;

  return OK;
}

libcgi::Cgi::error libcgi::Cgi::write(std::vector<char> &body) {
  int len;

  len = ::write(this->fd[1], &body.front(), body.size());
  if (len < 0)
    return FAILED_WRITE;
  this->bodySize += body.size();
  return OK;
};

libcgi::Cgi::error libcgi::Cgi::exec() {
  char **env, **argv;

  pid = fork();
  if (pid == 0) {
    dup2(fd[0], 0);
    dup2(fd[1], 1);
    argv = getScriptArgs(this->scriptPath);
    env = headersAsEnv(req.env);
    ::execve(this->scriptPath.c_str(), argv, env);
    delete2d(argv);
    delete2d(env);
    std::cerr << "child err" << std::endl;
    // write to the fd the error
    exit(1);
  } else if (pid > 0) {
    return OK;
  }
  return FAILED_FORK;
}

libcgi::Cgi::error libcgi::Cgi::read() {
  char    buff[4096 * 2];
  ssize_t len;

  if (this->state == libnet::CGI_INIT)
    this->state = libnet::CGI_READING_HEADERS;
  len = ::read(this->fd[0], buff, sizeof buff);
  if (len == 0) {
    this->state = libnet::CGI_FIN;
    return OK;
  }
  if (len < 0)
    return FAILED_READ;
  this->state = this->handleCgiBuff(buff, len);
  return OK;
}

void libcgi::Cgi::clean() {
  close(this->fd[0]);
  close(this->fd[1]);
  waitpid(this->pid, 0, 0); // this will block add WNOHANG
}
