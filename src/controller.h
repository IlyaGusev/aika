#pragma once

#include <drogon/HttpController.h>
#include <chess/pgn.h>

#include "random_strategy.h"

class TController : public drogon::HttpController<TController, /* AutoCreation */ false> {
private:
    TStrategies Strategies;

public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TController::MakeMove,"/make_move", drogon::Post);
    METHOD_LIST_END

    void Init(TStrategies&& strategies) {
        Strategies = std::move(strategies);
    }

    void MakeMove(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    ) const;
};
