#include <QApplication>
#include <QMessageBox>
#include <iostream>
#include <string>
#include <stdexcept>
#include <fenv.h>
#include "window.hpp"

int main(int argc, char *argv[]) {
    feenableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW);
    
    QApplication app(argc, argv);
    
    if (argc != 13) {
        std::cerr << "Error: Expected 12 command-line arguments." << std::endl;
        std::cerr << "Usage: " << argv[0] << " a b c d nx ny mx my k epsilon max_iterations threads" << std::endl;
        
        QMessageBox::critical(nullptr, "Error", 
                             "Invalid number of arguments.\n\n"
                             "Usage: gui_app a b c d nx ny mx my k epsilon max_iterations threads\n\n"
                             "Where:\n"
                             "a, b: boundaries in x\n"
                             "c, d: boundaries in y\n"
                             "nx, ny: computational grid dimensions\n"
                             "mx, my: visualization grid dimensions\n"
                             "k: function number (0-7)\n"
                             "epsilon: computation accuracy\n"
                             "max_iterations: maximum number of iterations\n"
                             "threads: number of parallel threads");
        return 1;
    }
    
    double a, b, c, d, eps;
    int nx, ny, mx, my, k, max_its, p;
    
    try {
        a = std::stod(argv[1]);
        b = std::stod(argv[2]);
        c = std::stod(argv[3]);
        d = std::stod(argv[4]);
        nx = std::stoi(argv[5]);
        ny = std::stoi(argv[6]);
        mx = std::stoi(argv[7]);
        my = std::stoi(argv[8]);
        k = std::stoi(argv[9]);
        eps = std::stod(argv[10]);
        max_its = std::stoi(argv[11]);
        p = std::stoi(argv[12]);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: Invalid argument format. All parameters must be valid numbers." << std::endl;
        
        QMessageBox::critical(nullptr, "Error", 
                             "Invalid argument format. All parameters must be valid numbers.");
        return 1;
    } catch (const std::out_of_range& e) {
        std::cerr << "Error: Number out of range." << std::endl;
        
        QMessageBox::critical(nullptr, "Error", "Number out of range.");
        return 1;
    }
    
    // Validate parameters
    if (nx < 5 || ny < 5) {
        QMessageBox::critical(nullptr, "Error", "Grid dimensions nx and ny must be at least 5.");
        return 1;
    }
    
    if (mx < 5 || my < 5) {
        QMessageBox::critical(nullptr, "Error", "Visualization dimensions mx and my must be at least 5.");
        return 1;
    }
    
    if (k < 0 || k > 7) {
        QMessageBox::critical(nullptr, "Error", "Function number k must be between 0 and 7.");
        return 1;
    }
    
    if (p < 1) {
        QMessageBox::critical(nullptr, "Error", "Number of threads must be at least 1.");
        return 1;
    }
    
    if (a >= b || c >= d) {
        QMessageBox::critical(nullptr, "Error", "Invalid boundaries. Must satisfy: a < b and c < d.");
        return 1;
    }
    
    // Create main window
    MainWindow mainWindow(a, b, c, d, nx, ny, mx, my, k, eps, max_its, p);
    mainWindow.show();
    
    // Run application event loop
    return app.exec();
} 