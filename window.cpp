#include "window.hpp"
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMessageBox>
#include <QCloseEvent>
#include <cmath>
#include <cstring>
#include <sstream>

// Thread function prototype
void* gui_solution(void* ptr);

MainWindow::MainWindow(double a, double b, double c, double d, 
                       int nx, int ny, int mx, int my, 
                       int k, double eps, int max_its, int p)
    : a(a), b(b), c(c), d(d), 
      nx(nx), ny(ny), mx(mx), my(my), 
      k(k), eps(eps), max_its(max_its), p(p),
      zoom_factor(1.0), paint_mode(what_to_paint::function),
      running(false), terminating(false) {
          
    // Set up the main window
    setWindowTitle("2D Function Approximation");
    setMinimumSize(100, 100);
    resize(1000, 1000);
    
    // Initialize function
    func.select_f(k);
    
    // Create renderer
    renderer = new Renderer(this);
    renderer->setBoundaries(a, b, c, d);
    renderer->setFunction(func.f);
    renderer->setRenderMode(paint_mode);
    
    // Create info label
    infoLabel = new QLabel(this);
    infoLabel->setStyleSheet("QLabel { color: #00008B; background-color: #F0F0F0; padding: 5px; }");
    
    // Set up layout
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(renderer);
    layout->addWidget(infoLabel);
    
    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    
    // Set up timer for UI updates
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateUI);
    timer->start(100); // Update every 100ms
    
    // Allocate memory for computation
    int n = (nx + 1) * (ny + 1);
    
    I = nullptr;
    if (allocate_msr_matrix(nx, ny, &A, &I)) {
        QMessageBox::critical(this, "Error", "Failed to allocate MSR matrix.");
        close();
        return;
    }
    
    init_reduce_sum(p);
    
    B = new double[n];
    x = new double[n];
    r = new double[n];
    u = new double[n];
    v = new double[n];
    
    fill_I(nx, ny, I);
    
    // Initialize solution vector with zeros
    memset(x, 0, n * sizeof(double));
    
    // Allocate thread resources
    threads = new pthread_t[p];
    args = new Args[p];
    
    // Start computation in separate threads
    startComputation();
    
    // Update info panel
    updateInfoPanel();
}

MainWindow::~MainWindow() {
    // Signal threads to terminate
    terminating = true;
    dataMutex.lock();
    dataReady.wakeAll();
    dataMutex.unlock();
    
    // Wait for threads to finish
    for (int i = 1; i < p; ++i) {
        pthread_join(threads[i], nullptr);
    }
    
    // Free memory
    free_results();
    delete[] I;
    delete[] A;
    delete[] B;
    delete[] x;
    delete[] r;
    delete[] u;
    delete[] v;
    delete[] args;
    delete[] threads;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    // Don't process keys if computation is running
    if (running) {
        QMessageBox::information(this, "Information", "Please wait until computation is completed.");
        return;
    }
    
    switch (event->key()) {
        case Qt::Key_0:
            toggleFunction();
            break;
        case Qt::Key_1:
            toggleRenderMode();
            break;
        case Qt::Key_2:
            zoomIn();
            break;
        case Qt::Key_3:
            zoomOut();
            break;
        case Qt::Key_4:
            increaseGridDimension();
            break;
        case Qt::Key_5:
            decreaseGridDimension();
            break;
        case Qt::Key_6:
            increaseEpsilon();
            break;
        case Qt::Key_7:
            decreaseEpsilon();
            break;
        case Qt::Key_8:
            increaseVisualizationDetail();
            break;
        case Qt::Key_9:
            decreaseVisualizationDetail();
            break;
        default:
            QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (running) {
        QMessageBox::warning(this, "Warning", "Computation is in progress. Please wait until it completes.");
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::updateUI() {
    if (!running) {
        // Update renderer with current data
        renderer->setData(x, nx + 1, ny + 1);
        renderer->update();
    }
}

void MainWindow::startComputation() {
    running = true;
    
    // Initialize arguments for each thread
    for (int i = 0; i < p; ++i) {
        args[i].a = a;
        args[i].b = b;
        args[i].c = c;
        args[i].d = d;
        args[i].eps = eps;
        args[i].I = I;
        args[i].A = A;
        args[i].B = B;
        args[i].x = x;
        args[i].r = r;
        args[i].u = u;
        args[i].v = v;
        args[i].nx = nx;
        args[i].ny = ny;
        args[i].maxit = max_its;
        args[i].p = p;
        args[i].k = i;
        args[i].f = func.f;
    }
    
    // Start threads
    for (int i = 1; i < p; ++i) {
        pthread_create(&threads[i], nullptr, &gui_solution, &args[i]);
    }
    
    // Create a separate thread for the main thread's computation
    pthread_t main_thread;
    pthread_create(&main_thread, nullptr, &gui_solution, &args[0]);
    pthread_join(main_thread, nullptr);
    
    // Join other threads
    for (int i = 1; i < p; ++i) {
        pthread_join(threads[i], nullptr);
    }
    
    running = false;
    
    // Get results from computation
    int its = args[0].its;
    double r1 = args[0].res_1;
    double r2 = args[0].res_2;
    double r3 = args[0].res_3;
    double r4 = args[0].res_4;
    double t1 = args[0].t1;
    double t2 = args[0].t2;

    // Print results to terminal in the required format
    const int task = 6;

    printf(
        "gui_app : Task = %d R1 = %e R2 = %e R3 = %e R4 = %e T1 = %.2f T2 = %.2f\n"
        "      It = %d E = %e K = %d Nx = %d Ny = %d P = %d\n",
        task, 
        r1, r2, r3, r4, 
        t1, t2, 
        its, eps, k, 
        nx, ny, p);
    
    // Update info panel
    updateInfoPanel();
}

void MainWindow::toggleFunction() {
    k = (k + 1) % 8;
    func.select_f(k);
    renderer->setFunction(func.f);
    
    // Restart computation with new function
    startComputation();
}

void MainWindow::toggleRenderMode() {
    switch (paint_mode) {
        case what_to_paint::function:
            paint_mode = what_to_paint::approximation;
            break;
        case what_to_paint::approximation:
            paint_mode = what_to_paint::residual;
            break;
        case what_to_paint::residual:
            paint_mode = what_to_paint::function;
            break;
    }
    
    renderer->setRenderMode(paint_mode);
    renderer->update();
}

void MainWindow::zoomIn() {
    zoom_factor *= 2.0;
    renderer->setZoom(zoom_factor);
    updateInfoPanel();
}

void MainWindow::zoomOut() {
    zoom_factor = 1.0;
    renderer->setZoom(zoom_factor);
    updateInfoPanel();
}

void MainWindow::increaseGridDimension() {
    nx *= 2;
    ny *= 2;
    
    // Reallocate memory for computation
    int n = (nx + 1) * (ny + 1);
    
    // Free old memory
    delete[] I;
    delete[] A;
    delete[] B;
    delete[] x;
    delete[] r;
    delete[] u;
    delete[] v;
    
    // Allocate new memory
    I = nullptr;
    if (allocate_msr_matrix(nx, ny, &A, &I)) {
        QMessageBox::critical(this, "Error", "Failed to allocate MSR matrix.");
        close();
        return;
    }
    
    B = new double[n];
    x = new double[n];
    r = new double[n];
    u = new double[n];
    v = new double[n];
    
    fill_I(nx, ny, I);
    
    // Initialize solution vector with zeros
    memset(x, 0, n * sizeof(double));
    
    // Restart computation
    startComputation();
}

void MainWindow::decreaseGridDimension() {
    if (nx <= 5 || ny <= 5) {
        QMessageBox::warning(this, "Warning", "Grid dimensions cannot be less than 5.");
        return;
    }
    
    nx /= 2;
    ny /= 2;
    
    // Reallocate memory for computation
    int n = (nx + 1) * (ny + 1);
    
    // Free old memory
    delete[] I;
    delete[] A;
    delete[] B;
    delete[] x;
    delete[] r;
    delete[] u;
    delete[] v;
    
    // Allocate new memory
    I = nullptr;
    if (allocate_msr_matrix(nx, ny, &A, &I)) {
        QMessageBox::critical(this, "Error", "Failed to allocate MSR matrix.");
        close();
        return;
    }
    
    B = new double[n];
    x = new double[n];
    r = new double[n];
    u = new double[n];
    v = new double[n];
    
    fill_I(nx, ny, I);
    
    // Initialize solution vector with zeros
    memset(x, 0, n * sizeof(double));
    
    // Restart computation
    startComputation();
}

void MainWindow::increaseEpsilon() {
    eps *= 10.0;
    
    // Restart computation
    startComputation();
}

void MainWindow::decreaseEpsilon() {
    eps /= 10.0;
    
    // Restart computation
    startComputation();
}

void MainWindow::increaseVisualizationDetail() {
    mx *= 2;
    my *= 2;
    updateInfoPanel();
}

void MainWindow::decreaseVisualizationDetail() {
    if (mx <= 5 || my <= 5) {
        QMessageBox::warning(this, "Warning", "Visualization detail cannot be less than 5.");
        return;
    }
    
    mx /= 2;
    my /= 2;
    updateInfoPanel();
}

void MainWindow::updateInfoPanel() {
    std::ostringstream oss;
    
    // Add function information
    oss << "Function: ";
    switch (k) {
        case 0: oss << "f(x,y) = 1"; break;
        case 1: oss << "f(x,y) = x"; break;
        case 2: oss << "f(x,y) = y"; break;
        case 3: oss << "f(x,y) = x + y"; break;
        case 4: oss << "f(x,y) = sqrt(x² + y²)"; break;
        case 5: oss << "f(x,y) = x² + y²"; break;
        case 6: oss << "f(x,y) = exp(x² - y²)"; break;
        case 7: oss << "f(x,y) = 1/(25(x² + y²) + 1)"; break;
    }
    
    // Add grid information
    oss << " | Grid: " << nx << "x" << ny;
    
    // Add visualization detail
    oss << " | Viz: " << mx << "x" << my;
    
    // Add zoom information
    oss << " | Zoom: " << zoom_factor << "x";
    
    // Add epsilon information
    oss << " | ε: " << eps;
    
    // Add max value information
    oss << " | Max value: " << renderer->getMaxValue();
    
    // Update label
    infoLabel->setText(oss.str().c_str());
}

QPointF MainWindow::l2g(double x, double y) {
    return renderer->l2g(x, y);
}

// Thread function implementation
void* gui_solution(void* ptr) {
    return solution(ptr);
} 