#ifndef BOARD_H
#define BOARD_H

#include <string>
#include <vector>
#include "pieces/pieces.h" 
#include "undoState.h"

class Board {
public:
    Board();
    ~Board();

    Board(const Board& other); // Copy constructor

    // Load a position using a FEN string
    void loadFEN(const std::string& fen);

    // Save the current board position to a FEN file
    bool saveFEN(const char* file_path) const;

    // Print the board to console (debugging)
    void print() const;

    // Get the piece at a specific position (returns nullptr if empty)
    Piece* getPiece(int row, int col) const;

    // Place a piece at the given location (replaces any existing piece)
    void setPiece(int row, int col, Piece* piece);

    // Remove (delete) the piece at the given location
    void removePiece(int row, int col);

    // Check if the given color's king is in check
    bool isKingInCheck(bool white) const;
    
    // Check if the given color's king is in checkmate
    bool isCheckmate(bool white) const;

    // Make a move
    bool makeMove(const Move& move, bool skipValidation = false);

    // Undo the last move made. This will require storing a history of moves
    bool undoMove();

    bool getWhiteToMove() const;

private:
    Piece* board[8][8]; // nullptr if square is empty

    std::vector<UndoState> history;

    // Helper: create a piece object from a FEN character
    Piece* createPieceFromSymbol(char symbol);

    bool whiteToMove;
};

#endif
