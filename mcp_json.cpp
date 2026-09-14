#include "mcp_json.h"
#include "rapidjson/include/rapidjson/writer.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"

namespace mcpp_async {
    namespace {
        const rj::Value* member(const rj::Value& v, const char* k) {
            if (!v.IsObject()) return 0;
            rj::Value::ConstMemberIterator it = v.FindMember(k);
            if (it == v.MemberEnd()) return 0;
            return &it->value;
        }
    }

    namespace args {
        bool has(const rj::Value& v, const char* key) {
            return member(v, key) != 0;
        }

        const rj::Value* get(const rj::Value& v, const char* key) { 
            return member(v, key); 
        }

        std::string str(const rj::Value& v, const char* key, const std::string& def) {
            const rj::Value* m = member(v, key);
            if (m && m->IsString()) {
                return std::string(m->GetString(), m->GetStringLength());
            }
            return def;
        }
        int64_t integer(const rj::Value& v, const char* key, int64_t def) {
            const rj::Value* m = member(v, key);
            if (!m) return def;
            if (m->IsInt64())  return m->GetInt64();
            if (m->IsUint64()) return static_cast<int64_t>(m->GetUint64());
            if (m->IsDouble()) return static_cast<int64_t>(m->GetDouble());
            return def;
        }
        double number(const rj::Value& v, const char* key, double def) {
            const rj::Value* m = member(v, key);
            if (m && m->IsNumber()) {
                return m->GetDouble();
            }
            return def;
        }
        bool boolean(const rj::Value& v, const char* key, bool def) {
            const rj::Value* m = member(v, key);
            if (m && m->IsBool()) {
                return m->GetBool();
            }
            return def;
        }
        
    }

    namespace json {
        void addStr(rj::Value& o, const char* k, const std::string& s, Alloc& a) {
            rj::Value v(s, a);
            o.AddMember(rj::StringRef(k), v, a);
        }
        void addInt(rj::Value& o, const char* k, long long n, Alloc& a) {
            rj::Value v(static_cast<int64_t>(n)); o.AddMember(rj::StringRef(k), v, a);
        }
        void addBool(rj::Value& o, const char* k, bool b, Alloc& a) {
            rj::Value v(b); o.AddMember(rj::StringRef(k), v, a);
        }
        void addVal(rj::Value& o, const char* k, rj::Value& v, Alloc& a) {
            o.AddMember(rj::StringRef(k), v, a);   // moves v
        }
        void addKeyVal(rj::Value& o, const std::string& key, rj::Value& v, Alloc& a) {
            rj::Value k(key, a); o.AddMember(k, v, a);
        }
        void pushStr(rj::Value& arr, const std::string& s, Alloc& a) {
            rj::Value v(s, a); arr.PushBack(v, a);
        }
        std::string serialize(const rj::Value& v) {
            rj::StringBuffer sb;
            rj::Writer<rj::StringBuffer> w(sb);
            v.Accept(w);
            return std::string(sb.GetString(), sb.GetSize());
        }
        bool parseInto(const std::string& raw, rj::Value& out, Alloc& a) {
            rj::Document d;
            d.Parse(raw.c_str());
            if (d.HasParseError()) return false;
            out.CopyFrom(d, a);
            return true;
        }
    }
}