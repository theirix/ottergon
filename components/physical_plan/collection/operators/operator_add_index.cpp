#include "operator_add_index.hpp"
#include <components/cursor/cursor.hpp>
#include <components/index/disk/route.hpp>
#include <components/index/single_field_index.hpp>
#include <core/pmr.hpp>
#include <services/collection/collection.hpp>
#include <services/collection/route.hpp>
#include <services/collection/session/session.hpp>
#include <services/disk/index_disk.hpp>

namespace services::collection::operators {

    operator_add_index::operator_add_index(context_collection_t* context, components::ql::create_index_ptr ql)
        : read_write_operator_t(context, operator_type::add_index)
        , index_ql_{std::move(ql)} {}

    void operator_add_index::on_execute_impl(components::pipeline::context_t* pipeline_context) {
        trace(context_->log(),
              "operator_add_index::on_execute_impl session: {}, index: {}",
              pipeline_context->session.data(),
              index_ql_->name());
        switch (index_ql_->index_type_) {
            case components::ql::index_type::single: {
                const bool index_exist = context_->index_engine()->has_index(index_ql_->name());
                const auto id_index = index_exist
                                          ? components::index::INDEX_ID_UNDEFINED
                                          : components::index::make_index<components::index::single_field_index_t>(
                                                context_->index_engine(),
                                                index_ql_->name(),
                                                index_ql_->keys_);

                sessions::make_session(context_->sessions(),
                                       pipeline_context->session,
                                       index_ql_->name(),
                                       sessions::create_index_t{pipeline_context->current_message_sender, id_index});
                pipeline_context->send(context_->disk(),
                                       services::index::handler_id(services::index::route::create),
                                       std::move(*(index_ql_.release())),
                                       context_);
                break;
            }
            case components::ql::index_type::composite:
            case components::ql::index_type::multikey:
            case components::ql::index_type::hashed:
            case components::ql::index_type::wildcard: {
                trace(context_->log(), "index_type not implemented");
                assert(false && "index_type not implemented");
                break;
            }
            case components::ql::index_type::no_valid: {
                trace(context_->log(), "index_type not valid");
                assert(false && "index_type not valid");
                break;
            }
        }
    }

} // namespace services::collection::operators