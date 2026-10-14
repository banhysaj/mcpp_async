# mcpp_async

The async version of [mcpp](https://github.com/banhysaj/mcpp.git). It is the same library, 
the difference is that this version runs tool calls on a worker thread
pool instead of one at a time, and it supports request cancellation.

If you're new to the project, read the base repo first and the core API (registering tools, `ToolResult`, arguments,
transports) all live there and are unchanged here. Everything below is just
what's different in the async build.

## What's different

- **`run()` uses a worker thread pool.** Incoming requests are dispatched to
  workers, so multiple tool calls run at the same time instead of blocking each
  other. `setMaxThreads(n)` sets the worker count (`0`, the default, is one per
  hardware thread).
- **The handshake and cancellations are handled inline** on the read thread, so
  `initialize` always completes before later requests are dispatched, and
  `notifications/cancelled` takes effect immediately instead of waiting behind a
  busy worker.
- **Output is serialized.** Replies and notifications from different threads go
  out through a mutex, so they never interleave on stdout.
- **Tool handlers can be cancelled.** A handler registered with `addToolCtx`
  gets a `RequestContext` with `cancelled()` to poll.
- **It can serve over HTTP** (Streamable HTTP, 2026-07-28) as well as stdio.
  See [Serving over HTTP](#serving-over-http).

## Using it

Register a long-running tool with `addToolCtx` and poll `ctx.cancelled()`:

```cpp
#include "mcp_server.h"

int main() {
    mcpp_async::Server server("my-server", "1.0.0");
    server.setMaxThreads(4);   // 0 (default) = one worker per hardware thread

    mcpp_async::Tool crunch("crunch", "A long-running job");
    crunch.addParameter("n", mcpp_async::PropertyType::Integer, "iterations");

    // addToolCtx gives the handler a RequestContext (progress, log, cancel).
    server.addToolCtx(crunch, [](const rapidjson::Value& args,
                                 mcpp_async::RequestContext& ctx) {
        long long n = mcpp_async::args::integer(args, "n");
        for (long long i = 0; i < n; ++i) {
            if (ctx.cancelled()) { // client sent notifications/cancelled
                return mcpp_async::ToolResult::error("cancelled");
            }
            ctx.progress((double)i, (double)n); // optional progress updates
            // ... do some work ...
        }
        return mcpp_async::ToolResult::text("done");
    });

    return server.run();
}
```

Plain `addTool` handlers still work exactly as in the base repo — they just
can't be cancelled because there is no `RequestContext`.

## Notes

- **Handlers must be thread-safe.** With more than one worker, handlers run
  concurrently. If a handler touches shared mutable state, please guard it yourself.
  Call `setMaxThreads(1)` to force a single worker (serial execution) if you
  want the sync behaviour back.
- **Cancellation is cooperative.** `ctx.cancelled()` only returns `true` if you
  poll it, the server never force-kills a running handler. A handler that never
  checks won't stop early.
- **Cancellation needs `addToolCtx`.** Plain `addTool` handlers have no
  `RequestContext`, so they can't observe cancellation.
- **`run()` owns the threads.** Call it from one thread, don't run the same
  `Server` from several.
- **Elicitation doesn't tie up a worker.** Asking the user something (see the
  base repo) returns straight away, and the client's follow-up call can land on
  any thread. Whatever the tool needs on the second run goes in
  `withRequestState()`, not in a member.
- **Deeply nested messages are refused.** Anything nested more than 128
  levels gets an error instead of being parsed, on stdio and HTTP alike. The
  parser is recursive, so without this one crafted message could crash the
  server.
- **Request state is client input.** The client sends `requestState()` back and
  can change it, so treat it like any other argument: don't put anything in it
  the client mustn't alter, and re-check what matters on the second run.

## Serving over HTTP

Add `mcp_http.cpp` to your build and hand the server to an `HttpServer`:

```cpp
#include "mcp_server.h"
#include "mcp_http.h"

int main() {
    mcpp_async::Server server("my-server", "1.0.0");
    // ... add tools ...

    mcpp_async::HttpOptions opts;
    opts.host = "0.0.0.0";
    opts.port = 8080;
    opts.authorize = [](const mcpp_async::HttpRequest& req,
                        std::string& principal, std::string& challenge) {
        if (req.header("Authorization") == "Bearer secret") {
            principal = "user-1";
            return true;
        }
        challenge = "Bearer resource_metadata=\"https://example.com/.well-known/oauth-protected-resource\"";
        return false;
    };

    mcpp_async::HttpServer http(server, opts);
    return http.listen() ? 0 : 1;
}
```

What it does:

- **One endpoint, POST only.** `opts.path` (default `/mcp`). GET and DELETE get
  405, as the 2026-07-28 spec asks.
- **`tools/call` streams.** When the client accepts `text/event-stream`, progress
  and log lines from `ctx` go out as SSE events before the result. Everything
  else gets a plain JSON reply.
- **Closing the stream cancels.** If the client hangs up mid-call,
  `ctx.cancelled()` turns true.
- **Headers are checked.** For 2026-07-28 requests, `MCP-Protocol-Version`,
  `Mcp-Method` and `Mcp-Name` must match the body, otherwise the reply is 400
  with `HeaderMismatch` (-32020).
- **Origin is checked.** A request whose `Origin` isn't in `allowedOrigins` gets
  403. With an empty list only localhost origins pass. Requests with no
  `Origin` (server to server) pass.
- **Auth runs first.** `authorize` sees every request before anything is
  parsed. Return true to let it through, and whatever you put in `principal`
  reaches tools as `ctx.principal()`. Return false for a 401 with `challenge`
  as the `WWW-Authenticate` header. If you can't check right now, for example
  because your token service is down, throw `mcpp_async::Unavailable(seconds)`
  for a 503 with `Retry-After`, so clients wait instead of asking the user to
  sign in again. Leave it unset for no auth, but then
  anyone who can reach the port can call every tool, so keep `host` at
  `127.0.0.1`.

Limits:

- **No TLS.** Put a reverse proxy (Caddy, nginx) in front for HTTPS.
- **No server-wide notifications.** `sendNotification` and
  `notifyToolsListChanged` reach stdio clients only, and `subscriptions/listen`
  isn't implemented.
- **Older clients work without sessions.** Clients that start with `initialize`
  are served, but they share one `Server`, so the initialized flag and log
  level are shared too.
- **`setMaxThreads` doesn't apply.** HTTP requests run on cpp-httplib's own
  pool. Set `opts.threads` to size it.

## Build requirements

C++11 or later, plus the threading part of the standard library (`<thread>`,
`<mutex>`, `<atomic>`, `<condition_variable>`). On GCC/Clang, link pthreads
(`-pthread`) while on MSVC nothing extra is needed. RapidJSON is the only other
dependency for stdio. HTTP adds cpp-httplib (vendored in `httplib/`) and, on
Windows, `ws2_32.lib`. Aside from the Windows binary-mode setup in the stdio
transport (guarded by an `#ifdef`), the source builds on Windows, Linux, and macOS.

## License

Same as the base repo. RapidJSON and cpp-httplib keep their own licenses (both
MIT), only their headers are vendored.
