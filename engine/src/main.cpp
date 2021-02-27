#include <string>

#include <boost/program_options.hpp>
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

void TController::MakeMove(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) const {
    std::string pgn = req->getParameter("pgn");
    Json::Value ret;
    ret["pgn"] = pgn;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

namespace po = boost::program_options;

int main(int argc, char** argv) {
    po::options_description desc("options");
    desc.add_options()
        ("port", po::value<uint32_t>()->default_value(8000), "port")
        ("host", po::value<std::string>()->default_value("0.0.0.0"), "host")
        ;
    po::positional_options_description p;
    po::command_line_parser parser{argc, argv};
    parser.options(desc);
    po::parsed_options parsed_options = parser.run();
    po::variables_map vm;
    po::store(parsed_options, vm);
    po::notify(vm);

    std::string host = vm["host"].as<std::string>();
    uint32_t port = vm["port"].as<uint32_t>();

    drogon::app()
        .setLogLevel(trantor::Logger::kTrace)
        .addListener(host, port)
        .setThreadNum(6)
        .setMaxConnectionNum(256)
        .setMaxConnectionNumPerIP(0)
        .setIdleConnectionTimeout(60)
        .setKeepaliveRequestsNumber(0)
        .setPipeliningRequestsNumber(0)
        .setDocumentRoot("./gui");

    auto controllerPtr = std::make_shared<TController>();
    drogon::app().registerController(controllerPtr);
    drogon::app().run();
    return 0;
}
