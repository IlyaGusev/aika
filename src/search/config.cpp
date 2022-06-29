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

    EnableNullMove = root.get("enable_null_move", EnableNullMove).asBool();
    NullMoveDepthReduction = root.get("null_move_depth_reduction", NullMoveDepthReduction).asInt();
    NullMoveEvalMargin = root.get("null_move_eval_margin", NullMoveEvalMargin).asInt();

    EnableLMR = root.get("enable_lmr", EnableLMR).asBool();
    LMRMinMoveNumber = root.get("lmr_min_move_number", LMRMinMoveNumber).asInt();
    LMRMinPly = root.get("lmr_min_ply", LMRMinPly).asInt();
    LMRMinDepth = root.get("lmr_min_depth", LMRMinDepth).asInt();
    LMRReduction = root.get("lmr_reduction", LMRReduction).asInt();

    EnableKillers = root.get("enable_killers", EnableKillers).asInt();
}
