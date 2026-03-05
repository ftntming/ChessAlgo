#include <jni.h>
#include <string>
#include <cctype>
#include "engine_BackendBridge.h"
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "search.h"
#include <iostream>
#include <chrono>
#include <iomanip>

// Performance logging helper
class PerfTimer {
public:
    PerfTimer(const char *name) : name_(name), start_(std::chrono::high_resolution_clock::now()) {
        std::cerr << "[PERF] >>> " << name_ << " started" << std::endl;
    }

    ~PerfTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start_).count();
        std::cerr << "[PERF] <<< " << name_ << " completed in " << std::fixed << std::setprecision(3) << duration <<
                "ms" << std::endl;
    }

private:
    const char *name_;
    std::chrono::high_resolution_clock::time_point start_;
};

#define LOG_PERF(funcName) PerfTimer perf_timer(funcName)

static Board board;

static bool initialized = [] {
    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1");
    return true;
}();

std::string moveToAlgebraic(const Move &m) {
    char fromFile = 'a' + m.fromCol;
    char fromRank = '8' - m.fromRow;
    char toFile = 'a' + m.toCol;
    char toRank = '8' - m.toRow;
    return {fromFile, fromRank, toFile, toRank};
}

Move parseMoveString(const std::string &moveStr) {
    int fromCol = moveStr[0] - 'a';
    int fromRow = 8 - (moveStr[1] - '0');
    int toCol = moveStr[2] - 'a';
    int toRow = 8 - (moveStr[3] - '0');
    return Move(fromRow, fromCol, toRow, toCol);
}

JNIEXPORT jboolean JNICALL Java_engine_BackendBridge_applyMove
(JNIEnv *env, jclass, jstring jmove) {
    LOG_PERF("applyMove");

    const char *moveChars = env->GetStringUTFChars(jmove, nullptr);
    std::string moveStr(moveChars);
    env->ReleaseStringUTFChars(jmove, moveChars);

    std::cerr << "[DEBUG] applyMove: move=" << moveStr << std::endl;

    Move move = parseMoveString(moveStr);
    jboolean result = board.makeMove(move);

    std::cerr << "[DEBUG] applyMove: result=" << (result ? "true" : "false") << std::endl;

    return result;
}

JNIEXPORT jobjectArray JNICALL Java_engine_BackendBridge_getBoardState(JNIEnv *env, jclass) {
    LOG_PERF("getBoardState");

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray outer = env->NewObjectArray(8, env->FindClass("[Ljava/lang/String;"), nullptr);

    int pieceCount = 0;
    for (int row = 0; row < 8; ++row) {
        jobjectArray inner = env->NewObjectArray(8, stringClass, nullptr);
        for (int col = 0; col < 8; ++col) {
            Piece *piece = board.getPiece(row, col);
            if (piece) {
                pieceCount++;
                char symbol = piece->getSymbol();
                std::string code(1, symbol);
                jstring jstr = env->NewStringUTF(code.c_str());
                env->SetObjectArrayElement(inner, col, jstr);
            } else {
                env->SetObjectArrayElement(inner, col, nullptr);
            }
        }
        env->SetObjectArrayElement(outer, row, inner);
    }

    std::cerr << "[DEBUG] getBoardState: " << pieceCount << " pieces on board" << std::endl;

    return outer;
}

JNIEXPORT jboolean JNICALL Java_engine_BackendBridge_isCheckmate
(JNIEnv *, jclass) {
    LOG_PERF("isCheckmate");

    jboolean result = board.isCheckmate(board.getWhiteToMove());

    std::cerr << "[DEBUG] isCheckmate: " << (result ? "true" : "false") << std::endl;

    return result;
}

JNIEXPORT jobjectArray JNICALL Java_engine_BackendBridge_getLegalMoves(JNIEnv *env, jclass) {
    LOG_PERF("getLegalMoves");

    MoveGenerator gen;
    std::vector<Move> legal = gen.generateLegalMoves(board, board.getWhiteToMove());

    std::cerr << "[DEBUG] getLegalMoves: generated " << legal.size() << " legal moves" << std::endl;

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray result = env->NewObjectArray(legal.size(), stringClass, nullptr);

    for (size_t i = 0; i < legal.size(); ++i) {
        std::string moveStr = moveToAlgebraic(legal[i]);
        jstring jmove = env->NewStringUTF(moveStr.c_str());
        env->SetObjectArrayElement(result, i, jmove);
    }


    return result;
}

JNIEXPORT jstring JNICALL Java_engine_BackendBridge_getSuggestedMove(JNIEnv *env, jclass) {
    LOG_PERF("getSuggestedMove");

    Move suggested = Search::findBestMove(board, board.getWhiteToMove(), 8);

    std::string moveStr = moveToAlgebraic(suggested);
    std::cerr << "[DEBUG] getSuggestedMove: returned move=" << moveStr << std::endl;

    return env->NewStringUTF(moveStr.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_engine_BackendBridge_loadFEN(JNIEnv *env, jclass, jstring fenString) {
    LOG_PERF("loadFEN");

    const char *fen = env->GetStringUTFChars(fenString, nullptr);

    std::string fenStr(fen);
    std::cerr << "[DEBUG] loadFEN: loading FEN=" << fenStr << std::endl;
    board.loadFEN(fenStr);

    env->ReleaseStringUTFChars(fenString, fen);
}
