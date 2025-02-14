#include "expr.hpp"
#include <components/document/document_view.hpp>

namespace components::ql {

    bool is_union_condition(condition_type type) {
        return type == condition_type::union_and ||
               type == condition_type::union_or ||
               type == condition_type::union_not;
    }

    std::string to_string(condition_type type) {
        switch (type) {
            case condition_type::eq:
                return "$eq";
            case condition_type::ne:
                return "$ne";
            case condition_type::gt:
                return "$gt";
            case condition_type::lt:
                return "$lt";
            case condition_type::gte:
                return "$gte";
            case condition_type::lte:
                return "$lte";
            case condition_type::regex:
                return "$regex";
            case condition_type::any:
                return "$any";
            case condition_type::all:
                return "$all";
            case condition_type::union_and:
                return "$and";
            case condition_type::union_or:
                return "$or";
            case condition_type::union_not:
                return "$not";
        }
        return {};
    }

    std::string to_string(const expr_ptr& expr) {
        if (expr->is_union()) {
            std::string result = "{\"" + to_string(expr->type_) + "\": [";
            for (std::size_t i = 0; i < expr->sub_conditions_.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += to_string(expr->sub_conditions_.at(i));
            }
            result += "]}";
            return result;
        }
        return "{\"" + expr->key_.as_string() + "\": {\"" + to_string(expr->type_) + "\": " + to_string(expr->value_) + "}}";
    }

    expr_t::expr_t(condition_type type, std::string key, expr_value_t value)
        : type_(type)
        , key_(std::move(key))
        , value_(value)
        , union_(is_union_condition(type_)) {}

    expr_t::expr_t(bool is_union)
        : type_(condition_type::novalid)
        , value_(nullptr)
        , union_(is_union){}

    bool expr_t::is_union() const {
        return union_;
    }

    void expr_t::append_sub_condition(expr_t::ptr sub_condition) {
        sub_conditions_.push_back(std::move(sub_condition));
    }


    ::document::wrapper_value_t get_value(const components::document::document_ptr &doc, const key_t &key) {
        return ::document::wrapper_value_t(components::document::document_view_t(doc).get_value(key.as_string()));
    }

} // namespace components::ql