#include "httplib/httplib.h"

#include "mcp_http.h"
#include "mcp_server.h"
#include "mcp_constants.h"
#include "mcp_json.h"

#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace mcpp_async {

    namespace {
        std::string lower(std::string s) {
            for (size_t i = 0; i < s.size(); ++i) s[i] = (char)std::tolower((unsigned char)s[i]);
            return s;
        }

        bool decodeBase64(const std::string& in, std::string& out) {
            static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            out.clear();
            int val = 0, bits = -8;
            size_t i = 0;
            for (; i < in.size() && in[i] != '='; ++i) {
                size_t p = chars.find(in[i]);
                if (p == std::string::npos) return false;
                val = (val << 6) + (int)p;
                bits += 6;
                if (bits >= 0) {
                    out.push_back((char)((val >> bits) & 0xFF));
                    bits -= 8;
                }
            }
            for (; i < in.size(); ++i) if (in[i] != '=') return false;
            return true;
        }

        bool headerText(const std::string& raw, std::string& out) {
            static const std::string pre = "=?base64?", suf = "?=";
            if (raw.size() >= pre.size() + suf.size() && raw.compare(0, pre.size(), pre) == 0 &&
                raw.compare(raw.size() - suf.size(), suf.size(), suf) == 0) {
                return decodeBase64(raw.substr(pre.size(), raw.size() - pre.size() - suf.size()), out);
            }
            out = raw;
            return true;
        }

        bool isLocalOrigin(const std::string& origin) {
            std::string rest;
            if (origin.compare(0, 7, "http://") == 0) rest = origin.substr(7);
            else if (origin.compare(0, 8, "https://") == 0) rest = origin.substr(8);
            else return false;
            if (rest.compare(0, 5, "[::1]") == 0) return rest.size() == 5 || rest[5] == ':';
            std::string host = rest.substr(0, rest.find(':'));
            return host == "localhost" || host == "127.0.0.1";
        }

        std::string errorBody(int code, const std::string& message, const rj::Value* id) {
            rj::Document env; env.SetObject();
            rj::Document::AllocatorType& a = env.GetAllocator();
            json::addStr(env, fldJsonrpc, kJsonRpcVersion, a);
            if (id) {
                rj::Value idCopy; idCopy.CopyFrom(*id, a);
                env.AddMember(rj::StringRef(fldId), idCopy, a);
            }
            rj::Value err(rj::kObjectType);
            json::addInt(err, fldCode, code, a);
            json::addStr(err, fldMessage, message, a);
            env.AddMember(rj::StringRef(fldError), err, a);
            return json::serialize(env);
        }

        int statusFor(const std::string& response) {
            rj::Document d;
            d.Parse(response.c_str(), response.size());
            if (d.HasParseError() || !d.IsObject()) return 200;
            const rj::Value* e = args::get(d, fldError);
            const rj::Value* c = e ? args::get(*e, fldCode) : 0;
            if (!c || !c->IsInt()) return 200;
            switch (c->GetInt()) {
            case kMethodNotFound: return 404;
            case kParseError:
            case kInvalidRequest:
            case kHeaderMismatch:
            case kMissingRequiredClientCapability:
            case kUnsupportedProtocolVersion: return 400;
            default: return 200;
            }
        }

        void reply(httplib::Response& res, int status, const std::string& body) {
            res.status = status;
            res.set_content(body, "application/json");
        }

        bool mismatch(const char* header, const std::string& got, const std::string& want, std::string& err) {
            if (got.empty()) { err = std::string("Header mismatch: missing ") + header + " header"; return true; }
            if (got != want) {
                err = std::string("Header mismatch: ") + header + " header value '" + got + "' does not match body value '" + want + "'";
                return true;
            }
            return false;
        }

        bool checkHeaders(const rj::Value& msg, const HttpRequest& req, std::string& err) {
            const rj::Value* params = args::get(msg, fldParams);
            const rj::Value* meta = params ? args::get(*params, fldMeta) : 0;
            const rj::Value* pv = meta ? args::get(*meta, kMetaProtocolVersion) : 0;
            if (!pv || !pv->IsString()) return true;

            std::string method = args::str(msg, fldMethod);
            if (mismatch("MCP-Protocol-Version", req.header("mcp-protocol-version"), pv->GetString(), err)) return false;
            if (mismatch("Mcp-Method", req.header("mcp-method"), method, err)) return false;

            const char* nameField = 0;
            if (method == mtdToolsCall || method == mtdPromptsGet) nameField = fldName;
            else if (method == mtdResourcesRead) nameField = fldUri;
            if (!nameField) return true;

            std::string name;
            if (!headerText(req.header("mcp-name"), name)) { err = "Header mismatch: Mcp-Name header is not valid base64"; return false; }
            return !mismatch("Mcp-Name", name, params ? args::str(*params, nameField) : std::string(), err);
        }

        const size_t kMaxQueuedEvents = 1024;

        struct Stream {
            std::mutex m;
            std::condition_variable cv;
            std::deque<std::string> events;
            bool done;
            std::shared_ptr<std::atomic<bool>> cancel;
            std::thread worker;

            Stream() : done(false), cancel(std::make_shared<std::atomic<bool>>(false)) {}

            void push(const std::string& line, bool last) {
                {
                    std::lock_guard<std::mutex> lk(m);
                    if (!last && events.size() >= kMaxQueuedEvents) return;
                    if (!line.empty()) events.push_back(line);
                    if (last) done = true;
                }
                cv.notify_one();
            }
        };

        bool pump(Stream& st, httplib::DataSink& sink, int keepAliveSeconds) {
            int idle = 0;
            std::unique_lock<std::mutex> lk(st.m);
            for (;;) {
                while (!st.events.empty()) {
                    std::string ev = "data: " + st.events.front() + "\n\n";
                    st.events.pop_front();
                    lk.unlock();
                    bool ok = sink.write(ev.data(), ev.size());
                    lk.lock();
                    if (!ok) return false;
                    idle = 0;
                }
                if (st.done) {
                    lk.unlock();
                    sink.done();
                    return true;
                }
                if (st.cv.wait_for(lk, std::chrono::seconds(1)) == std::cv_status::timeout && st.events.empty() && !st.done) {
                    lk.unlock();
                    bool alive = sink.is_writable();
                    if (alive && keepAliveSeconds > 0 && ++idle >= keepAliveSeconds) {
                        idle = 0;
                        static const char ka[] = ": keep-alive\n\n";
                        alive = sink.write(ka, sizeof(ka) - 1);
                    }
                    lk.lock();
                    if (!alive) return false;
                }
            }
        }
    }

    std::string HttpRequest::header(const std::string& name) const {
        std::map<std::string, std::string>::const_iterator it = headers.find(lower(name));
        return it == headers.end() ? std::string() : it->second;
    }

    struct HttpServer::Impl {
        Server& server;
        HttpOptions opts;
        std::unique_ptr<httplib::Server> http;
        int boundPort;

        Impl(Server& s, const HttpOptions& o) : server(s), opts(o), http(makeServer(o)), boundPort(-1) {}

        static httplib::Server* makeServer(const HttpOptions& o) {
#ifdef CPPHTTPLIB_SSL_ENABLED
            if (!o.certFile.empty()) return new httplib::SSLServer(o.certFile.c_str(), o.keyFile.c_str());
#endif
            return new httplib::Server();
        }

        bool originAllowed(const std::string& origin) const {
            if (opts.allowedOrigins.empty()) return isLocalOrigin(origin);
            for (size_t i = 0; i < opts.allowedOrigins.size(); ++i) {
                if (opts.allowedOrigins[i] == origin) return true;
            }
            return false;
        }

        void onPost(const httplib::Request& req, httplib::Response& res) {
            std::string origin = req.get_header_value("Origin");
            if (!origin.empty() && !originAllowed(origin)) {
                reply(res, 403, errorBody(kInvalidRequest, "Origin not allowed", 0));
                return;
            }

            HttpRequest info;
            info.remoteAddr = req.remote_addr;
            for (httplib::Headers::const_iterator it = req.headers.begin(); it != req.headers.end(); ++it) {
                info.headers[lower(it->first)] = it->second;
            }

            RequestScope scope;
            std::string challenge;
            bool allowed = true;
            try {
                allowed = !opts.authorize || opts.authorize(info, scope.principal, challenge);
            }
            catch (const Unavailable& u) {
                res.set_header("Retry-After", std::to_string(u.retryAfterSeconds));
                reply(res, 503, errorBody(kInternalError, "Service unavailable", 0));
                return;
            }
            if (!allowed) {
                res.status = 401;
                if (!challenge.empty()) res.set_header("WWW-Authenticate", challenge);
                return;
            }

            if (!json::withinDepth(req.body.c_str(), req.body.size())) {
                reply(res, 400, errorBody(kInvalidRequest, "Message is nested too deeply", 0));
                return;
            }
            rj::Document msg;
            msg.Parse(req.body.c_str(), req.body.size());
            if (msg.HasParseError()) { reply(res, 400, errorBody(kParseError, "Parse error", 0)); return; }
            if (!msg.IsObject()) { reply(res, 400, errorBody(kInvalidRequest, "Body must be a single JSON-RPC message", 0)); return; }

            const rj::Value* id = args::get(msg, fldId);
            std::string err;
            if (!checkHeaders(msg, info, err)) { reply(res, 400, errorBody(kHeaderMismatch, err, id)); return; }

            if (!id) {
                server.handleMessage(req.body, scope);
                res.status = 202;
                return;
            }

            bool sse = args::str(msg, fldMethod) == mtdToolsCall &&
                req.get_header_value("Accept").find("text/event-stream") != std::string::npos;
            if (!sse) {
                std::string out = server.handleMessage(req.body, scope);
                if (out.empty()) { res.status = 202; return; }
                reply(res, statusFor(out), out);
                return;
            }

            std::shared_ptr<Stream> st = std::make_shared<Stream>();
            Stream* raw = st.get();
            scope.cancel = st->cancel;
            scope.notify = [raw](const std::string& line) { raw->push(line, false); };

            Server* srv = &server;
            std::string body = req.body;
            st->worker = std::thread([srv, raw, body, scope]() {
                std::string out;
                try { out = srv->handleMessage(body, scope); }
                catch (...) { out = errorBody(kInternalError, "Internal error", 0); }
                raw->push(out, true);
            });

            int keepAlive = opts.keepAliveSeconds;
            res.set_header("Cache-Control", "no-cache");
            res.set_header("X-Accel-Buffering", "no");
            res.set_chunked_content_provider("text/event-stream",
                [st, keepAlive](size_t, httplib::DataSink& sink) {
                    if (pump(*st, sink, keepAlive)) return true;
                    st->cancel->store(true);
                    return false;
                },
                [st](bool ok) {
                    if (!ok) st->cancel->store(true);
                    if (st->worker.joinable()) st->worker.join();
                });
        }
    };

    HttpServer::HttpServer(Server& server, const HttpOptions& options) : impl_(new Impl(server, options)) {
        Impl* impl = impl_.get();
        impl->http->set_payload_max_length(options.maxBodyBytes);
        if (options.threads > 0) {
            size_t n = (size_t)options.threads;
            impl->http->new_task_queue = [n] { return new httplib::ThreadPool(n); };
        }
        impl->http->Post(options.path, [impl](const httplib::Request& req, httplib::Response& res) {
            impl->onPost(req, res);
        });
        httplib::Server::Handler notAllowed = [](const httplib::Request&, httplib::Response& res) {
            res.status = 405;
            res.set_header("Allow", "POST");
        };
        impl->http->Get(options.path, notAllowed);
        impl->http->Delete(options.path, notAllowed);
    }

    HttpServer::~HttpServer() { stop(); }

    bool HttpServer::supportsTls() {
#ifdef CPPHTTPLIB_SSL_ENABLED
        return true;
#else
        return false;
#endif
    }

    bool HttpServer::bind() {
        if (impl_->boundPort > 0) return true;
        // a cert that wouldn't load, or one asked for in a build without TLS, never falls back to plain HTTP
        if (!impl_->opts.certFile.empty() && (!supportsTls() || !impl_->http->is_valid())) return false;
        if (impl_->opts.port == 0) {
            impl_->boundPort = impl_->http->bind_to_any_port(impl_->opts.host);
        }
        else if (impl_->http->bind_to_port(impl_->opts.host, impl_->opts.port)) {
            impl_->boundPort = impl_->opts.port;
        }
        return impl_->boundPort > 0;
    }

    int HttpServer::port() const { return impl_->boundPort; }

    bool HttpServer::listen() {
        if (!bind()) return false;
        return impl_->http->listen_after_bind();
    }

    void HttpServer::stop() {
        if (impl_->http->is_running()) impl_->http->stop();
    }

}
