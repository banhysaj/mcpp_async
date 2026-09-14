#pragma once

#include <string>
#include <vector>

namespace mcpp_async {

	extern const char* const kProtocolVersionLatest;

	// _meta keys
	extern const char* const kMetaProtocolVersion;
	extern const char* const kMetaClientInfo;
	extern const char* const kMetaClientCapabilities;
	extern const char* const kMetaServerInfo;

	// Protocol-version negotiation. Separate between stateless and stateful, as of now
	// the latest version is the only one that is stateless
	// "stateless" = 2026-07-28 (no handshake), "stateful" = all the ones before this
	bool isStatelessVersion(const std::string& v);
	bool isStatefulVersion(const std::string& v);
	const char* defaultStatefulVersion(); // negotiated when the client asks for none
	std::vector<std::string> supportedProtocolVersions(); // stateless first, then stateful

  ///////////////////// MCP PROTOCOL //////////////////////

  //methods
  extern const char* const mtdInitialize;
  extern const char* const mtdPing;
  extern const char* const mtdServerDiscover;

  extern const char* const mtdToolsList;
  extern const char* const mtdToolsCall;

  extern const char* const mtdResourcesList;
  extern const char* const mtdResourcesTemplatesList;
  extern const char* const mtdResourcesRead;
  extern const char* const mtdResourcesSubscribe;
  extern const char* const mtdResourcesUnsubscribe;

  extern const char* const mtdPromptsList;
  extern const char* const mtdPromptsGet;

  extern const char* const mtdCompletionComplete;
  extern const char* const mtdLoggingSetLevel;

  extern const char* const mtdRootsList;
  extern const char* const mtdSamplingCreateMessage;
  extern const char* const mtdElicitationCreate;


  // Notifications
  extern const char* const ntfInitialized;
  extern const char* const ntfCancelled;
  extern const char* const ntfProgress;
  extern const char* const ntfMessage;
  extern const char* const ntfResourcesUpdated;
  extern const char* const ntfResourcesListChanged;
  extern const char* const ntfToolsListChanged;
  extern const char* const ntfPromptsListChanged;
  extern const char* const ntfRootsListChanged;


  // JSON field names
  extern const char* const fldJsonrpc;
  extern const char* const fldId;
  extern const char* const fldMethod;
  extern const char* const fldParams;
  extern const char* const fldResult;
  extern const char* const fldError;
  extern const char* const fldCode;
  extern const char* const fldMessage;
  extern const char* const fldData;
  extern const char* const fldMeta;

  extern const char* const fldProtocolVersion;
  extern const char* const fldCapabilities;
  extern const char* const fldServerInfo;
  extern const char* const fldClientInfo;
  extern const char* const fldInstructions;
  extern const char* const fldSupportedVersions;
  extern const char* const fldRequested;
  extern const char* const fldSupported;

  extern const char* const fldName;
  extern const char* const fldVersion;
  extern const char* const fldTitle;
  extern const char* const fldDescription;

  extern const char* const fldListChanged;
  extern const char* const fldSubscribe;

  extern const char* const fldTools;
  extern const char* const fldInputSchema;
  extern const char* const fldOutputSchema;
  extern const char* const fldArguments;
  extern const char* const fldContent;
  extern const char* const fldStructuredContent;
  extern const char* const fldIsError;

  extern const char* const fldType;
  extern const char* const fldProperties;
  extern const char* const fldRequired;
  extern const char* const fldItems;
  extern const char* const fldEnum;

  extern const char* const fldText;
  extern const char* const fldBlob;
  extern const char* const fldMimeType;
  extern const char* const fldUri;
  extern const char* const fldResource;

  extern const char* const fldResources;
  extern const char* const fldResourceTemplates;
  extern const char* const fldUriTemplate;
  extern const char* const fldContents;
  extern const char* const fldSize;

  extern const char* const fldPrompts;
  extern const char* const fldMessages;
  extern const char* const fldRole;

  extern const char* const fldCursor;
  extern const char* const fldNextCursor;

  extern const char* const fldCompletion;
  extern const char* const fldRef;
  extern const char* const fldArgument;
  extern const char* const fldValue;
  extern const char* const fldValues;
  extern const char* const fldTotal;
  extern const char* const fldHasMore;

  extern const char* const fldProgressToken;
  extern const char* const fldProgress;
  extern const char* const fldRequestId;
  extern const char* const fldReason;
  extern const char* const fldLevel;
  extern const char* const fldLogger;

  extern const char* const fldResultType;
  extern const char* const fldTtlMs;
  extern const char* const fldCacheScope;



  // Fixed string values
  extern const char* const kJsonRpcVersion;
  extern const char* const kResultTypeComplete;
  extern const char* const valTypeText;
  extern const char* const valTypeImage;
  extern const char* const valTypeAudio;
  extern const char* const valTypeResourceLink;
  extern const char* const valTypeResource;

  extern const char* const valRoleUser;
  extern const char* const valRoleAssistant;

  extern const char* const valSchemaString;
  extern const char* const valSchemaNumber;
  extern const char* const valSchemaInteger;
  extern const char* const valSchemaBoolean;
  extern const char* const valSchemaObject;
  extern const char* const valSchemaArray;
}
