#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include <limits>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <algorithm>

#ifndef SEARCH_DEBUG_LOGS
#define SEARCH_DEBUG_LOGS 0
#endif

#ifndef SEARCH_PERF_LOGS
#define SEARCH_PERF_LOGS 1
#endif

#ifndef SEARCH_PARALLEL
#define SEARCH_PARALLEL 1
#endif

#if SEARCH_DEBUG_LOGS
#define SEARCH_DEBUG(stmt) do { stmt; } while (0)
#else
#define SEARCH_DEBUG(stmt) do {} while (0)
#endif

#if SEARCH_PERF_LOGS
#define SEARCH_PERF(stmt) do { stmt; } while (0)
#else
#define SEARCH_PERF(stmt) do {} while (0)
#endif

namespace Search {
    static const int MAX_DEPTH = 64; // Probably will never end up using this much
    static const int MATE = 100000; // score for a checkmate (very large)
    const int INF = 1000000; // int_min but we want to avoid overflow!

    long long count; // for testing purposes, counts amount of nodes in a search
    double time;

    static Move killerMoves[MAX_DEPTH][2];
    // To store quiet (non-capture) moves that killed using beta <= alpha in previous (maybe-siblings/cousins)
    // So that they are sorted over other ones in future move orderings
    // Not much need to clear between searches

    // All nodes at a depth will either ALL be min/max nodes, so no need to store two.


    // Forward declarations
    // IMPORTANT: With current implementation, maxDepth must stay constant through all related calls (for killer-moves)
    int minimax(Board& board, int depth, int alpha, int beta, bool maximizingPlayer, Evaluator& evaluator, int maxDepth);
    int negamax(Board& board, int depth, int alpha, int beta, Evaluator& evaluator, int maxDepth);
    std::vector<Move> generateSearchMoves(const Board& board, int ply, bool whiteToMove);
    
    Move findBestMove(Board& board, bool whiteToMove, int depth) {
        count = 0;
        time = 0;
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<Move> legalMoves = generateSearchMoves(board, 0, whiteToMove);
        auto end = std::chrono::high_resolution_clock::now();
        time += std::chrono::duration<double>(end - start).count();

        double moveGenTime = std::chrono::duration<double>(end - start).count();
        SEARCH_PERF(
            std::cerr << "[PERF] Move generation: " << legalMoves.size() << " moves generated in "
                      << std::fixed << std::setprecision(3) << (moveGenTime * 1000) << "ms\n";
        );

        if (legalMoves.empty()) {
            SEARCH_DEBUG(std::cerr << "[DEBUG] findBestMove: No legal moves available!\n";);
            return Move();
        }

        auto searchStart = std::chrono::high_resolution_clock::now();

#if SEARCH_PARALLEL
        // Parallel search: evaluate moves concurrently
        const unsigned int numThreads = std::min(static_cast<unsigned int>(legalMoves.size()),
                                                  std::max(1u, std::thread::hardware_concurrency()));

        std::atomic<int> bestEval(-INF);
        std::atomic<size_t> bestMoveIndex(0);
        std::atomic<long long> totalNodes(0);
        std::mutex evalMutex;

        std::vector<int> moveEvals(legalMoves.size(), -INF);
        std::vector<long long> moveNodeCounts(legalMoves.size(), 0);

        auto workerFunction = [&](size_t startIdx, size_t endIdx) {
            for (size_t i = startIdx; i < endIdx; ++i) {
                Board localBoard = board; // Deep copy for thread safety
                Evaluator evaluator;
                long long localCount = 0;

                localBoard.makeMove(legalMoves[i]);

                // Save current global count, do local search, compute delta
                long long countBefore = count;
                int eval = -negamax(localBoard, depth - 1, -INF, INF, evaluator, depth);
                long long nodesUsed = count - countBefore;

                localBoard.undoMove();

                moveEvals[i] = eval;
                moveNodeCounts[i] = nodesUsed;
                totalNodes.fetch_add(nodesUsed, std::memory_order_relaxed);

                // Thread-safe best move update
                std::lock_guard<std::mutex> lock(evalMutex);
                int currentBest = bestEval.load(std::memory_order_relaxed);
                if (eval > currentBest) {
                    bestEval.store(eval, std::memory_order_relaxed);
                    bestMoveIndex.store(i, std::memory_order_relaxed);
                }
            }
        };

        std::vector<std::thread> threads;
        size_t movesPerThread = (legalMoves.size() + numThreads - 1) / numThreads;

        for (unsigned int t = 0; t < numThreads; ++t) {
            size_t startIdx = t * movesPerThread;
            size_t endIdx = std::min(startIdx + movesPerThread, legalMoves.size());
            if (startIdx < endIdx) {
                threads.emplace_back(workerFunction, startIdx, endIdx);
            }
        }

        for (auto& thread : threads) {
            thread.join();
        }

        Move bestMove = legalMoves[bestMoveIndex.load()];
        int finalBestEval = bestEval.load();
        count = totalNodes.load(); // Update global count with parallel total

        SEARCH_PERF(
            std::cerr << "[PERF] Parallel threads: " << numThreads << "\n";
        );

#else
        // Sequential search (original single-threaded)
        Evaluator evaluator;
        int bestEval = -INF;
        Move bestMove = legalMoves[0];

        for (const Move& move : legalMoves) {
            board.makeMove(move);
            int eval = -negamax(board, depth - 1, -INF, INF, evaluator, depth);
            board.undoMove();

            if (eval > bestEval) {
                bestEval = eval;
                bestMove = move;
            }
        }
        int finalBestEval = bestEval;
#endif

        auto searchEnd = std::chrono::high_resolution_clock::now();
        double searchTime = std::chrono::duration<double>(searchEnd - searchStart).count();

#if SEARCH_PERF_LOGS
        double totalTime = moveGenTime + searchTime;
        double nodesPerSecond = totalTime > 0.0 ? (static_cast<double>(count) / totalTime) : 0.0;

        SEARCH_PERF(
            std::cerr << "[PERF] ========== SEARCH SUMMARY ==========\n";
            std::cerr << "[PERF] Search depth: " << depth << "\n";
            std::cerr << "[PERF] Node count: " << count << "\n";
            std::cerr << "[PERF] Time (move generation): " << std::fixed << std::setprecision(3) << (moveGenTime * 1000) << "ms\n";
            std::cerr << "[PERF] Time (negamax search): " << std::fixed << std::setprecision(3) << (searchTime * 1000) << "ms\n";
            std::cerr << "[PERF] Time (total): " << std::fixed << std::setprecision(3) << (totalTime * 1000) << "ms\n";
            std::cerr << "[PERF] Nodes per second: " << std::fixed << std::setprecision(0) << nodesPerSecond << "\n";
            std::cerr << "[PERF] Best eval: " << finalBestEval << "\n";
            std::cerr << "[PERF] Best move: " << (char)('a' + bestMove.fromCol) << (char)('8' - bestMove.fromRow)
                      << (char)('a' + bestMove.toCol) << (char)('8' - bestMove.toRow) << "\n";
            std::cerr << "[PERF] =====================================\n";
        );
#else
        (void)moveGenTime;
        (void)searchTime;
#endif

        return bestMove;
    }

    // minimum is same as maximizing the negative!
    // Try to use an even valued depth to avoid choosing paths that do a bad capture as their last move.
    int negamax(Board& board, int depth, int alpha, int beta, Evaluator& evaluator, int maxDepth) {
        ++count;
        if (depth == 0) {
            // 0 measurement of the "potential" in a board
            // If no moves take then either it goes for a bad result or goes for a random move... not great. 
            int eval = evaluator.evaluate(board);
            SEARCH_DEBUG(
                if (maxDepth - depth <= 2) {
                    std::cerr << "[DEBUG] Leaf node evaluation: " << eval << "\n";
                }
            );
            return eval;
        }

        int ply = maxDepth - depth;
        auto start = std::chrono::high_resolution_clock::now(); // for testing
        std::vector<Move> moves = generateSearchMoves(board, ply, board.getWhiteToMove());
        auto end = std::chrono::high_resolution_clock::now(); // testing
        double moveGenTime = std::chrono::duration<double>(end - start).count();
        time += moveGenTime; // testing

        SEARCH_DEBUG(
            if (ply <= 2) {
                std::cerr << "[DEBUG] Depth " << ply << ": Generated " << moves.size() << " moves in "
                          << std::fixed << std::setprecision(3) << (moveGenTime * 1000) << "ms\n";
            }
        );

        int bestEval = -INF;
        bool tried = false;
#if SEARCH_DEBUG_LOGS
        int movesTried = 0;
#endif

        for (const Move& move : moves) {
            board.makeMove(move, true);
            // If move leaves us in check → illegal
            if (board.isKingInCheck(!board.getWhiteToMove())) {
                board.undoMove();
                continue;
            }
            tried = true;
#if SEARCH_DEBUG_LOGS
            movesTried++;
#endif

            int eval = -negamax(board, depth - 1, -beta, -alpha, evaluator, maxDepth);
            board.undoMove();

            SEARCH_DEBUG(
                if (ply <= 1) {
                    std::cerr << "[DEBUG] Depth " << ply << ": Move " << (char)('a' + move.fromCol)
                              << (char)('8' - move.fromRow) << (char)('a' + move.toCol) << (char)('8' - move.toRow)
                              << " evaluated to " << eval << "\n";
                }
            );

            if (eval > bestEval) {
                bestEval = eval;
            }
            alpha = std::max(alpha, eval); // You are a maximizing node so you would like to maximize the eval

            if (beta <= alpha) {
                if (!board.getPiece(move.toRow, move.toCol)) {
                    killerMoves[maxDepth - depth][1] = killerMoves[maxDepth - depth][0];
                    killerMoves[maxDepth - depth][0] = move;
                }
                SEARCH_DEBUG(
                    if (ply <= 2) {
                        std::cerr << "[DEBUG] Depth " << ply << ": Beta cutoff after " << movesTried << " moves\n";
                    }
                );
                break; // if beta <= alpha, that means your parent (a minimizer) has found a path with value beta,
                // and since it wants to minimze, then it will never choose you, alpha, since you're maximizing
                // so your alpha only goes up. 
            }
        }

        if (!tried) {
            if (board.isCheckmate(board.getWhiteToMove())) {
                return -MATE; // checkmate
            } else {
                return 0; // stalemate
            }
        }

        return bestEval;
    } 

    // Generate moves for a search, which are legal moves sorted by a score (captures prioritized)
    // NOTE: This method of sorting favours trading pieces over passive playing which is... interesting to note!
    std::vector<Move> generateSearchMoves(const Board& board, int ply, bool whiteToMove) {
        std::vector<Move> moves = MoveGenerator::generateMoves(board, whiteToMove);

        // Precompute score values to use in sort below
        for (Move& move : moves) {
            if (move == Search::killerMoves[ply][0]) {
                move.score = 5000; // rather large score, 0 = more recent killer move
                continue;
            } 
            if (move == Search::killerMoves[ply][1]) {
                move.score = 4999; // one less than above
                continue;
            }

            Piece* captured = board.getPiece(move.toRow, move.toCol);
            Piece* moving = board.getPiece(move.fromRow, move.fromCol);
            int val;

            if (captured) {
                val = 10 * getCaptureValue(captured->getSymbol());
                val -= getCaptureValue(moving->getSymbol()); // If causes error something is very wrong.
                // ^ We prefer taking with smaller value pieces (tends to be safer/better)
            } else {
                val = 0;
            }

            move.score = val;
        }

        // Sort moves by their score value. 
        sort(moves.begin(), moves.end(), [](const Move& moveA, const Move& moveB) {
            return moveA.score > moveB.score;
        });

        return moves;
    }

    int getCaptureValue(char symbol) {
        switch (tolower(symbol)) {
            case 'p': return 100;   // Pawn
            case 'n': return 320;   // Knight
            case 'b': return 330;   // Bishop
            case 'r': return 500;   // Rook
            case 'q': return 900;   // Queen
            case 'k': return 20000;   // King
            default: return 0;
        }
    }

    // Retired from use:
    /*
    int minimax(Board& board, int depth, int alpha, int beta, bool maximizingPlayer, Evaluator& evaluator, int maxDepth) {
        // LATER: Check stalemate
        if (depth == 0) {
            return evaluator.evaluate(board);
        }

        std::vector<Move> moves = generateSearchMoves(board, maxDepth - depth, maximizingPlayer);
        if (moves.empty() && board.isCheckmate(maximizingPlayer)) {
            return maximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max(); // checkmate
        }
        if(board.isCheckmate(!maximizingPlayer)) {
            return !maximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max(); // checkmate
        }

        int bestEval = maximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();

        for (const Move& move : moves) {
            board.makeMove(move, true);                
            int eval = minimax(board, depth - 1, alpha, beta, !maximizingPlayer, evaluator, maxDepth);
            board.undoMove();

            if (maximizingPlayer) {
                if (eval > bestEval) {
                    bestEval = eval;
                }
                alpha = std::max(alpha, eval); // You are a maximizing node so you would like to maximize the eval

                if (beta <= alpha) {
                    if (!board.getPiece(move.toRow, move.toCol)) {
                        killerMoves[maxDepth - depth][1] = killerMoves[maxDepth - depth][0];
                        killerMoves[maxDepth - depth][0] = move;
                    }
                    break; // if beta <= alpha, that means your parent (a minimizer) has found a path with value beta,
                    // and since it wants to minimze, then it will never choose you, alpha, since you're maximizing
                    // so your alpha only goes up. 
                }
            } else {
                if (eval < bestEval) {
                    bestEval = eval; 
                }
                beta = std::min(beta, eval); // You are a minimizing node so you would like to maximize the eval

                if (beta <= alpha) {
                    if (!board.getPiece(move.toRow, move.toCol)) {
                        killerMoves[maxDepth - depth][1] = killerMoves[maxDepth - depth][0];
                        killerMoves[maxDepth - depth][0] = move;
                    }
                    break; 
                }
            }
        }
        return bestEval;
    } 
    */
} // namespace Search

