# Save FEN Feature

## Overview
A "Save FEN" button has been added to the Chess App that allows users to save the current board position to a file in FEN (Forsyth-Edwards Notation) format.

## Features

### Save FEN Button
- Located in the bottom control panel, next to "Load FEN" and "Suggest Move" buttons
- Opens a file dialog when clicked
- Default filename: `chess_position.fen`
- Filters to show only `.fen` files

### File Dialog
- Allows users to:
  - Choose save location
  - Change filename
  - Overwrite existing files
  - Cancel the operation

### FEN Generation
- Fetches the current board state from the native C++ backend
- Constructs a valid FEN string including:
  - Piece placement (board position from rank 8 to rank 1)
  - Active color (white or black to move)
  - Simplified castling rights (set to '-')
  - Simplified en passant (set to '-')
  - Halfmove clock (set to '0')
  - Fullmove number (set to '1')

## Example Usage

1. Play a game or load a position
2. Click the "Save FEN" button
3. In the file dialog:
   - Choose where to save the file
   - Optionally change the filename
   - Click "Save"
4. A success message confirms the file was saved
5. You can later load this FEN back using "Load FEN"

## Error Handling

- **Empty board state**: Shows error if board state cannot be fetched
- **File I/O errors**: Shows error if file cannot be written
- **Cancelled save**: No action if user cancels the file dialog

All errors are displayed in user-friendly dialog boxes.

## Technical Details

### New Components
- `JFileChooser` for file selection dialog
- `BufferedWriter` for writing FEN to file
- `FileNameExtensionFilter` for FEN file filtering
- `generateFenFromBoard()` method to construct FEN string

### Method: `generateFenFromBoard()`
```java
private static String generateFenFromBoard()
```

Constructs a FEN string from the current board state by:
1. Fetching board state from `BackendBridge.getBoardState()`
2. Iterating through each row and column
3. Counting consecutive empty squares
4. Building the FEN notation string
5. Appending active color and simplified parameters

## Files Modified
- `java_swing_frontend/src/ui/ChessApp.java`
  - Added imports for file dialog components
  - Added "Save FEN" button with action listener
  - Added to button panel
  - Added `generateFenFromBoard()` helper method

