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
          
    setWindowTitle("2D Function Approximation");
    setMinimumSize(100, 100);
    resize(1000, 1000);
    
    func.select_f(k);
    
    renderer = new Renderer(this);
    renderer->setBoundaries(a, b, c, d);
    renderer->setFunction(func.f);
    renderer->setRenderMode(paint_mode);
    
    infoLabel = new QLabel(this);
    infoLabel->setStyleSheet("QLabel { color: #00008B; background-color: #F0F0F0; padding: 2px; font-size: 9px; }");
    infoLabel->setMaximumHeight(20);
    
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(renderer);
    layout->addWidget(infoLabel);
    
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateUI);
    timer->start(50); // Update every 50ms (more frequent updates)
    
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
    
    memset(x, 0, n * sizeof(double));
    
    threads = new pthread_t[p];
    args = new Args[p];
    
    startComputation();
    
    updateInfoPanel();
}

MainWindow::~MainWindow() {
    terminating = true;
    dataMutex.lock();
    dataReady.wakeAll();
    dataMutex.unlock();
    
    for (int i = 1; i < p; ++i) {
        pthread_join(threads[i], nullptr);
    }
    
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
    // Update the info panel to show current status
    updateInfoPanel();
    
    // Check if computation has completed
    if (running && args[0].completed) {
        // Join all threads
        pthread_join(main_thread, nullptr);
        
        for (int i = 1; i < p; ++i) {
            pthread_join(threads[i], nullptr);
        }
        
        running = false;
        
        int its = args[0].its;
        double r1 = args[0].res_1;
        double r2 = args[0].res_2;
        double r3 = args[0].res_3;
        double r4 = args[0].res_4;
        double t1 = args[0].t1;
        double t2 = args[0].t2;

        const int task = 6;

        printf(
            "gui_app : Task = %d R1 = %e R2 = %e R3 = %e R4 = %e T1 = %.2f T2 = %.2f\n"
            "      It = %d E = %e K = %d Nx = %d Ny = %d P = %d\n",
            task, 
            r1, r2, r3, r4, 
            t1, t2, 
            its, eps, k, 
            nx, ny, p);
        
        renderer->setData(x, nx + 1, ny + 1);
        updateInfoPanel(); // Update again after completion
    }
    
    if (!running) {
        renderer->setData(x, nx + 1, ny + 1);
        renderer->update();
    }
}

void MainWindow::startComputation() {
    running = true;
    // Update immediately to show "Computing..." status
    updateInfoPanel();
    
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
        args[i].completed = false;
    }
    
    for (int i = 1; i < p; ++i) {
        pthread_create(&threads[i], nullptr, &gui_solution, &args[i]);
    }
    
    // Start computation without waiting
    pthread_create(&main_thread, nullptr, &gui_solution, &args[0]);
}

void MainWindow::toggleFunction() {
    k = (k + 1) % 8;
    func.select_f(k);
    renderer->setFunction(func.f);
    
    startComputation();
}

void MainWindow::toggleRenderMode() {
    // If computation is running, don't allow switching modes
    if (running) {
        QMessageBox::information(this, "Information", "Please wait until computation is completed.");
        return;
    }
    
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
    
    // Force update if switching to residual mode
    if (paint_mode == what_to_paint::residual) {
        // This causes the residual to be recalculated
        renderer->setData(x, nx + 1, ny + 1);
    }
    
    renderer->update();
    updateInfoPanel();
}

void MainWindow::zoomIn() {
    if (running) {
        QMessageBox::information(this, "Information", "Please wait until computation is completed.");
        return;
    }
    
    zoom_factor *= 2.0;
    renderer->setZoom(zoom_factor);
    updateInfoPanel();
}

void MainWindow::zoomOut() {
    if (running) {
        QMessageBox::information(this, "Information", "Please wait until computation is completed.");
        return;
    }
    
    zoom_factor = 1.0;
    renderer->setZoom(zoom_factor);
    updateInfoPanel();
}

void MainWindow::increaseGridDimension() {
    nx *= 2;
    ny *= 2;
    
    int n = (nx + 1) * (ny + 1);
    
    delete[] I;
    delete[] A;
    delete[] B;
    delete[] x;
    delete[] r;
    delete[] u;
    delete[] v;
    
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
    
    memset(x, 0, n * sizeof(double));
    
    startComputation();
}

void MainWindow::increaseEpsilon() {
    eps *= 10.0;
    
    startComputation();
}

void MainWindow::decreaseEpsilon() {
    eps /= 10.0;
    
    startComputation();
}

void MainWindow::increaseVisualizationDetail() {
    if (running) {
        QMessageBox::information(this, "Information", "Please wait until computation is completed.");
        return;
    }
    
    mx *= 2;
    my *= 2;
    updateInfoPanel();
}

void MainWindow::decreaseVisualizationDetail() {
    if (running) {
        QMessageBox::information(this, "Information", "Please wait until computation is completed.");
        return;
    }
    
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
    
    // Add computation status
    if (running) {
        oss << "[Производятся вычисления] ";
    } else {
        oss << "[Готов к работе] ";
    }
    
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
    if (paint_mode == what_to_paint::residual) {
        oss << " | Max residual: " << renderer->getMaxValue();
    } else {
        oss << " | Max value: " << renderer->getMaxValue();
    }
    
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