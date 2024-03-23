#pragma once
#include <components/ql/index.hpp>
#include <memory>
#include <services/collection/operators/operator.hpp>
#include <string>

namespace services::collection::operators {

    class operator_add_index final : public read_write_operator_t {
    public:
        operator_add_index(context_collection_t* context, components::ql::create_index_t* ql);

    private:
        void on_execute_impl(components::pipeline::context_t* pipeline_context) final;

        std::unique_ptr<components::ql::create_index_t> ql_;
    };

} // namespace services::collection::operators
