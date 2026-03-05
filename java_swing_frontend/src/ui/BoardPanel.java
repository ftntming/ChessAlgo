package ui;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Point;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;

import javax.swing.ImageIcon;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;

public class BoardPanel extends JPanel {
    private static final int TILE_SIZE = 64;
    private String[][] board = new String[8][8];

    private Point selectedSquare = null;
    private Point suggestedFrom = null;
    private Point suggestedTo = null;

    private java.util.List<Point> legalMoves = new ArrayList<>();

    // Async legal-move cache + loading flag
    private volatile String[] allMovesCache = null;
    private volatile boolean loadingAllMoves = false;

    // NEW: loading flag for suggested-move request to avoid concurrent requests
    private volatile boolean loadingSuggestedMove = false;

    private boolean isBusy() {
        return loadingAllMoves || loadingSuggestedMove;
    }

    public BoardPanel() {
        setupStartingPosition();

        addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                int row = e.getY() / TILE_SIZE;
                int col = e.getX() / TILE_SIZE;
                handleClick(row, col);
            }
        });
    }

    // TODO: There is a bug where you can (ghost) move pieces while C++ is running the search
    // It completely messes up everything else and prevents future movement of pieces.
    // Likely due to desync of which colour is to move
    private void handleClick(int row, int col) {
        // Freeze board interactions while async operations are in progress
        if (isBusy()) {
            System.err.println("[DEBUG] Board is busy, ignoring click at (" + row + ", " + col + ")");
            return;
        }

        if (!isInBounds(row, col)) {
            resetSelection();
            repaint();
            return;
        }

        if (selectedSquare == null) { // no square currently selected, select current if valid
            java.util.List<Point> foundMoves = getLegalDestinationsFrom(row, col);
            if (foundMoves.isEmpty()) return;
            selectedSquare = new Point(row, col);
            legalMoves = foundMoves;
        } else {
            Point from = selectedSquare;
            Point to = new Point(row, col);

            if (from.x == row && from.y == col) {
                resetSelection(); // clicked on the same square twice in a row, deselect
                repaint();
                return;
            }

            if (legalMoves.contains(to)) {
                ChessApp.makeMove(from, to); // Only legal moves allowed
                resetSelection();
            } else {
                java.util.List<Point> newMoves = getLegalDestinationsFrom(row, col);
                if (!newMoves.isEmpty()) {
                    selectedSquare = new Point(row, col); // select clicked square if it is valid
                    legalMoves = newMoves;
                } else {
                    resetSelection(); // deselect
                }
            }
        }
        repaint(); // show changes
    }

    private void resetSelection() {
        selectedSquare = null;
        legalMoves.clear();
        suggestedFrom = null;
        suggestedTo = null;
    }

    public void resetForNewPosition() {
        resetSelection();
        allMovesCache = null;  // Invalidate cached moves for new position
    }

    private void setupStartingPosition() {
        board[0] = new String[] { "r", "n", "b", "q", "k", "b", "n", "r" };
        board[1] = new String[] { "p", "p", "p", "p", "p", "p", "p", "p" };

        for (int r = 2; r <= 5; r++)
            board[r] = new String[] { null, null, null, null, null, null, null, null };

        board[6] = new String[] { "P", "P", "P", "P", "P", "P", "P", "P" };
        board[7] = new String[] { "R", "N", "B", "Q", "K", "B", "N", "R" };
    }

    public void updateBoard(String[][] newBoard) {
        this.board = newBoard;
        // Board changed; invalidate cached legal moves so next request triggers a fresh fetch
        allMovesCache = null;
        repaint();
    }

    @Override
    public Dimension getPreferredSize() {
        return new Dimension(8 * TILE_SIZE, 8 * TILE_SIZE);
    }

    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);

        // Draws out the tiles of the chessboard and pieces
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                // Tiles have a checkerboard pattern
                boolean isLight = (row + col) % 2 == 0;
                g.setColor(isLight ? new Color(240, 217, 181) : new Color(181, 136, 99));
                g.fillRect(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE);

                // Highlight legal moves for selected piece (if there is one) as translucent green
                if (legalMoves.contains(new Point(row, col))) {
                    g.setColor(new Color(0, 255, 0, 100)); // translucent green
                    g.fillRect(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE);
                }

                // Highlight the suggested move as translucent orange
                if ((suggestedFrom != null && new Point(row, col).equals(suggestedFrom)) ||
                        (suggestedTo != null && new Point(row, col).equals(suggestedTo))) {
                    g.setColor(new Color(255, 165, 0, 120)); // translucent orange
                    g.fillRect(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE);
                }
            }
        }


        // Highlight selected piece as yellow
        if (selectedSquare != null) {
            g.setColor(new Color(255, 255, 0, 100)); // translucent yellow
            g.fillRect(selectedSquare.y * TILE_SIZE, selectedSquare.x * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }

        // LATER: Highlight king with red, if in check. Low priority

        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                // if there's a piece on this tile, draw it
                String code = board[row][col];
                if (code != null) {
                    ImageIcon icon = PieceIconLoader.getIcon(code); // loads images from resources/ folder
                    g.drawImage(icon.getImage(), col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, null);
                }
            }
        }

        // Visual indicator: semi-transparent overlay when board is busy
        if (isBusy()) {
            g.setColor(new Color(128, 128, 128, 80)); // semi-transparent gray
            g.fillRect(0, 0, 8 * TILE_SIZE, 8 * TILE_SIZE);
        }
    }

    // NEW: helper which uses the cached allMovesCache without triggering fetch
    private java.util.List<Point> getLegalDestinationsFromCached(int row, int col) {
        java.util.List<Point> moves = new ArrayList<>();
        if (!isInBounds(row, col)) return moves;

        String piece = board[row][col];
        if (piece == null || Character.isUpperCase(piece.charAt(0)) != ChessApp.whiteToMove()) {
            return moves;
        }

        String[] allMoves = allMovesCache; // read volatile once
        if (allMoves == null) return moves;

        String fromCoord = toAlgebraic(new Point(row, col));

        for (String move : allMoves) {
            if (move == null || move.length() < 4) continue;
            if (!move.startsWith(fromCoord)) continue;

            String to = move.substring(2, 4);
            Point toPoint = fromAlgebraic(to);
            if (toPoint != null && isInBounds(toPoint.x, toPoint.y)) {
                moves.add(toPoint);
            }
        }

        return moves;
    }

    // Modified to use async fetching: returns cached results if present; otherwise triggers background fetch and returns empty list
    private java.util.List<Point> getLegalDestinationsFrom(int row, int col) {
        java.util.List<Point> cached = getLegalDestinationsFromCached(row, col);
        if (!cached.isEmpty()) {
            return cached;
        }

        // If cache empty and we're not already loading, start loading in background
        if (allMovesCache == null && !loadingAllMoves) {
            fetchLegalMovesAsync();
        }

        return new ArrayList<>(); // return empty for now; UI will update when background fetch completes
    }

    // Runs engine.BackendBridge.getLegalMoves() on a background thread and updates UI when done
    private synchronized void fetchLegalMovesAsync() {
        if (loadingAllMoves) return;
        loadingAllMoves = true;

        Thread t = new Thread(() -> {
            try {
                String[] moves = null;
                try {
                    moves = engine.BackendBridge.getLegalMoves();
                } catch (Throwable tInner) {
                    System.err.println("Error fetching legal moves from backend: " + tInner);
                }

                allMovesCache = moves; // may be null on error

            } finally {
                loadingAllMoves = false;
                // Update UI on EDT: recompute legalMoves for current selection and repaint
                SwingUtilities.invokeLater(() -> {
                    if (selectedSquare != null) {
                        legalMoves = getLegalDestinationsFromCached(selectedSquare.x, selectedSquare.y);
                    }
                    repaint();
                });
            }
        }, "LegalMovesFetcher");
        t.setDaemon(true);
        t.start();
    }

    // Inverse of below
    private static String toAlgebraic(Point p) {
        char file = (char) ('a' + p.y);
        int rank = 8 - p.x;
        return "" + file + rank;
    }

    // Converting portions of FEN (eg. "a2", "e4") to Point(row, col)
    // eg. "a2" -> Point(6, 0) since row 6 is the second to last row and column 0 is the 'a' file
    // Note: On chessboard, row 1 is on the BOTTOM
    private static Point fromAlgebraic(String s) {
        if (s == null || s.length() != 2) {
            return null;
        }

        char fileChar = Character.toLowerCase(s.charAt(0));
        char rankChar = s.charAt(1);
        if (fileChar < 'a' || fileChar > 'h' || rankChar < '1' || rankChar > '8') {
            return null;
        }

        int col = fileChar - 'a';
        int row = 8 - Character.getNumericValue(rankChar);
        if (!isInBounds(row, col)) {
            return null;
        }
        return new Point(row, col);
    }

    // Ask the CPP backend for the suggested move given the current board state
    // Backward-compatible no-arg method: delegates to the new method with null callback
    public void generateSuggestedMove() {
        generateSuggestedMove(null);
    }

    // New: accepts an optional callback to run on the EDT when the suggestion completes
    public void generateSuggestedMove(Runnable onComplete) {
        // If a suggestion request is already in progress, don't start another
        synchronized (this) {
            if (loadingSuggestedMove) return;
            loadingSuggestedMove = true;
        }

        Thread t = new Thread(() -> {
            String move = null;
            try {
                try {
                    move = engine.BackendBridge.getSuggestedMove();
                } catch (Throwable tInner) {
                    System.err.println("Error fetching suggested move from backend: " + tInner);
                }

                final String suggested = move; // final for use in EDT

                SwingUtilities.invokeLater(() -> {
                    try {
                        if (suggested != null && suggested.length() == 4) {
                            Point from = fromAlgebraic(suggested.substring(0, 2));
                            Point to = fromAlgebraic(suggested.substring(2, 4));
                            if (from != null && to != null) {
                                suggestedFrom = from;
                                suggestedTo = to;
                            } else {
                                suggestedFrom = null;
                                suggestedTo = null;
                            }
                        } else {
                            suggestedFrom = null;
                            suggestedTo = null;
                        }
                        repaint();

                        // Run completion callback if provided (still on EDT)
                        if (onComplete != null) {
                            try { onComplete.run(); } catch (Throwable cb) { System.err.println("suggested-move callback failed: " + cb); }
                        }
                    } finally {
                        loadingSuggestedMove = false;
                    }
                });
            } catch (Throwable outer) {
                // Ensure flag cleared even if something unexpected happens
                loadingSuggestedMove = false;
                System.err.println("Unexpected error in suggested-move fetcher: " + outer);
                // Run callback to allow caller to re-enable UI controls
                if (onComplete != null) {
                    try { SwingUtilities.invokeLater(onComplete); } catch (Throwable cb) { System.err.println("suggested-move callback failed: " + cb); }
                }
            }
        }, "SuggestedMoveFetcher");
        t.setDaemon(true);
        t.start();
    }

    public void loadFEN(String fen) {
        String[] parts = fen.split(" ");
        if (parts.length < 1)
            return;

        String[] rows = parts[0].split("/");
        if (rows.length != 8) {
            return;
        }

        for (int r = 0; r < 8; r++) {
            String row = rows[r];
            int c = 0;
            for (char ch : row.toCharArray()) {
                if (Character.isDigit(ch)) {
                    int emptyCount = ch - '0';
                    for (int i = 0; i < emptyCount && c < 8; i++) {
                        board[r][c++] = null;
                    }
                } else if (c < 8) {
                    board[r][c++] = String.valueOf(ch);
                }
            }
            while (c < 8) {
                board[r][c++] = null;
            }
        }

        // Update white to move
        if (parts.length > 1) {
            boolean whiteToMove = parts[1].equalsIgnoreCase("w");
            ChessApp.whiteToMove = whiteToMove;

        }

        // reset
        selectedSquare = null;
        legalMoves.clear();
        suggestedFrom = null;
        suggestedTo = null;

        // Invalidate cached moves when loading a new position
        allMovesCache = null;

        repaint(); // Update the board with the new positions from the FEN string
    }

    private static boolean isInBounds(int row, int col) {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
}

