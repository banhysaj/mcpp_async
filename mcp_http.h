#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <cstddef>

namespace mcpp_async {

	class Server;

	struct HttpRequest {
		std::string remoteAddr;
		std::map<std::string, std::string> headers;
		std::string header(const std::string& name) const;
	};

	struct HttpOptions {
		std::string host;
		int port;
		std::string path;
		std::vector<std::string> allowedOrigins;
		size_t maxBodyBytes;
		int threads;
		int keepAliveSeconds;
		std::function<bool(const HttpRequest& req, std::string& principal, std::string& challenge)> authorize;

		HttpOptions() : host("127.0.0.1"), port(8080), path("/mcp"), maxBodyBytes(4 * 1024 * 1024), threads(0), keepAliveSeconds(15) {}
	};

	class HttpServer {
	public:
		HttpServer(Server& server, const HttpOptions& options);
		~HttpServer();

		bool bind();
		int port() const;
		bool listen();
		void stop();

	private:
		HttpServer(const HttpServer&);
		HttpServer& operator=(const HttpServer&);
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};

}
