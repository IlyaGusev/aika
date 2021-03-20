#include <search/config.h>

#include <util.h>

#include <fstream>
#include <string>

void TSearchConfig::Parse(const std::string& configFile) {
    Json::Value root;
    Json::Reader reader;
    std::ifstream file(configFile);
    bool parsingSuccessful = reader.parse(file, root, false);
    ENSURE(parsingSuccessful, "Bad config");
    Depth = root.get("depth", Depth).asInt();
    EnableAlphaBeta = root.get("enable_alpha_beta", EnableAlphaBeta).asBool();
    EnablePST = root.get("enable_pst", EnablePST).asBool();
    EnableTT = root.get("enable_tt", EnableTT).asBool();
    QuiescenceSearchDepth = root.get("qsearch_depth", QuiescenceSearchDepth).asInt();
}
