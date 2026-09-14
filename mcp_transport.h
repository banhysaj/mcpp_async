#pragma once

#include <string>
#include <iostream>

#if defined(_WIN32)
  #include <io.h>
  #include <fcntl.h>
#endif

namespace mcpp_async {

  class Transport {
  public:
    virtual ~Transport() {}
    virtual bool readLine(std::string& out) = 0;         // false at end of input (EOF)
    virtual void writeLine(const std::string& line) = 0; // deliver one message
  };

  // JSON split by newline over stdin/stdout (binary mode on Windows so '\n' is not rewritten to "\r\n").
  class StdioTransport : public Transport {
  public:
    StdioTransport() {
#if defined(_WIN32)
      _setmode(_fileno(stdin), _O_BINARY);
      _setmode(_fileno(stdout), _O_BINARY);
#endif
      std::ios::sync_with_stdio(false);
    }
    bool readLine(std::string& out) override {
      return static_cast<bool>(std::getline(std::cin, out));
    }
    void writeLine(const std::string& line) override {
      std::cout << line << "\n";
      std::cout.flush();
    }
  };

}
