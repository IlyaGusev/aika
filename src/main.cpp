#include <chess/board.h>
#include <chess/pgn.h>

#include <controller.h>
#include <random_strategy.h>
#include <book_strategy.h>
#include <search_strategy.h>

#include <boost/program_options.hpp>
#include <drogon/drogon.h>

#include <string>

namespace po = boost::program_options;

int main(int argc, char** argv) {
    lczero::InitializeMagicBitboards();

    po::options_description desc("options");
    desc.add_options()
        ("port", po::value<uint32_t>()->default_value(8000), "port")
        ("host", po::value<std::string>()->default_value("0.0.0.0"), "host")
        ("book", po::value<std::string>()->default_value(""), "book")
        ("search_config", po::value<std::string>()->default_value("search_config.json"), "search_config")
        ;
    po::positional_options_description p;
    po::command_line_parser parser{argc, argv};
    parser.options(desc);
    po::parsed_options parsed_options = parser.run();
    po::variables_map vm;
    po::store(parsed_options, vm);
    po::notify(vm);

    std::string host = vm["host"].as<std::string>();
    std::string book = vm["book"].as<std::string>();
    std::string search_config = vm["search_config"].as<std::string>();
    uint32_t port = vm["port"].as<uint32_t>();

    drogon::app()
        .setLogLevel(trantor::Logger::kInfo)
        .addListener(host, port)
        .setThreadNum(6)
        .setMaxConnectionNum(256)
        .setMaxConnectionNumPerIP(0)
        .setIdleConnectionTimeout(60)
        .setKeepaliveRequestsNumber(0)
        .setPipeliningRequestsNumber(0)
        .setDocumentRoot("./gui");

    TStrategies strategies;
    strategies.emplace_back(new TBookStrategy(book));
    strategies.emplace_back(new TSearchStrategy(search_config));

    auto controllerPtr = std::make_shared<TController>();
    controllerPtr->Init(std::move(strategies));
    drogon::app().registerController(controllerPtr);
    std::cout << "Server started" << std::endl;
    drogon::app().run();
    return 0;
}
