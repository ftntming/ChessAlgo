#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "../src/board.h"
#include "../src/search.h"
#include "../src/movegen.h"

namespace {

std::string moveToAlgebraic(const Move& m) {
    char fromFile = static_cast<char>('a' + m.fromCol);
    char fromRank = static_cast<char>('8' - m.fromRow);
    char toFile = static_cast<char>('a' + m.toCol);
    char toRank = static_cast<char>('8' - m.toRow);
    return {fromFile, fromRank, toFile, toRank};
}

bool containsMove(const std::vector<Move>& moves, const Move& target) {
    return std::any_of(moves.begin(), moves.end(), [&](const Move& m) {
        return m == target;
    });
}

} // namespace

int main() {
    const std::vector<std::string> fens = {
        // Starting position
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1",
        // Open middle game style position
        "r1bqkbnr/pppp1ppp/2n5/4p3/3P4/2N2N2/PPP1PPPP/R1BQKB1R w - - 0 4",
        // Enclosed kings and many pieces
        "r3k2r/pppq1ppp/2npbn2/3Np3/2P1P3/2N1B3/PP2BPPP/R2Q1RK1 w - - 0 10",
        // White pawn on last rank (edge case for pawn move generation bounds)
        "4k3/8/8/8/8/8/8/4P3 w - - 0 1",
        // Black pawn on first rank (edge case for pawn move generation bounds)
        "4k3/8/8/8/8/8/8/4p3 b - - 0 1",
        // Checkmate-like/no-legal-move candidate
        "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1"
    };

    constexpr int kDepth = 4;
    constexpr int kIterationsPerFen = 50;

    Board board;

    int totalCases = 0;
    int validatedCases = 0;

    for (const std::string& fen : fens) {
        for (int i = 0; i < kIterationsPerFen; ++i) {
            board.loadFEN(fen);
            const bool whiteToMove = board.getWhiteToMove();
            const std::vector<Move> legalMoves = MoveGenerator::generateLegalMoves(board, whiteToMove);

            Move best = Search::findBestMove(board, whiteToMove, kDepth);
            ++totalCases;

            if (legalMoves.empty()) {
                // Search currently returns default Move() when no legal move exists.
                continue;
            }

            if (!containsMove(legalMoves, best)) {
                std::cerr << "[FAIL] Non-legal best move returned at iteration " << i
                          << " for FEN: " << fen << "\n"
                          << "[FAIL] Move: " << moveToAlgebraic(best) << "\n";
                return 1;
            }

            ++validatedCases;
        }
    }

    std::cout << "[PASS] Stress search completed. Total cases: " << totalCases
              << ", validated legal-move cases: " << validatedCases << std::endl;
    return 0;
}