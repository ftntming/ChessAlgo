#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "../src/board.h"
#include "../src/search.h"
#include "../src/movegen.h"

namespace {

// Parse FEN file, skipping lines starting with '#' (comments)
std::string readFenFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open FEN file: " + filePath);
    }

    std::string line;
    std::string fenString;

    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (!line.empty()) {
            fenString = line;
            break; // Take the first non-comment, non-empty line
        }
    }

    file.close();

    if (fenString.empty()) {
        throw std::runtime_error("No valid FEN string found in file: " + filePath);
    }

    return fenString;
}

std::string moveToAlgebraic(const Move& m) {
    char fromFile = static_cast<char>('a' + m.fromCol);
    char fromRank = static_cast<char>('8' - m.fromRow);
    char toFile = static_cast<char>('a' + m.toCol);
    char toRank = static_cast<char>('8' - m.toRow);
    return {fromFile, fromRank, toFile, toRank};
}

} // namespace

int main(int argc, char* argv[]) {
    // find current executable path to locate fen-files relative to it
    std::string fenFilePath = "fen-files/chess_position.fen";
    int searchDepth = 8; // Default to depth 4 for reasonable speed

    // Allow override from command line arguments
    if (argc > 1) {
        fenFilePath = argv[1];
    }
    if (argc > 2) {
        searchDepth = std::atoi(argv[2]);
        if (searchDepth < 1 || searchDepth > 10) {
            std::cerr << "[WARNING] Invalid depth " << searchDepth << ", using default 4" << std::endl;
            searchDepth = 4;
        }
    }

    try {
        std::cout << "[INFO] Loading FEN from: " << fenFilePath << std::endl;
        std::string fen = readFenFromFile(fenFilePath);
        std::cout << "[INFO] Loaded FEN: " << fen << std::endl;

        Board board;
        board.loadFEN(fen);

        int moveCount = 0;
        constexpr int kMaxMoves = 20; // Safety limit to prevent infinite loops

        std::cout << "\n[START] Game simulation started (depth=" << searchDepth << ")" << std::endl;
        std::cout << "======================================" << std::endl;

        while (moveCount < kMaxMoves) {
            bool whiteToMove = board.getWhiteToMove();
            std::vector<Move> legalMoves = MoveGenerator::generateLegalMoves(board, whiteToMove);

            std::string playerColor = whiteToMove ? "White" : "Black";
            std::cout << "\nMove " << (moveCount + 1) << ": " << playerColor << " to move" << std::endl;
            std::cout << "  Legal moves available: " << legalMoves.size() << std::endl;

            if (legalMoves.empty()) {
                bool isCheckmate = board.isCheckmate(whiteToMove);
                if (isCheckmate) {
                    std::cout << "[END] Checkmate! " << (whiteToMove ? "Black" : "White") << " wins!" << std::endl;
                } else {
                    std::cout << "[END] Stalemate! Game is a draw." << std::endl;
                }
                break;
            }

            // Get suggested move from AI
            std::cout << "  Computing best move (depth=" << searchDepth << ")..." << std::endl;
            Move suggestedMove = Search::findBestMove(board, whiteToMove, searchDepth);

            if (legalMoves.empty()) {
                std::cout << "[ERROR] No legal moves found, but board is not in checkmate?" << std::endl;
                break;
            }

            // Verify the suggested move is legal
            bool isMoveValid = false;
            for (const Move& m : legalMoves) {
                if (m == suggestedMove) {
                    isMoveValid = true;
                    break;
                }
            }

            if (!isMoveValid) {
                std::cerr << "[ERROR] Suggested move is not legal!" << std::endl;
                std::cerr << "[ERROR] Suggested: " << moveToAlgebraic(suggestedMove) << std::endl;
                return 1;
            }

            std::string moveStr = moveToAlgebraic(suggestedMove);
            std::cout << "  Suggested move: " << moveStr << std::endl;

            // Apply the move
            if (!board.makeMove(suggestedMove)) {
                std::cerr << "[ERROR] Failed to apply move: " << moveStr << std::endl;
                return 1;
            }

            std::cout << "  Move applied: " << moveStr << std::endl;
            moveCount++;
        }

        if (moveCount >= kMaxMoves) {
            std::cout << "\n[WARNING] Reached max move limit (" << kMaxMoves << ")" << std::endl;
        }

        std::cout << "\n======================================" << std::endl;
        std::cout << "[SUMMARY] Total moves: " << moveCount << std::endl;
        std::cout << "[PASS] Game simulation completed successfully" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[ERROR] Unexpected error occurred" << std::endl;
        return 1;
    }
}

