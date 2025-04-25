#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <QMainWindow>
#include <QTimer>
#include <QKeyEvent>
#include <QMutex>
#include <QWaitCondition>
#include <QLabel>
#include <pthread.h>
#include "all_includes.h"
#include "renderer.hpp"

// Enumeration for visualization modes
enum class what_to_paint {
    function,
    approximation,
    residual
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(double a, double b, double c, double d, 
               int nx, int ny, int mx, int my, 
               int k, double eps, int max_its, int p);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void updateUI();

private:
    // Computational parameters
    double a, b, c, d;      // Boundaries
    int nx, ny;             // Computational grid dimensions
    int mx, my;             // Visualization grid dimensions
    int k;                  // Function number (0-7)
    double eps;             // Accuracy parameter
    int max_its;            // Maximum iterations
    int p;                  // Number of threads
    double zoom_factor;     // Current zoom factor
    
    // UI elements
    Renderer *renderer;     // Custom rendering widget
    QLabel *infoLabel;      // Information panel
    QTimer *timer;          // Timer for UI updates
    
    // Visualization state
    what_to_paint paint_mode;
    
    // Multithreading
    pthread_t *threads;
    pthread_t main_thread;  // Thread handle for main computation thread
    Args *args;
    QMutex dataMutex;
    QWaitCondition dataReady;
    bool running;
    bool terminating;
    
    // Computational data
    double *A;              // Matrix A
    int *I;                 // Matrix I indices
    double *B;              // Right-hand side vector
    double *x;              // Solution vector
    double *r;              // Residual vector
    double *u, *v;          // Work vectors
    Functions func;         // Function object
    
    // Private methods
    void startComputation();
    void toggleFunction();
    void toggleRenderMode();
    void zoomIn();
    void zoomOut();
    void increaseGridDimension();
    void decreaseGridDimension();
    void increaseEpsilon();
    void decreaseEpsilon();
    void increaseVisualizationDetail();
    void decreaseVisualizationDetail();
    void updateInfoPanel();
    void showHelp();        // Метод для отображения справки по командам
    
    // Logical to graphical coordinate conversion
    QPointF l2g(double x, double y);
};

#endif // WINDOW_HPP 