#include "document.hpp"

#include <boost/json.hpp>
#include <components/document/mutable/mutable_array.h>
#include <components/document/mutable/mutable_dict.h>
#include <components/document/document_view.hpp>

namespace components::document {

    using ::document::impl::mutable_array_t;
    using ::document::impl::mutable_dict_t;
    using ::document::impl::value_type;

    document_view_t::const_value_ptr inc_(document_view_t::const_value_ptr src, document_view_t::const_value_ptr value) {
        if (value->type() == ::document::impl::value_type::number) {
            if (src->is_unsigned()) {
                return ::document::impl::new_value(src->as_unsigned() + value->as_unsigned()).detach();
            } else if (src->is_int()) {
                return ::document::impl::new_value(src->as_int() + value->as_int()).detach();
            } else if (src->is_double()) {
                return ::document::impl::new_value(src->as_double() + value->as_double()).detach();
            }
        }
        //todo error not valid type
        return src;
    }

    document_const_value_t get_value_by_key_(document_const_value_t object, const std::string &key) {
        if (object->type() == value_type::dict) {
            return object->as_dict()->as_mutable()->get(key);
        } else if (object->type() == value_type::array) {
            try {
                return object->as_array()->as_mutable()->get(std::atol(key.c_str()));
            } catch (...) {
            }
        }
        return nullptr;
    }

    void set_new_value_(document_const_value_t object, const std::string &key, document_const_value_t value) {
        auto dot_pos = key.find('.');
        if (dot_pos != std::string::npos) {
            auto key_parent = key.substr(0, dot_pos);
            document_const_value_t object_parent = get_value_by_key_(object, key_parent);
            if (!object_parent) {
                auto dot_pos_next = key.find('.', dot_pos + 1);
                auto key_next = key.substr(dot_pos + 1, (dot_pos_next == std::string::npos ? key.size() : dot_pos_next) - dot_pos - 1);
                if (key_next.find_first_not_of("0123456789") == std::string::npos) {
                    set_new_value_(object, key_parent, mutable_array_t::new_array().detach());
                } else {
                    set_new_value_(object, key_parent, mutable_dict_t::new_dict().detach());
                }
                object_parent = get_value_by_key_(object, key_parent);
                if (!object_parent && object->type() == value_type::array) {
                    object_parent = object->as_array()->get(object->as_array()->count() - 1);
                }
            }
            if (object_parent) {
                set_new_value_(object_parent, key.substr(dot_pos + 1, key.size() - dot_pos), value);
            }
        } else {
            if (object->type() == value_type::dict) {
                object->as_dict()->as_mutable()->set(key, std::move(value));
            } else if (object->type() == value_type::array) {
                try {
                    auto index = uint32_t(std::atol(key.c_str()));
                    if (index < object->as_array()->count()) {
                        object->as_array()->as_mutable()->set(index, value);
                    } else {
                        object->as_array()->as_mutable()->append(value);
                    }
                } catch (...) {
                }
            }
        }
    }

    document_t::document_t()
        : value_(mutable_dict_t::new_dict()) {
    }

    document_t::document_t(document_value_t value)
        : value_(std::move(value)) {
    }

    bool document_t::update(const ptr& update) {
        bool result = false;
        document_view_t view(this);
        auto dict = document_view_t(update).as_dict();
        for (auto it_update = dict->begin(); it_update; ++it_update) {
            auto key_update = static_cast<std::string>(it_update.key()->as_string());
            auto fields = it_update.value()->as_dict();
            for (auto it_field = fields->begin(); it_field; ++it_field) {
                auto key_field = it_field.key()->as_string().as_string();
                auto old_value = view.get_value(key_field);
                auto new_value = old_value;
                if (key_update == "$set") {
                    new_value = it_field.value();
                } else if (key_update == "$inc") {
                    new_value = inc_(new_value, it_field.value());
                }
                //todo impl others methods
                if (!new_value->is_equal(old_value)) {
                    set_(key_field, new_value);
                    result = true;
                }
            }
        }
        return result;
    }

    void document_t::set_(const std::string &key, document_const_value_t value) {
        set_new_value_(value_, key, std::move(value));
    }

    document_ptr make_document() {
        return new document_t();
    }

    document_ptr make_document(const ::document::impl::dict_t *dict) {
        return new document_t(::document::impl::mutable_dict_t::new_dict(dict));
    }

    document_ptr make_document(const ::document::impl::array_t *array) {
        return new document_t(::document::impl::mutable_array_t::new_array(array));
    }

    document_ptr make_document(const ::document::impl::value_t *value) {
        if (value->type() == value_type::dict) {
            return make_document(value->as_dict());
        } else if (value->type() == value_type::array) {
            return make_document(value->as_array());
        }
        return nullptr;
    }

    document_ptr make_upsert_document(const document_ptr& source) {
        auto doc = make_document();
        document_view_t view(source);
        const auto *update_dict = view.as_dict();
        for (auto it = update_dict->begin(); it; ++it) {
            auto cmd = static_cast<std::string_view>(it.key()->as_string());
            if (cmd == "$set" || cmd == "$inc") {
                auto values = it.value()->as_dict();
                for (auto it_field = values->begin(); it_field; ++it_field) {
                    auto key = static_cast<std::string>(it_field.key()->as_string());
                    doc->set(key, it_field.value());
                }
            }
        }
        doc->set("_id", view.get_string("_id"));
        return doc;
    }

    document_id_t get_document_id(const document_ptr &document) {
        return components::document::document_view_t(document).id();
    }

    document_const_value_t json2value(const boost::json::value &item) {
        if (item.is_bool()) {
            return ::document::impl::new_value(item.get_bool());
        } else if (item.is_uint64()) {
            return ::document::impl::new_value(item.get_uint64());
        } else if (item.is_int64()) {
            return ::document::impl::new_value(item.get_int64());
        } else if (item.is_double()) {
            return ::document::impl::new_value(item.get_double());
        } else if (item.is_string()) {
            return ::document::impl::new_value(::document::slice_t(item.get_string().c_str()));
        } else if (item.is_array()) {
            auto array = ::document::impl::mutable_array_t::new_array();
            for (const auto &child : item.get_array()) {
                array->append(json2value(child));
            }
            return array->as_array();
        } else if (item.is_object()) {
            auto dict = ::document::impl::mutable_dict_t::new_dict();
            for (const auto &child : item.get_object()) {
                dict->set(std::string(child.key()), json2value(child.value()));
            }
            return dict->as_dict();
        }
        return ::document::impl::value_t::null_value;
    }

    document_ptr document_from_json(const std::string &json) {
        auto doc = make_document();
        auto tree = boost::json::parse(json);
        for (const auto &item : tree.as_object()) {
            doc->set(std::string(item.key()), json2value(item.value()));
        }
        return doc;
    }

    std::string document_to_json(const document_ptr &doc) {
        return document_view_t(doc).to_json();
    }

    std::string serialize_document(const document_ptr &document) {
        return document_to_json(document);
    }

    document_ptr deserialize_document(const std::string &text) {
        return document_from_json(text);
    }

    bool is_equals_documents(const document_ptr& doc1, const document_ptr& doc2) {
        return document_view_t(doc1).get_value()->is_equal(document_view_t(doc2).get_value());
    }

}
