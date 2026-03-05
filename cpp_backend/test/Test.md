debug 

Source File: cpp_backend/test/test_run_fen.cpp

Compiler Options: -std=c++17 -Wall -g -Icpp_backend/src cpp_backend/src/board.cpp cpp_backend/src/movegen.cpp cpp_backend/src/search.cpp cpp_backend/src/evaluate.cpp cpp_backend/src/pieces/bishop.cpp cpp_backend/src/pieces/king.cpp cpp_backend/src/pieces/knight.cpp cpp_backend/src/pieces/pawn.cpp cpp_backend/src/pieces/queen.cpp cpp_backend/src/pieces/rook.cpp

Working directory: /Volumes/Data/go/src/github.com/ftntming/ChessAlgo



Why it was so slow before:
Depth 8 vs Depth 4:
Depth 8: Would search millions/billions of nodes per move
Depth 4: Searches ~20,000-60,000 nodes per move (manageable)
Speed difference: Depth 8 is roughly 100-1000x slower than depth 4
Debug logging overhead:
Even gated debug logs add checking overhead on every node
With debug off, you get cleaner output and faster execution

