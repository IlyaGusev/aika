#pragma once

#include <drogon/HttpController.h>

class TController : public drogon::HttpController<TController, /* AutoCreation */ false> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TController::MakeMove,"/make_move", drogon::Post);
    METHOD_LIST_END

    void MakeMove(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    ) const;
};
