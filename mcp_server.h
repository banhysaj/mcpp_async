#pragma once

#include <string>
#include "mcp_types.h"
#include "mcp_json.h"
#include <functional>
#include <atomic>
#include <memory>
#include <mutex>
#include <map>

namespace mcpp_async {

	class Server;    //Forward declaration
	class Transport;
	typedef rapidjson::Document::AllocatorType Alloc;

	class RequestContext {
	public:
		std::string method; // the JSON-RPC method being handled
		bool stateless; // true for a stateless (2026-07-28) request
		std::string protocolVersion;
		bool hasProgressToken;

		// progress update for THIS request. Does nothing unless the client attached a progressToken (notifications/progress)
		void progress(double progress, double total = -1.0, const std::string& message = std::string());
		// a log line to the client
		void log(const std::string& level, const std::string& message);
		void notify(const std::string& method, const std::string& rawJsonParams);

		// true if the client sent notifications/cancelled for THIS request
		// long-running handlers should poll this and exit early.
		bool cancelled() const;

		Server* server() const { return server_; }

	private:
		friend class Server;
		RequestContext() : stateless(false), hasProgressToken(false), server_(0), progressToken_(0) {}
		Server* server_;
		const rapidjson::Value* progressToken_; // valid only for the duration of the call
		std::shared_ptr<std::atomic<bool>> cancel_; // set true when this request is cancelled
	};


	typedef std::function<ToolResult(const rapidjson::Value& arguments)> ToolHandler;
	typedef std::function<ToolResult(const rapidjson::Value& arguments, RequestContext& ctx)> ToolHandlerCtx;

	class Server
	{
		friend class RequestContext;

	public:
		Server(const std::string& name, const std::string& version);
		~Server();

		const std::string& name() const { return name_; }
		const std::string& version() const { return version_; }

		int run(); // serve over stdio
		int run(Transport& transport);
		void setTransport(Transport* transport) { transport_ = transport; }
		bool initialized() const { return initialized_.load(); }

		// number of worker threads run() uses
		void setMaxThreads(int n) { maxThreads_ = n; }
		void sendNotification(const std::string& method, const std::string& rawJsonParams = std::string("{}"));
		void notifyToolsListChanged();
		void logMessage(const std::string& level, const std::string& message, const std::string& logger = std::string());
		void addTool(const Tool& tool, ToolHandler handler);
		void addToolCtx(const Tool& tool, ToolHandlerCtx handler);

		// text returned in the initialize result's "instructions" field
		// clients usually surface this to the model as usage guidance when a session is created
		void setInstructions(const std::string& text) { instructions_ = text; }
		void setCacheHint(int64_t ttlMs, const std::string& scope = std::string("private")) {
			cacheTtlMs_ = ttlMs;
			cacheScope_ = scope;
		}
		std::string handleLine(const std::string& line);

	private:
		struct ToolEntry {
			Tool tool;
			ToolHandler handler;
			ToolHandlerCtx handlerCtx;
			bool ctx;
			ToolEntry() : ctx(false) {}
		};

		std::string name_;
		std::string version_;
		int64_t cacheTtlMs_;
		std::string cacheScope_;
		std::atomic<bool> initialized_;
		std::string loggingLevel_;
		std::string instructions_; // returned in initialize result 
		Transport* transport_;
		std::vector<ToolEntry> tools_;

		int maxThreads_;
		std::mutex outMutex_; //serialize all outbound writes
		std::mutex stateMutex_; //guards loggingLevel_ + cancels_
		std::map<std::string, std::shared_ptr<std::atomic<bool>>> cancels_;

		struct Incoming { std::string method; std::string id; std::string cancelId; };
		int workerCount() const;
		Incoming preview(const std::string& line) const; //peek method/id before dispatching
		void handleAndSend(const std::string& line);
		void runRequest(const std::string& line, const std::string& id);
		void registerCancel(const std::string& id);// create a cancel flag for a request
		void markCancelled(const std::string& id);
		void clearCancel(const std::string& id);
		std::shared_ptr<std::atomic<bool>> cancelFlagFor(const std::string& id);

		void addCapabilities(rapidjson::Value& result, Alloc& a) const;
		void buildServerInfo(rapidjson::Value& out, Alloc& a) const;
		static std::string stripBomAndTrim(const std::string& line);
		const ToolEntry* findTool(const std::string& name) const;
		void sendLine(const std::string& line);
		void sendNotificationDoc(const std::string& method, rapidjson::Document& params);

		std::string dispatchOne(const rapidjson::Value& msg);
		std::string finalize(const rapidjson::Value& id, const char* method, bool stateless, rapidjson::Document& resultDoc);
		std::string makeError(const rapidjson::Value& id, int code, const std::string& message);

		std::string onInitialize(const rapidjson::Value& id, const rapidjson::Value& params);
		std::string onDiscover(const rapidjson::Value& id);
		std::string onToolsList(const rapidjson::Value& id, bool stateless);
		std::string onToolsCall(const rapidjson::Value& id, bool stateless, const rapidjson::Value& params);
	};
}