#include "convert.hpp"

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "components/document/mutable/mutable_array.h"
#include "components/document/mutable/mutable_dict.h"

#include "components/document/document_view.hpp"
#include "components/ql/expr.hpp"
#include "components/ql/parser.hpp"

// The bug related to the use of RTTI by the pybind11 library has been fixed: a
// declaration should be in each translation unit.
PYBIND11_DECLARE_HOLDER_TYPE(T, boost::intrusive_ptr<T>)

using components::document::document_ptr;
using components::document::document_view_t;

::document::retained_const_t<::document::impl::value_t> to_(const py::handle& obj) {
    if (py::isinstance<py::bool_>(obj)) {
        return ::document::impl::new_value(obj.cast<bool>());
    }
    if (py::isinstance<py::int_>(obj)) {
        return ::document::impl::new_value(obj.cast<long>());
    }
    if (py::isinstance<py::float_>(obj)) {
        return ::document::impl::new_value(obj.cast<double>());
    }
    if (py::isinstance<py::bytes>(obj)) {
        py::module base64 = py::module::import("base64");
        return ::document::impl::new_value(base64.attr("b64encode")(obj).attr("decode")("utf-8").cast<std::string>());
    }
    if (py::isinstance<py::str>(obj)) {
        return ::document::impl::new_value(obj.cast<std::string>());
    }
    if (py::isinstance<py::tuple>(obj) || py::isinstance<py::list>(obj)) {
        auto out = ::document::impl::mutable_array_t::new_array();
        for (const py::handle value : obj) {
            out->append(to_(value));
        }
        return out->as_array();
    }
    if (py::isinstance<py::dict>(obj)) {
        auto out = ::document::impl::mutable_dict_t::new_dict();
        for (const py::handle key : obj) {
            out->set(py::str(key).cast<std::string>(), to_(obj[key]));
        }
        return out->as_dict();
    }
}

py::object from_(const ::document::impl::value_t* value) {
    using document::impl::value_type;
    if (!value) {
        return py::none();
    } else if (value->type() == value_type::boolean) {
        return py::bool_(value->as_bool());
    } else if (value->is_unsigned()) {
        return py::int_(value->as_unsigned());
    } else if (value->is_int()) {
        return py::int_(value->as_int());
    } else if (value->is_double()) {
        return py::float_(value->as_double());
    } else if (value->type() == value_type::string) {
        return py::str(value->as_string().as_string());
    } else if (value->type() == value_type::array) {
        py::list list;
        for (uint32_t i = 0; i < value->as_array()->count(); ++i) {
            list.append(from_(value->as_array()->get(i)));
        }
        return std::move(list);
    } else if (value->type() == value_type::dict) {
        py::dict dict;
        for (auto it = value->as_dict()->begin(); it; ++it) {
            auto key = static_cast<std::string>(it.key()->as_string());
            dict[py::str(key)] = from_(it.value());
        }
        return std::move(dict);
    }
    return py::none();
}

auto to_document(const py::handle& source) -> components::document::document_ptr {
    return components::document::make_document(to_(source)->as_dict());
}

auto from_document(const document_view_t& document) -> py::object {
    return from_(document.get_value());
}

auto from_object(const document_view_t& document, const std::string& key) -> py::object {
    return from_(document.get(key));
}

auto from_object(const document_view_t& document, uint32_t index) -> py::object {
    return from_(document.get(index));
}

auto to_pylist(const std::vector<std::string>& src) -> py::list {
    py::list res;
    for (const auto& str : src) {
        res.append(str);
    }
    return res;
}

auto to_pylist(const std::vector<components::document::document_id_t>& src) -> py::list {
    py::list res;
    for (const auto& str : src) {
        res.append(str.to_string());
    }
    return res;
}

auto to_sorter(const py::handle& sort_dict) -> services::storage::sort::sorter_t {
    services::storage::sort::sorter_t sorter;
    for (const py::handle key : sort_dict) {
        sorter.add(py::str(key).cast<std::string>(), to_order(sort_dict[key]));
    }
    return sorter;
}

auto to_order(const py::object& order) -> services::storage::sort::order {
    return py::int_(order).cast<int>() < 0
               ? services::storage::sort::order::descending
               : services::storage::sort::order::ascending;
}

namespace experimental {
    using components::ql::aggregate_statement;
    using components::ql::expr_ptr;
    using components::ql::expr_t;
    using components::ql::make_aggregate_operator;
    using components::ql::make_union_expr;
    using components::ql::to_string;
    using components::ql::from_string;
    using components::ql::aggregate_op_steps;
    using components::ql::make_expr;
    using components::ql::get_condition_type_;

    auto to_v2_(const py::handle& obj) -> ::document::retained_const_t<::document::impl::value_t> {
        if (py::isinstance<py::bool_>(obj)) {
            return ::document::impl::new_value(obj.cast<bool>());
        }
        if (py::isinstance<py::int_>(obj)) {
            return ::document::impl::new_value(obj.cast<long>());
        }
        if (py::isinstance<py::float_>(obj)) {
            return ::document::impl::new_value(obj.cast<double>());
        }
        if (py::isinstance<py::bytes>(obj)) {
            py::module base64 = py::module::import("base64");
            return ::document::impl::new_value(base64.attr("b64encode")(obj).attr("decode")("utf-8").cast<std::string>());
        }
        if (py::isinstance<py::str>(obj)) {
            return ::document::impl::new_value(obj.cast<std::string>());
        }
    }

    void parse_condition_(expr_t* parent_condition, const py::handle& condition, const std::string& prev_key, const std::string& key_word);
    void parse_condition_dict_(expr_t* parent_condition, const py::handle& condition, const std::string& prev_key);
    void parse_condition_array_(expr_t* parent_condition, const py::handle& condition, const std::string& prev_key);

    void parse_condition_(components::ql::expr_t* parent_condition, const py::handle& condition, const std::string& prev_key, const std::string& key_word) {

        auto real_key = prev_key;
        auto type = from_string(key_word);

        if (type == aggregate_op_steps::novalid) {
            type = from_string(prev_key);
            if (type != aggregate_op_steps::novalid) {
                real_key = key_word;
            }
        }

        if (py::isinstance<py::dict>(condition)) {
            parse_condition_dict_(parent_condition, condition, real_key);
        } else if (py::isinstance<py::list>(condition)) {
            parse_condition_array_(parent_condition, condition, real_key);
        } else {
            auto sub_condition = make_expr(type, real_key, ::document::impl::new_value(to_v2_(condition)).detach());
            if (sub_condition->is_union()) {
                parse_condition_(sub_condition.get(), condition, real_key, std::string());
            }
            parent_condition->append_sub_condition(std::move(sub_condition));
        }
    }

    void parse_condition_dict_(expr_t* parent_condition, const py::handle& condition, const std::string& prev_key) {
        for (const py::handle i :condition) {
            auto key = py::str(i).cast<std::string>();
            auto type = get_condition_type_(key);
            auto union_condition = parent_condition;
            if (is_union_condition(type)) {
                parent_condition->append_sub_condition(make_union_expr());
                union_condition = parent_condition->sub_conditions_.at(parent_condition->sub_conditions_.size() - 1).get();
                union_condition->type_ = type;
            }
            if (prev_key.empty()) {
                parse_condition_(union_condition,i[key], key, std::string());
            } else {
                parse_condition_(union_condition, i[key], prev_key, key);
            }
        }
    }

    void parse_condition_array_(expr_t* parent_condition, const py::handle& condition, const std::string& prev_key) {
        for (const py::handle it :condition) {
            parse_condition_(parent_condition, it, prev_key, std::string());
        }
    }

    expr_ptr parse_condition_(const py::handle& condition) {
        auto res_condition = make_union_expr();
        for (const py::handle it :condition) {
            if (py::len(condition) == 1) {
                res_condition->type_ = get_condition_type_(py::str(it).cast<std::string>());
            }
            parse_condition_(res_condition.get(), it, py::str(it).cast<std::string>(), std::string());
        }
        if (res_condition->sub_conditions_.size() == 1) {
            return std::move(res_condition->sub_conditions_[0]);
        }
        return res_condition;
    }

    auto to_statement(const py::handle& source, aggregate_statement* aggregate) -> void {
        auto is_sequence = py::isinstance<py::sequence>(source);

        if (!is_sequence) {
            throw py::type_error(" not list ");
        }

        auto size = py::len(source);

        for (const py::handle obj : source) {
            auto is_mapping = py::isinstance<py::dict>(obj);
            if (!is_mapping) {
                throw py::type_error(" not mapping ");
            }

            for (const py::handle key : obj) {
                auto name = py::str(key).cast<std::string>();
                auto op = parse_condition_(obj[key]);
                aggregate->append(make_aggregate_operator(name, std::move(op)));
            }
        }
    }

} // namespace experimental
