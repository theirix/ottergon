#include "operator_delete.hpp"

namespace services::collection::operators {

    operator_delete::operator_delete(context_collection_t* context)
        : read_write_operator_t(context, operator_type::remove) {
    }

    void operator_delete::on_execute_impl(planner::transaction_context_t* transaction_context) {
        if (left_ && left_->output() && !left_->output()->documents().empty()) {
            modified_ = make_operator_write_data(context_->resource());
            for (const auto& document : left_->output()->documents()) {
                const auto id = get_document_id(document);
                if (context_->storage().erase(context_->storage().find(id)) != context_->storage().end()) {
                    modified_->append(id);
                    context_->index_engine()->delete_document(document);
                }
            }
        }
    }

} // namespace services::collection::operators
