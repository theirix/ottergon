#pragma once

#include <services/collection/operators/operator.hpp>
#include <services/collection/operators/predicates/limit.hpp>

namespace services::collection::operators::merge {

    class operator_merge_t : public read_only_operator_t {
    public:
        explicit operator_merge_t(context_collection_t* context, predicates::limit_t limit);

    protected:
        predicates::limit_t limit_;

    private:
        void on_execute_impl(planner::transaction_context_t* transaction_context) final;
        virtual void on_merge_impl(planner::transaction_context_t* transaction_context) = 0;
    };

    using operator_merge_ptr = std::unique_ptr<operator_merge_t>;

    bool is_operator_merge(const components::ql::expr_ptr& expr);
    operator_merge_ptr create_operator_merge(context_collection_t* context, const components::ql::expr_ptr& expr, predicates::limit_t limit);
    operator_merge_ptr create_operator_merge(context_collection_t* context, const components::ql::find_statement_ptr& cond, predicates::limit_t limit);

} // namespace services::collection::operators::merge
