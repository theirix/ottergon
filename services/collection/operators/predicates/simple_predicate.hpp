#pragma once

#include <functional>
#include "predicate.hpp"

namespace services::collection::operators::predicates {

    class simple_predicate : public predicate {
    public:
        simple_predicate(context_collection_t* context, std::function<bool(const components::document::document_ptr&)> func);

        bool check_impl(const components::document::document_ptr& document) final;

    private:
        std::function<bool(const components::document::document_ptr&)> func_;
    };

    predicate_ptr create_simple_predicate(context_collection_t* context, const components::ql::expr_ptr& expr);
    predicate_ptr create_simple_predicate(context_collection_t* context, const components::ql::find_statement_ptr& cond);

} // namespace services::operators::predicates
