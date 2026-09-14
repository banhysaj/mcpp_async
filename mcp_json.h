#pragma once

#include <string>
#include <cstdint>

#ifndef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1
#endif


#include "rapidjson/include/rapidjson/document.h"

namespace rj = rapidjson;
namespace mcpp_async {
    // Read side
    namespace args {

        bool has(const rj::Value& v, const char* key);
        const rj::Value* get(const rj::Value& v, const char* key);
        std::string str(const rj::Value& v, const char* key, const std::string& def = {} );
        int64_t integer( const rj::Value& v, const char* key, int64_t def = 0);
        double number(const rj::Value& v, const char* key, double def = 0.0);
        bool boolean(const rj::Value& v, const char* key, bool def = false);
    }

    // Write side
    namespace json {
        typedef rj::Document::AllocatorType Alloc;

        void addStr(rj::Value& o, const char* k, const std::string& s, Alloc& a);
        void addInt(rj::Value& o, const char* k, long long n, Alloc& a);
        void addBool(rj::Value& o, const char* k, bool b, Alloc& a);
        void addVal(rj::Value& o, const char* k, rj::Value& v, Alloc& a);              // moves v
        void addKeyVal(rj::Value& o, const std::string& key, rj::Value& v, Alloc& a);  // runtime key, moves v
        void pushStr(rj::Value& arr, const std::string& s, Alloc& a);
        std::string serialize(const rj::Value& v);
        bool parseInto(const std::string& raw, rj::Value& out, Alloc& a);              // parse into a value owned by 'a'
    }
}