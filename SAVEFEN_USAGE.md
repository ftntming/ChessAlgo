# saveFEN Function Usage

## Description
The `saveFEN()` function saves the current board state to a file in FEN (Forsyth-Edwards Notation) format.

## Signature
```cpp
bool Board::saveFEN(const char* file_path) const;
```

## Parameters
- `file_path`: Path to the file where the FEN string should be saved

## Return Value
- `true`: File was successfully written
- `false`: File could not be opened or written

## Example Usage

```cpp
#include "board.h"

int main() {
    Board board;
    
    // Load a starting position
    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1");
    
    // Make some moves...
    
    // Save the current position to a file
    if (board.saveFEN("my_game.fen")) {
        std::cout << "Position saved successfully!" << std::endl;
    } else {
        std::cerr << "Failed to save position" << std::endl;
    }
    
    return 0;
}
```

## FEN Format
The saved FEN string includes:
1. **Piece placement**: Board position from rank 8 to rank 1, files a-h
2. **Active color**: 'w' for white, 'b' for black
3. **Castling rights**: Currently saved as '-' (simplified)
4. **En passant**: Currently saved as '-' (simplified)
5. **Halfmove clock**: Currently saved as '0' (simplified)
6. **Fullmove number**: Currently saved as '1' (simplified)

Example output:
```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1
```

## Notes
- The function creates the file if it doesn't exist
- Existing files will be overwritten
- Error messages are written to stderr
- Success messages are written to stderr with `[INFO]` prefix

