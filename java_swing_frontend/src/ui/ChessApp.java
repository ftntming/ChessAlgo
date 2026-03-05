package ui;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.Point;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;

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

            // Controls panel (bottom)
            JButton suggestButton = new JButton("Suggest Move");
            // When clicked: disable the button, request a suggestion asynchronously, re-enable when done
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

            JTextField fenField = new JTextField("", 30);
            JButton loadButton = new JButton("Load FEN");
            loadButton.addActionListener(e -> {
                String fen = fenField.getText().trim();
                boardPanel.loadFEN(fen);

                BackendBridge.loadFEN(fen);
            });

            JPanel buttonPanel = new JPanel();
            buttonPanel.setLayout(new FlowLayout());
            buttonPanel.add(fenField);
            buttonPanel.add(loadButton);
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
}