#include <jni.h>
#include <string>
#include <cctype>
#include "engine_BackendBridge.h"
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "search.h"


#include <iostream> // For debugging

static Board board;

static bool initialized = [] {
    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1");
    return true;
}();

std::string moveToAlgebraic(const Move& m) {
    char fromFile = 'a' + m.fromCol;
    char fromRank = '8' - m.fromRow;
    char toFile = 'a' + m.toCol;
    char toRank = '8' - m.toRow;
    return {fromFile, fromRank, toFile, toRank};
}

Move parseMoveString(const std::string& moveStr) {
    int fromCol = moveStr[0] - 'a';
    int fromRow = 8 - (moveStr[1] - '0');
    int toCol = moveStr[2] - 'a';
    int toRow = 8 - (moveStr[3] - '0');
    return Move(fromRow, fromCol, toRow, toCol);
}

JNIEXPORT jboolean JNICALL Java_engine_BackendBridge_applyMove
(JNIEnv* env, jclass, jstring jmove) {
    const char* moveChars = env->GetStringUTFChars(jmove, nullptr);
    std::string moveStr(moveChars);
    env->ReleaseStringUTFChars(jmove, moveChars);

    Move move = parseMoveString(moveStr);
    return board.makeMove(move); 
}

JNIEXPORT jobjectArray JNICALL Java_engine_BackendBridge_getBoardState(JNIEnv* env, jclass) {
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray outer = env->NewObjectArray(8, env->FindClass("[Ljava/lang/String;"), nullptr);

    for (int row = 0; row < 8; ++row) {
        jobjectArray inner = env->NewObjectArray(8, stringClass, nullptr);
        for (int col = 0; col < 8; ++col) {
            Piece* piece = board.getPiece(row, col);
            if (piece) {
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

    return outer;
}

JNIEXPORT jboolean JNICALL Java_engine_BackendBridge_isCheckmate
(JNIEnv*, jclass) {
    return board.isCheckmate(board.getWhiteToMove());
}

JNIEXPORT jobjectArray JNICALL Java_engine_BackendBridge_getLegalMoves(JNIEnv* env, jclass) {
    MoveGenerator gen;
    std::vector<Move> legal = gen.generateLegalMoves(board, board.getWhiteToMove());

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray result = env->NewObjectArray(legal.size(), stringClass, nullptr);

    for (size_t i = 0; i < legal.size(); ++i) {
        std::string moveStr = moveToAlgebraic(legal[i]); 
        jstring jmove = env->NewStringUTF(moveStr.c_str());
        env->SetObjectArrayElement(result, i, jmove);
    }

    return result;
}

JNIEXPORT jstring JNICALL Java_engine_BackendBridge_getSuggestedMove(JNIEnv* env, jclass) {
    Move suggested = Search::findBestMove(board, board.getWhiteToMove(), 4);

    std::string moveStr = moveToAlgebraic(suggested);
    return env->NewStringUTF(moveStr.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_engine_BackendBridge_loadFEN(JNIEnv* env, jclass, jstring fenString) {

    const char* fen = env->GetStringUTFChars(fenString, nullptr);

    std::string fenStr(fen);
    board.loadFEN(fenStr);

    env->ReleaseStringUTFChars(fenString, fen);
}