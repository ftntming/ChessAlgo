package ui;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.Point;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.filechooser.FileNameExtensionFilter;

import engine.BackendBridge;

public class ChessApp {

    private static BoardPanel boardPanel;
    protected static boolean whiteToMove = true;


    public static void main(String[] args) {
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception ignored) {
        }

        // If the native library failed to load, show an error dialog and exit.
        if (!BackendBridge.isLibraryLoaded()) {
            final String err = BackendBridge.getLibraryLoadError();
            SwingUtilities.invokeLater(() -> {
                JOptionPane.showMessageDialog(null,
                        "Native library failed to load:\n" + (err == null ? "Unknown error" : err),
                        "Native library error", JOptionPane.ERROR_MESSAGE);
                System.exit(1);
            });
            return;
        }

        SwingUtilities.invokeLater(() -> {
            JFrame frame = new JFrame("Java Chess");
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setResizable(false);
            frame.setLayout(new BorderLayout());

            // Main board
            boardPanel = new BoardPanel();
            frame.add(boardPanel, BorderLayout.CENTER);

            // Create menu bar
            JMenuBar menuBar = new JMenuBar();
            JMenu fileMenu = new JMenu("File");

            JMenuItem loadFenItem = new JMenuItem("Load FEN");
            loadFenItem.addActionListener(e -> {
                JFileChooser fileChooser = new JFileChooser();
                fileChooser.setDialogTitle("Load FEN from file");
                fileChooser.setFileFilter(new FileNameExtensionFilter("FEN files (*.fen)", "fen"));
                int result = fileChooser.showOpenDialog(frame);
                if (result != JFileChooser.APPROVE_OPTION) return;

                File selectedFile = fileChooser.getSelectedFile();
                try {
                    String fen = new String(java.nio.file.Files.readAllBytes(selectedFile.toPath()), StandardCharsets.UTF_8).trim();
                    if (fen.isEmpty()) {
                        JOptionPane.showMessageDialog(frame, "Selected FEN file is empty", "Empty FEN", JOptionPane.WARNING_MESSAGE);
                        return;
                    }

                    BackendBridge.loadFEN(fen);

                    // Fetch the board state from the native backend to sync the UI
                    String[][] boardState = BackendBridge.getBoardState();
                    if (boardState != null) {
                        boardPanel.updateBoard(boardState);
                    } else {
                        JOptionPane.showMessageDialog(frame, "Failed to load FEN", "Error", JOptionPane.ERROR_MESSAGE);
                    }

                    // Parse FEN to get white-to-move flag
                    String[] parts = fen.split(" ");
                    if (parts.length > 1) {
                        ChessApp.whiteToMove = parts[1].equalsIgnoreCase("w");
                    }

                    boardPanel.resetForNewPosition();
                    boardPanel.repaint();
                } catch (IOException ex) {
                    JOptionPane.showMessageDialog(frame, "Error reading FEN file: " + ex.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
                } catch (Exception ex) {
                    JOptionPane.showMessageDialog(frame, "Error loading FEN: " + ex.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
                }
            });

            JMenuItem saveFenItem = new JMenuItem("Save FEN");
            saveFenItem.addActionListener(e -> {
                JFileChooser fileChooser = new JFileChooser();
                fileChooser.setDialogTitle("Save FEN to File");
                fileChooser.setSelectedFile(new File("chess_position.fen"));
                fileChooser.setFileFilter(new FileNameExtensionFilter("FEN files (*.fen)", "fen"));

                int result = fileChooser.showSaveDialog(frame);
                if (result == JFileChooser.APPROVE_OPTION) {
                    File selectedFile = fileChooser.getSelectedFile();

                    try {
                        // Get the current FEN from the native backend
                        String fen = generateFenFromBoard();

                        // Write FEN to file
                        try (BufferedWriter writer = new BufferedWriter(new FileWriter(selectedFile))) {
                            writer.write(fen);
                            writer.newLine();
                        }

                        JOptionPane.showMessageDialog(frame,
                            "FEN saved successfully to:\n" + selectedFile.getAbsolutePath(),
                            "Success",
                            JOptionPane.INFORMATION_MESSAGE);
                    } catch (IOException ex) {
                        JOptionPane.showMessageDialog(frame,
                            "Error saving FEN: " + ex.getMessage(),
                            "Error",
                            JOptionPane.ERROR_MESSAGE);
                    }
                }
            });

            JMenuItem exitItem = new JMenuItem("Exit");
            exitItem.addActionListener(e -> System.exit(0));

            fileMenu.add(loadFenItem);
            fileMenu.add(saveFenItem);
            fileMenu.addSeparator();
            fileMenu.add(exitItem);

            menuBar.add(fileMenu);
            frame.setJMenuBar(menuBar);

            // Controls panel (bottom) - only Suggest Move button
            JButton suggestButton = new JButton("Suggest Move");
            suggestButton.addActionListener(function -> {
                suggestButton.setEnabled(false);
                boardPanel.generateSuggestedMove(() -> {
                    // This callback runs on the EDT (BoardPanel invokes it via SwingUtilities.invokeLater)
                    try {
                        suggestButton.setEnabled(true);
                    } catch (Throwable ignored) {
                    }
                });
            });

            JPanel buttonPanel = new JPanel();
            buttonPanel.setLayout(new FlowLayout());
            buttonPanel.add(suggestButton);

            frame.add(buttonPanel, BorderLayout.SOUTH);

            frame.pack();
            frame.setLocationRelativeTo(null);
            frame.setVisible(true);
        });
    }


    public static void makeMove(Point from, Point to) {
        String move = convertPointsToMove(from, to);

        boolean legal = BackendBridge.applyMove(move);
        if (legal) {
            boardPanel.updateBoard(BackendBridge.getBoardState()); // <- Syncs Java board with C++
            // LATER: Maybe should handle Java and C++ board sync in BackendBridge instead
            checkGameOver();
            switchTurn();
        } else {
            System.out.println("Invalid move"); // change to UI later
        }
    }

    public static Boolean whiteToMove() {
        return whiteToMove;
    }

    private static String convertPointsToMove(Point from, Point to) {
        return pointToCoord(from) + pointToCoord(to);
    }

    private static String pointToCoord(Point p) {
        char file = (char) ('a' + p.y);
        int rank = 8 - p.x;
        return "" + file + rank;
    }

    private static void switchTurn() {
        whiteToMove = !whiteToMove;
    }

    private static void checkGameOver() {
        if (BackendBridge.isCheckmate()) {
            JOptionPane.showMessageDialog(null, (whiteToMove ? "White" : "Black") + " wins by checkmate!");
        }
    }

    private static String generateFenFromBoard() {
        String[][] board = BackendBridge.getBoardState();
        StringBuilder fen = new StringBuilder();

        if (board == null) {
            throw new IllegalStateException("Could not fetch board state from backend");
        }

        // Build the piece placement part of the FEN
        for (int r = 0; r < 8; r++) {
            if (r > 0) {
                fen.append("/");
            }

            int emptyCount = 0;
            for (int c = 0; c < 8; c++) {
                String piece = board[r][c];
                if (piece == null || piece.isEmpty()) {
                    emptyCount++;
                } else {
                    if (emptyCount > 0) {
                        fen.append(emptyCount);
                        emptyCount = 0;
                    }
                    fen.append(piece);
                }
            }

            if (emptyCount > 0) {
                fen.append(emptyCount);
            }
        }

        // Add active color
        fen.append(" ");
        fen.append(whiteToMove ? "w" : "b");

        // Add simplified castling rights, en passant, halfmove, fullmove
        fen.append(" - - 0 1");

        return fen.toString();
    }
}