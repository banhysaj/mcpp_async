#include "mcp_types.h"

#include "rapidjson/include/rapidjson/document.h"

#include <string>

namespace rj = rapidjson;

namespace mcpp_async {

	Content Content::makeText(const std::string& t) {
		Content c;
		c.kind = Content::Kind::Text;
		c.text = t;
		return c;
	}
	Content Content::makeImage(const std::string& d, const std::string& m) {
		Content c;
		c.kind = Content::Kind::Image;
		c.data = d;
		c.mimeType = m;
		return c;
	}
	Content Content::makeAudio(const std::string& d, const std::string& m) {
		Content c;
		c.kind = Content::Kind::Audio;
		c.data = d;
		c.mimeType = m;
		return c;
	}
	Content Content::makeResourceLink(const std::string& uri, const std::string& name, const std::string& mime, const std::string& desc) {
		Content c;
		c.kind = Content::Kind::ResourceLink;
		c.uri = uri;
		c.name = name;
		c.mimeType = mime;
		c.description = desc;
		return c;
	}
	Content Content::makeEmbeddedText(const std::string& uri, const std::string& text, const std::string& mime) {
		Content c;
		c.kind = Content::Kind::EmbeddedResource;
		c.uri = uri;
		c.text = text;
		c.mimeType = mime;
		return c;
	}
	Content Content::makeEmbeddedBlob(const std::string& uri, const std::string& base64, const std::string& mime) {
		Content c;
		c.kind = Content::Kind::EmbeddedResource;
		c.uri = uri;
		c.blob = base64;
		c.mimeType = mime;
		return c;
	}

	ToolResult ToolResult::text(const std::string& t) {
		ToolResult r;
		r.content.push_back(Content::makeText(t));
		return r;
	}
	ToolResult ToolResult::error(const std::string& m) {
		ToolResult r;
		r.isError = true;
		r.content.push_back(Content::makeText(m));
		return r;
	}
	ToolResult ToolResult::elicit(const Elicitation& e) {
		ToolResult r;
		r.elicitations.push_back(e);
		return r;
	}

	Elicitation Elicitation::makeForm(const std::string& key, const std::string& message) {
		Elicitation e;
		e.key = key;
		e.message = message;
		return e;
	}
	Elicitation Elicitation::makeUrl(const std::string& key, const std::string& message, const std::string& url) {
		Elicitation e;
		e.key = key;
		e.message = message;
		e.url = url;
		return e;
	}
}
