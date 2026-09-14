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

## Build requirements

C++11 or later, plus the threading part of the standard library (`<thread>`,
`<mutex>`, `<atomic>`, `<condition_variable>`). On GCC/Clang, link pthreads
(`-pthread`) while on MSVC nothing extra is needed. RapidJSON is the only other
dependency. Aside from the Windows binary-mode setup in the stdio transport (guarded
by an `#ifdef`), the source builds on Windows, Linux, and macOS.

## License

Same as the base repo. RapidJSON keeps its own license (MIT), only its headers
are vendored.
