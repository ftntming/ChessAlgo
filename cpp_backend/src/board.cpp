#include "board.h"
#include "move.h"
#include "movegen.h"
#include "pieces/pieces.h"  
#include <iostream>
#include <sstream>
#include <fstream>
#include <cctype>

namespace {
bool inBounds(int row, int col) {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

bool moveInBounds(const Move& move) {
    return inBounds(move.fromRow, move.fromCol) && inBounds(move.toRow, move.toCol);
}
}

Board::Board() {
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            board[r][c] = nullptr;
}

Board::~Board() {
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            delete board[r][c];
}

Piece* Board::getPiece(int row, int col) const {
    if (!inBounds(row, col)) {
        return nullptr;
    }
    return board[row][col];
}

void Board::setPiece(int row, int col, Piece* piece) {
    if (!inBounds(row, col)) {
        delete piece;
        return;
    }
    delete board[row][col];  // Avoid memory leak
    board[row][col] = piece;
}

// TODO: Add guards in case given FEN is invalid. Eg. no king, too many pieces, bad format, etc.,
void Board::loadFEN(const std::string& fen) {

    // Clear the board
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            removePiece(r, c);

    // Captured pieces are owned by undo history; release them when resetting position.
    for (const UndoState& state : history) {
        delete state.capturedPiece;
    }
    history.clear();

    std::istringstream ss(fen);
    std::string boardPart, activeColor;

    ss >> boardPart >> activeColor; // LATER: Add castling rights (not really implementable until castling is implemented)

    int row = 0, col = 0;

    for (char ch : boardPart) {
        if (ch == '/') {
            ++row;
            col = 0;
        } else if (std::isdigit(ch)) {
            col += ch - '0';
        } else {
            Piece* piece = createPieceFromSymbol(ch);
            if (piece && row < 8 && col < 8) {
                setPiece(row, col, piece);
                ++col;
            }
        }
    }

    whiteToMove = (activeColor == "w");
}

bool Board::saveFEN(const char* file_path) const {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[ERROR] saveFEN: Could not open file: " << file_path << std::endl;
        return false;
    }

    // Build the board part of the FEN string
    std::string fenBoard;
    for (int r = 0; r < 8; ++r) {
        if (r > 0) {
            fenBoard += '/';
        }

        int emptyCount = 0;
        for (int c = 0; c < 8; ++c) {
            Piece* piece = board[r][c];
            if (piece == nullptr) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fenBoard += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                fenBoard += piece->getSymbol();
            }
        }

        if (emptyCount > 0) {
            fenBoard += std::to_string(emptyCount);
        }
    }

    // Build the full FEN string
    std::string fen = fenBoard + " ";
    fen += (whiteToMove ? "w" : "b");
    fen += " - - 0 1"; // Castling rights, en passant, halfmove clock, fullmove number (simplified)

    file << fen << std::endl;
    file.close();

    std::cerr << "[INFO] saveFEN: Saved FEN to " << file_path << std::endl;
    return true;
}

void Board::removePiece(int row, int col) {
    if (!inBounds(row, col)) {
        return;
    }
    delete board[row][col];
    board[row][col] = nullptr;
}

Piece* Board::createPieceFromSymbol(char symbol) {
    bool isWhite = isupper(symbol);
    char lower = std::tolower(symbol);

    switch (lower) {
        case 'p': return new Pawn(isWhite);
        case 'n': return new Knight(isWhite); 
        case 'b': return new Bishop(isWhite);
        case 'r': return new Rook(isWhite);
        case 'q': return new Queen(isWhite);
        case 'k': return new King(isWhite);
        default: return nullptr;
    }
}

bool Board::isKingInCheck(bool white) const {
    int kingRow = -1;
    int kingCol = -1;

    // Find the king's position
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece* piece = board[r][c];
            if (piece && piece->isWhitePiece() == white && std::tolower(piece->getSymbol()) == 'k') { 
                kingRow = r;
                kingCol = c;
                break;
            }
        }
        if (kingRow != -1) break;
    }

    if (kingRow == -1 || kingCol == -1) {
        // King not found, default true but shouldn't happen in a valid game
        return true;
    }

    // Check if any opposing piece can attack the king
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece* piece = board[r][c];
            if (piece && piece->isWhitePiece() != white) {
                std::vector<Move> attacks = piece->generateMoves(r, c, *this);
                for (const Move& move : attacks) {
                    if (move.toRow == kingRow && move.toCol == kingCol) {
                        return true; // King is under attack
                    }
                }
            }
        }
    }

    return false; // No threats found
}

// skipValidation can be set true to avoid infinite recursion with generateMoves
bool Board::makeMove(const Move& move, bool skipValidation) {
    if (!moveInBounds(move)) {
        return false;
    }

    Piece* movingPiece = board[move.fromRow][move.fromCol];
    if (!movingPiece || (!skipValidation && movingPiece->isWhitePiece() != whiteToMove))
        return false;

    Piece* targetPiece = board[move.toRow][move.toCol];
    if (targetPiece && targetPiece->isWhitePiece() == movingPiece->isWhitePiece()) {
        return false;
    }

    if (!skipValidation) {
        std::vector<Move> legalMoves = movingPiece->generateMoves(move.fromRow, move.fromCol, *this);
        bool isLegal = false;
        for (const Move& m : legalMoves) {
            if (m.toRow == move.toRow && m.toCol == move.toCol) {
                isLegal = true;
                break;
            }
        }
        if (!isLegal) return false;
    }

    UndoState state(move, board[move.toRow][move.toCol], movingPiece->getHasMoved());
    movingPiece->setHasMoved(true);

    board[move.toRow][move.toCol] = movingPiece;
    board[move.fromRow][move.fromCol] = nullptr;
    whiteToMove = !whiteToMove;

    history.push_back(state);

    return true; 
}

// Undo the last move made. This will require storing a history of moves
// Important to help with search to avoid deep copying boards
// TODO: Make sure that the stored history is maintained properly in other areas,
// such as when needing to reset the history 
bool Board::undoMove() {
    if (history.empty()) {
        return false;
    }

    // TOOD: Below currently assumes the history is correct. Add guards
    // TODO: Fix move so that we can keep piece history and don't have to re-make captured

    UndoState prevState = history.back();
    history.pop_back();

    Piece* captured = prevState.capturedPiece;

    const Move& move = prevState.move;

    Piece* movedPiece = board[move.toRow][move.toCol];
    if (!movedPiece) {
        return false;
    }

    board[move.fromRow][move.fromCol] = movedPiece;
    board[move.fromRow][move.fromCol]->setHasMoved(prevState.hadMoved);
    board[move.toRow][move.toCol] = captured;

    whiteToMove = !whiteToMove;

    return true;
}

bool Board::isCheckmate(bool white) const {
    if (!isKingInCheck(white)) {
        return false; 
    }

    static MoveGenerator moveGen; // TODO: Decide what to do about MoveGenerator
    std::vector<Move> legalMoves = moveGen.generateLegalMoves(*this, white);

    return legalMoves.empty();
}

// Deep copy constructor
Board::Board(const Board& other) {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (other.board[r][c]) {
                char symbol = other.board[r][c]->getSymbol();
                bool isWhite = other.board[r][c]->isWhitePiece();
                switch (std::tolower(symbol)) {
                    case 'p': board[r][c] = new Pawn(isWhite); break;
                    case 'r': board[r][c] = new Rook(isWhite); break;
                    case 'n': board[r][c] = new Knight(isWhite); break;
                    case 'b': board[r][c] = new Bishop(isWhite); break;
                    case 'q': board[r][c] = new Queen(isWhite); break;
                    case 'k': board[r][c] = new King(isWhite); break;
                    default:  board[r][c] = nullptr; break;
                }
                board[r][c]->setHasMoved(other.board[r][c]->getHasMoved());
            } else {
                board[r][c] = nullptr;
            }
        }
    }

    whiteToMove = other.whiteToMove;
}

bool Board::getWhiteToMove() const {
    return whiteToMove;
}

// So far only for testing:
void Board::print() const {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece* p = board[r][c];
            if (p)
                std::cout << p->getSymbol();
            else
                std::cout << '.';
            std::cout << ' ';
        }
        std::cout << '\n';
    }
}