#include <catch2/catch.hpp>

#include "components/document/document.hpp"
#include "components/ql/parser.hpp"

using namespace components::ql;

TEST_CASE("ql::expr") {
    auto expr = make_expr(condition_type::eq, "key1", int64_t(100));
    REQUIRE(to_string(expr) == R"({"key1": {"$eq": 100}})");

    expr = make_expr(condition_type::eq, "key2", std::string("value"));
    REQUIRE(to_string(expr) == R"({"key2": {"$eq": "value"}})");

    expr = make_union_expr();
    expr->type_ = condition_type::union_and;
    expr->append_sub_condition(make_expr(condition_type::eq, "key1", int64_t(100)));
    expr->append_sub_condition(make_expr(condition_type::eq, "key2", std::string("value")));
    REQUIRE(to_string(expr) == R"({"$and": [{"key1": {"$eq": 100}}, {"key2": {"$eq": "value"}}]})");
}

TEST_CASE("ql::parser") {
    std::string value = R"({"count": {"$gt": 10}})";
    auto d = components::document::document_from_json(value);
    auto condition = parse_find_condition(d);
    REQUIRE(to_string(condition) == value);

    value = R"({"$or": [{"count": {"$gt": 10}}, {"count": {"$lte": 50}}]})";
    d = components::document::document_from_json(value);
    condition = parse_find_condition(d);
    REQUIRE(to_string(condition) == value);

    value = R"({"$or": [{"count": {"$gt": 10}}, {"$and": [{"count": {"$lte": 50}}, {"value": {"$gt": 3}}]}]})";
    d = components::document::document_from_json(value);
    condition = parse_find_condition(d);
    REQUIRE(to_string(condition) == value);

    value = R"({"$or": [{"count": {"$gt": 10}}, {"$and": [{"count": {"$lte": 50}}, {"value": {"$gt": 3}}, )"
            R"({"$or": [{"count": {"$gt": 10}}, {"$and": [{"count": {"$lte": 50}}, {"value": {"$gt": 3}}]}]}]}]})";
    d = components::document::document_from_json(value);
    condition = parse_find_condition(d);
    REQUIRE(to_string(condition) == value);

    value = R"({"count": 10})";
    d = components::document::document_from_json(value);
    condition = parse_find_condition(d);
    REQUIRE(to_string(condition) == R"({"count": {"$eq": 10}})");

    value = R"({"count": {"$gt": 40, "$lte": 60}})";
    d = components::document::document_from_json(value);
    condition = parse_find_condition(d);
    REQUIRE(to_string(condition) == R"({"$and": [{"count": {"$gt": 40}}, {"count": {"$lte": 60}}]})");

//    value = R"({"count": {"$not": {"$gte": 90, "$lt": 10}}})";
//    d = components::document::document_from_json(value);
//    condition = parse_find_condition(d);
//    REQUIRE(to_string(condition) == R"({"$not": [{"$and": [{"count": {"$gte": 90}}], {"count": {"$lt": 10}}]}]})");
}