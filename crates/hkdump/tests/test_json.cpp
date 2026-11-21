#include <catch2/catch_test_macros.hpp>

#include "Json.h"

TEST_CASE("Json::Writer pretty prints with 2-space indent", "[json]") {
    Json::Writer w;
    w.begin_object();
    w.key("name").value("hkAabb");
    w.key("version").value(std::int64_t{0});
    w.key("vtable").value(false);
    w.key("items").begin_array();
    w.begin_object();
    w.key("value").value(std::int64_t{-1});
    w.end_object();
    w.end_array();
    w.end_object();

    CHECK(w.finish() == R"({
  "name": "hkAabb",
  "version": 0,
  "vtable": false,
  "items": [
    {
      "value": -1
    }
  ]
}
)");
}

TEST_CASE("Json::Writer writes empty containers inline", "[json]") {
    Json::Writer w;
    w.begin_object();
    w.key("members").begin_array();
    w.end_array();
    w.key("obj").begin_object();
    w.end_object();
    w.end_object();

    CHECK(w.finish() == "{\n  \"members\": [],\n  \"obj\": {}\n}\n");
}

TEST_CASE("Json::Writer escapes strings", "[json]") {
    Json::Writer w;
    w.value("a\"b\\c\nd\te\x01");
    CHECK(w.finish() == "\"a\\\"b\\\\c\\nd\\te\\u0001\"\n");
}
