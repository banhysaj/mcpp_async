#include "mcp_constants.h"

#include <cstddef>
#include <vector>

namespace mcpp_async {

    const char* const kProtocolVersionLatest = "2026-07-28";
    const char* const kMetaProtocolVersion = "io.modelcontextprotocol/protocolVersion";
    const char* const kMetaClientInfo = "io.modelcontextprotocol/clientInfo";
    const char* const kMetaClientCapabilities = "io.modelcontextprotocol/clientCapabilities";
    const char* const kMetaServerInfo = "io.modelcontextprotocol/serverInfo";

    namespace {
        // Published MCP protocol revisions (newest first)
        const char* const kStatelessVersions[] = { "2026-07-28" };
        const char* const kStatefulVersions[] = { "2025-11-25", "2025-06-18", "2025-03-26", "2024-11-05" };

        template <std::size_t N>
        bool inList(const char* const (&arr)[N], const std::string& v) {
            for (std::size_t i = 0; i < N; ++i) if (v == arr[i]) return true;
            return false;
        }
    }

    bool isStatelessVersion(const std::string& v) { return inList(kStatelessVersions, v); }
    bool isStatefulVersion(const std::string& v) { return inList(kStatefulVersions, v); }
    const char* defaultStatefulVersion() { return kStatefulVersions[0]; }

    std::vector<std::string> supportedProtocolVersions() {
        std::vector<std::string> out;
        for (std::size_t i = 0; i < sizeof(kStatelessVersions) / sizeof(kStatelessVersions[0]); ++i) {
            out.push_back(kStatelessVersions[i]);
        }
        for (std::size_t i = 0; i < sizeof(kStatefulVersions) / sizeof(kStatefulVersions[0]); ++i) {
            out.push_back(kStatefulVersions[i]);
        }
        return out;
    }

  ////////////////////////// MCP Protocol //////////////////////

  // Methods
  const char* const mtdInitialize = "initialize";
  const char* const mtdPing = "ping";
  const char* const mtdServerDiscover = "server/discover";

  const char* const mtdToolsList = "tools/list";
  const char* const mtdToolsCall = "tools/call";

  const char* const mtdResourcesList = "resources/list";
  const char* const mtdResourcesTemplatesList = "resources/templates/list";
  const char* const mtdResourcesRead = "resources/read";
  const char* const mtdResourcesSubscribe = "resources/subscribe";
  const char* const mtdResourcesUnsubscribe = "resources/unsubscribe";

  const char* const mtdPromptsList = "prompts/list";
  const char* const mtdPromptsGet = "prompts/get";

  const char* const mtdCompletionComplete = "completion/complete";
  const char* const mtdLoggingSetLevel = "logging/setLevel";

  const char* const mtdRootsList = "roots/list";
  const char* const mtdSamplingCreateMessage = "sampling/createMessage";
  const char* const mtdElicitationCreate = "elicitation/create";
  

  // Notifications
  const char* const ntfInitialized = "notifications/initialized";
  const char* const ntfCancelled = "notifications/cancelled";
  const char* const ntfProgress = "notifications/progress";
  const char* const ntfMessage = "notifications/message";
  const char* const ntfResourcesUpdated = "notifications/resources/updated";
  const char* const ntfResourcesListChanged = "notifications/resources/list_changed";
  const char* const ntfToolsListChanged = "notifications/tools/list_changed";
  const char* const ntfPromptsListChanged = "notifications/prompts/list_changed";
  const char* const ntfRootsListChanged = "notifications/roots/list_changed";


  // JSON field names
  const char* const fldJsonrpc = "jsonrpc";
  const char* const fldId = "id";
  const char* const fldMethod = "method";
  const char* const fldParams = "params";
  const char* const fldResult = "result";
  const char* const fldError = "error";
  const char* const fldCode = "code";
  const char* const fldMessage = "message";
  const char* const fldData = "data";
  const char* const fldMeta = "_meta";

  const char* const fldProtocolVersion = "protocolVersion";
  const char* const fldCapabilities = "capabilities";
  const char* const fldServerInfo = "serverInfo";
  const char* const fldClientInfo = "clientInfo";
  const char* const fldInstructions = "instructions";
  const char* const fldSupportedVersions = "supportedVersions";
  const char* const fldRequested = "requested";
  const char* const fldSupported = "supported";

  const char* const fldName = "name";
  const char* const fldVersion = "version";
  const char* const fldTitle = "title";
  const char* const fldDescription = "description";

  const char* const fldListChanged = "listChanged";
  const char* const fldSubscribe = "subscribe";

  const char* const fldTools = "tools";
  const char* const fldInputSchema = "inputSchema";
  const char* const fldOutputSchema = "outputSchema";
  const char* const fldArguments = "arguments";
  const char* const fldContent = "content";
  const char* const fldStructuredContent = "structuredContent";
  const char* const fldIsError = "isError";

  const char* const fldType = "type";
  const char* const fldProperties = "properties";
  const char* const fldRequired = "required";
  const char* const fldItems = "items";
  const char* const fldEnum = "enum";

  const char* const fldText = "text";
  const char* const fldBlob = "blob";
  const char* const fldMimeType = "mimeType";
  const char* const fldUri = "uri";
  const char* const fldResource = "resource";

  const char* const fldResources = "resources";
  const char* const fldResourceTemplates = "resourceTemplates";
  const char* const fldUriTemplate = "uriTemplate";
  const char* const fldContents = "contents";
  const char* const fldSize = "size";

  const char* const fldPrompts = "prompts";
  const char* const fldMessages = "messages";
  const char* const fldRole = "role";

  const char* const fldCursor = "cursor";
  const char* const fldNextCursor = "nextCursor";

  const char* const fldCompletion = "completion";
  const char* const fldRef = "ref";
  const char* const fldArgument = "argument";
  const char* const fldValue = "value";
  const char* const fldValues = "values";
  const char* const fldTotal = "total";
  const char* const fldHasMore = "hasMore";

  const char* const fldProgressToken = "progressToken";
  const char* const fldProgress = "progress";
  const char* const fldRequestId = "requestId";
  const char* const fldReason = "reason";
  const char* const fldLevel = "level";
  const char* const fldLogger = "logger";

  const char* const fldResultType = "resultType";
  const char* const fldTtlMs = "ttlMs";
  const char* const fldCacheScope = "cacheScope";



  // Fixed string values
  const char* const kJsonRpcVersion = "2.0";
  const char* const kResultTypeComplete = "complete";
  const char* const valTypeText = "text";
  const char* const valTypeImage = "image";
  const char* const valTypeAudio = "audio";
  const char* const valTypeResourceLink = "resource_link";
  const char* const valTypeResource = "resource";

  const char* const valRoleUser = "user";
  const char* const valRoleAssistant = "assistant";

  const char* const valSchemaString = "string";
  const char* const valSchemaNumber = "number";
  const char* const valSchemaInteger = "integer";
  const char* const valSchemaBoolean = "boolean";
  const char* const valSchemaObject = "object";
  const char* const valSchemaArray = "array";

}
