#include <iostream>
#include <fstream>
#include <string>
#include "../src/board.h"

int main() {
    Board board;

    // Test 1: Save starting position
    std::cout << "Test 1: Saving starting position..." << std::endl;
    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1");

    if (!board.saveFEN("/tmp/test_starting.fen")) {
        std::cerr << "[FAIL] Could not save starting position" << std::endl;
        return 1;
    }

    // Read back and verify
    std::ifstream file("/tmp/test_starting.fen");
    std::string savedFen;
    std::getline(file, savedFen);
    file.close();

    std::cout << "Saved FEN: " << savedFen << std::endl;

    if (savedFen.find("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w") == std::string::npos) {
        std::cerr << "[FAIL] Saved FEN does not match expected" << std::endl;
        return 1;
    }

    // Test 2: Save a different position
    std::cout << "\nTest 2: Saving custom position..." << std::endl;
    board.loadFEN("r1bqkbnr/pppp1ppp/2n5/4p3/3P4/2N2N2/PPP1PPPP/R1BQKB1R w - - 0 4");

    if (!board.saveFEN("/tmp/test_custom.fen")) {
        std::cerr << "[FAIL] Could not save custom position" << std::endl;
        return 1;
    }

    std::ifstream file2("/tmp/test_custom.fen");
    std::string savedFen2;
    std::getline(file2, savedFen2);
    file2.close();

    std::cout << "Saved FEN: " << savedFen2 << std::endl;

    if (savedFen2.find("r1bqkbnr/pppp1ppp/2n5/4p3/3P4/2N2N2/PPP1PPPP/R1BQKB1R w") == std::string::npos) {
        std::cerr << "[FAIL] Saved FEN does not match expected" << std::endl;
        return 1;
    }

    // Test 3: Black to move
    std::cout << "\nTest 3: Saving position with black to move..." << std::endl;
    board.loadFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b - - 0 1");

    if (!board.saveFEN("/tmp/test_black.fen")) {
        std::cerr << "[FAIL] Could not save black-to-move position" << std::endl;
        return 1;
    }

    std::ifstream file3("/tmp/test_black.fen");
    std::string savedFen3;
    std::getline(file3, savedFen3);
    file3.close();

    std::cout << "Saved FEN: " << savedFen3 << std::endl;

    if (savedFen3.find(" b ") == std::string::npos) {
        std::cerr << "[FAIL] Saved FEN should have 'b' for black to move" << std::endl;
        return 1;
    }

    std::cout << "\n[PASS] All saveFEN tests passed!" << std::endl;
    return 0;
}

