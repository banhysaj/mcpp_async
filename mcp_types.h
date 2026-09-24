#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mcpp_async {
	template<typename T>
	using vector = std::vector<T>;

	enum ErrorCode {
		kParseError = -32700,
		kInvalidRequest = -32600,
		kMethodNotFound = -32601,
		kInvalidParams = -32602,
		kInternalError = -32603,
		kMissingRequiredClientCapability = -32021,
		kUnsupportedProtocolVersion = -32022
	};

	enum class PropertyType {
		String,
		Number,
		Integer,
		Boolean,
		Object,
		Array
	};

	struct Content {
		enum class Kind { Text, Image, Audio, ResourceLink, EmbeddedResource };

		Kind kind;
		std::string text;
		std::string data;
		std::string blob;
		std::string mimeType;
		std::string uri;
		std::string name;
		std::string description;

		Content() : kind(Content::Kind::Text) {}

		static Content makeText(const std::string& t);
		static Content makeImage(const std::string& base64Data, const std::string& mime);
		static Content makeAudio(const std::string& base64Data, const std::string& mime);
		static Content makeResourceLink(const std::string& uri, const std::string& name, const std::string& mime = std::string(), const std::string& description = std::string());
		static Content makeEmbeddedText(const std::string& uri, const std::string& text, const std::string& mime = std::string());
		static Content makeEmbeddedBlob(const std::string& uri, const std::string& base64, const std::string& mime = std::string());
	};

	struct ToolParameter {
		std::string name;
		PropertyType type;
		std::string description;
		bool required;
		std::string itemType; //the element type when the type is array
		vector<std::string> enumValues;

		ToolParameter() : type(PropertyType::String), required(true) {}
		ToolParameter(const std::string& n, PropertyType t, const std::string& d, bool req = true) : name(n), type(t), description(d), required(req) {}

		ToolParameter& setEnum(const vector<std::string>& v) {
			enumValues = v;
			return *this;
		}
		ToolParameter& setItemType(const std::string& t) {
			itemType = t;
			return *this;
		}
	};

	struct Elicitation {
		std::string key; // names the answer then the tool runs for the second time
		std::string message;
		std::string url; // for url mode
		std::string schema; //raw json for the form, optional
		vector<ToolParameter> fields; // flat primitives only, string num, int, bool, enums

		static Elicitation makeForm(const std::string& key, const std::string& message);
		static Elicitation makeUrl(const std::string& key, const std::string& message, const std::string& url);

		bool isUrl() const { return !url.empty(); }
		Elicitation& addField(const ToolParameter& param) {
			fields.push_back(param);
			return *this;
		}
		Elicitation& addField(const std::string& n, PropertyType t, const std::string& d, bool req = true) {
			fields.push_back(ToolParameter(n, t, d, req));
			return *this;
		}
		Elicitation& setSchema(const std::string& rawJson) {
			schema = rawJson;
			return *this;
		}
	};

	struct ToolResult {
		vector<Content> content;
		bool isError;
		std::string structuredContent;
		bool hasStructuredContent;
		vector<Elicitation> elicitations; // ask the user firstm the tool will run again with the answers
		std::string requestState;

		ToolResult() : isError(false), hasStructuredContent(false) {}

		static ToolResult text(const std::string& t);   // A single text block
		static ToolResult error(const std::string& message);
		static ToolResult elicit(const Elicitation& e);
		ToolResult& addElicitation(const Elicitation& e) {
			elicitations.push_back(e);
			return *this;
		}
		ToolResult& withRequestState(const std::string& state) {
			requestState = state;
			return *this;
		}

		ToolResult& add(const Content& c) {
			content.push_back(c); return *this;
		}
		ToolResult& addText(const std::string& t) {
			content.push_back(Content::makeText(t));
			return *this;
		}
		ToolResult& withStructured(const std::string& rawJson) {
			structuredContent = rawJson; hasStructuredContent = true;
			return *this;
		}
		ToolResult& markError(bool e = true) {
			isError = e;
			return *this;
		}
	};

	struct Tool {
		std::string name;
		std::string title;
		std::string description;
		vector<ToolParameter> properties;
		std::string customInputSchema; //When set, custom json overrides the generated schema
		std::string outputSchema; //Optional raw json

		Tool() {}
		Tool(const std::string& n, const std::string& d) : name(n), description(d) {}

		Tool& setTitle(const std::string& t) {
			title = t;
			return *this;
		}
		Tool& setDescription(const std::string& d) {
			description = d;
			return *this;
		}
		Tool& setInputSchema(const std::string& rawJson) {
			customInputSchema = rawJson;
			return *this;
		}
		Tool& setOutputSchema(const std::string& rawJson) {
			outputSchema = rawJson;
			return *this;
		}

		Tool& addParameter(const ToolParameter& p) {
			properties.push_back(p);
			return *this;
		}
		Tool& addParameter(const std::string& n, PropertyType t, const std::string& d, bool req = true) {
			properties.push_back(ToolParameter(n, t, d, req));
			return *this;
		}

		Tool& addEnumParam(const std::string& n, const vector<std::string>& values, const std::string& d, bool req = true) {
			ToolParameter p(n, PropertyType::String, d, req);
			p.setEnum(values);
			return addParameter(p);
		}
	};

	struct Resource {
		std::string uri;
		std::string name;
		std::string title;
		std::string description;
		std::string mimeType;
		int64_t size; //Size in bytes, optional

		Resource() : size(0) {}
		Resource(const std::string& u, const std::string& n) : uri(u), name(n) {}
	};

	struct ResourceContent {
		std::string uri;
		std::string mimeType;
		std::string text; //If non-empty, and blob is empty, sent as text
		std::string blob; //Else, sent as base64 "blob"
		ResourceContent() {}
	};
}