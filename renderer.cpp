#include "renderer.hpp"
#include "window.hpp"
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <vector>
#include <cmath>

Renderer::Renderer(QWidget *parent)
    : QWidget(parent),
      data(nullptr),
      approximation(nullptr),
      dataWidth(0),
      dataHeight(0),
      maxValue(1.0),
      a(-1.0),
      b(1.0),
      c(-1.0),
      d(1.0),
      zoomFactor(1.0),
      mode(what_to_paint::function),
      func(nullptr) {
    
    // Set background color
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#F0F0F0"));
    setPalette(pal);
    
    // Setup color gradients
    setupGradients();
    
    // Initialize visible rect
    updateVisibleRect();
}

void Renderer::setData(double *data, int width, int height) {
    this->data = data;
    this->dataWidth = width;
    this->dataHeight = height;
    calculateMaxValue();
    update();
}

void Renderer::setBoundaries(double a, double b, double c, double d) {
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;
    updateVisibleRect();
    update();
}

void Renderer::setZoom(double factor, QPointF center) {
    zoomFactor = factor;
    zoomCenter = center.isNull() ? QPointF(width() / 2, height() / 2) : center;
    updateVisibleRect();
    update();
}

void Renderer::setRenderMode(what_to_paint mode) {
    this->mode = mode;
    update();
}

void Renderer::setFunction(double (*f)(double, double)) {
    func = f;
    update();
}

void Renderer::setApproximation(double *approx, int width, int height) {
    approximation = approx;
    dataWidth = width;
    dataHeight = height;
    update();
}

double Renderer::getMaxValue() const {
    return maxValue;
}

QPointF Renderer::l2g(double x, double y) const {
    // Convert logical (mathematical) coordinates to graphics (screen) coordinates
    double widthRatio = width() / (visibleRect.width());
    double heightRatio = height() / (visibleRect.height());
    
    double screenX = (x - visibleRect.left()) * widthRatio;
    double screenY = height() - (y - visibleRect.top()) * heightRatio;
    
    return QPointF(screenX, screenY);
}

QPointF Renderer::g2l(double x, double y) const {
    // Convert graphics (screen) coordinates to logical (mathematical) coordinates
    double widthRatio = visibleRect.width() / width();
    double heightRatio = visibleRect.height() / height();
    
    double logicalX = visibleRect.left() + x * widthRatio;
    double logicalY = visibleRect.top() + (height() - y) * heightRatio;
    
    return QPointF(logicalX, logicalY);
}

void Renderer::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Fill background
    painter.fillRect(event->rect(), QColor("#F0F0F0"));
    
    // Draw grid but grid drawing is disabled
    drawGrid(painter);
    
    // Draw data based on mode
    switch (mode) {
        case what_to_paint::function:
            drawFunction(painter);
            break;
        case what_to_paint::approximation:
            drawData(painter);
            break;
        case what_to_paint::residual:
            drawResidual(painter);
            break;
    }
}

void Renderer::resizeEvent(QResizeEvent *) {
    updateVisibleRect();
}

void Renderer::updateVisibleRect() {
    // Calculate the visible rectangle in logical coordinates
    double centerX = (a + b) / 2.0;
    double centerY = (c + d) / 2.0;
    
    double width = (b - a) / zoomFactor;
    double height = (d - c) / zoomFactor;
    
    visibleRect = QRectF(
        centerX - width / 2.0,
        centerY - height / 2.0,
        width,
        height
    );
}

void Renderer::setupGradients() {
    // Standard gradient (blue-green-red)
    standardGradient.setStart(0, 0);
    standardGradient.setFinalStop(1, 0);
    standardGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    standardGradient.setColorAt(0.0, QColor("#0000FF")); // Blue
    standardGradient.setColorAt(0.5, QColor("#00FF00")); // Green
    standardGradient.setColorAt(1.0, QColor("#FF0000")); // Red
    
    // Residual gradient (green-purple)
    residualGradient.setStart(0, 0);
    residualGradient.setFinalStop(1, 0);
    residualGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    residualGradient.setColorAt(0.0, QColor("#00FF00")); // Green
    residualGradient.setColorAt(1.0, QColor("#8B00FF")); // Purple
    
    // Approximation gradient (cyan-orange)
    approximationGradient.setStart(0, 0);
    approximationGradient.setFinalStop(1, 0);
    approximationGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    approximationGradient.setColorAt(0.0, QColor("#00FFFF")); // Cyan
    approximationGradient.setColorAt(1.0, QColor("#FFA500")); // Orange
}

QColor Renderer::getColor(double value, double min, double max) {
    // Normalize value between 0 and 1
    double normalizedValue = (value - min) / (max - min);
    normalizedValue = qBound(0.0, normalizedValue, 1.0);
    
    // Choose appropriate gradient based on mode
    QLinearGradient *gradient;
    switch (mode) {
        case what_to_paint::function:
            gradient = &standardGradient;
            break;
        case what_to_paint::approximation:
            gradient = &approximationGradient;
            break;
        case what_to_paint::residual:
            gradient = &residualGradient;
            break;
        default:
            gradient = &standardGradient;
    }
    
    // Sample gradient at the normalized value position
    QGradientStops stops = gradient->stops();
    
    if (stops.size() < 2) {
        return QColor(0, 0, 0);
    }
    
    for (int i = 0; i < stops.size() - 1; i++) {
        double pos1 = stops[i].first;
        double pos2 = stops[i + 1].first;
        
        if (normalizedValue >= pos1 && normalizedValue <= pos2) {
            double t = (normalizedValue - pos1) / (pos2 - pos1);
            
            QColor color1 = stops[i].second;
            QColor color2 = stops[i + 1].second;
            
            int r = color1.red() + t * (color2.red() - color1.red());
            int g = color1.green() + t * (color2.green() - color1.green());
            int b = color1.blue() + t * (color2.blue() - color1.blue());
            
            return QColor(r, g, b);
        }
    }
    
    return QColor(0, 0, 0);
}

void Renderer::drawGrid(QPainter &painter) {
    // This method is intentionally left empty to disable grid drawing
    return; // Не рисуем сетку
    
    // Код ниже не будет выполняться
    
    // Set pen for grid lines
    QPen gridPen(QColor("#C0C0C0"));
    gridPen.setWidth(1);
    painter.setPen(gridPen);
    
    // Draw horizontal grid lines
    double stepY = (d - c) / 10.0;
    for (double y = c; y <= d; y += stepY) {
        QPointF start = l2g(a, y);
        QPointF end = l2g(b, y);
        painter.drawLine(start, end);
        
        // Draw y-axis labels
        QFont font = painter.font();
        font.setPointSize(8);
        painter.setFont(font);
        painter.setPen(QColor("#00008B"));
        QPointF labelPos = l2g(a, y);
        painter.drawText(QRectF(labelPos.x() - 40, labelPos.y() - 10, 35, 20), 
                         Qt::AlignRight | Qt::AlignVCenter, 
                         QString::number(y, 'g', 3));
    }
    
    // Draw vertical grid lines
    double stepX = (b - a) / 10.0;
    for (double x = a; x <= b; x += stepX) {
        QPointF start = l2g(x, c);
        QPointF end = l2g(x, d);
        painter.drawLine(start, end);
        
        // Draw x-axis labels
        QFont font = painter.font();
        font.setPointSize(8);
        painter.setFont(font);
        painter.setPen(QColor("#00008B"));
        QPointF labelPos = l2g(x, c);
        painter.drawText(QRectF(labelPos.x() - 20, labelPos.y() + 5, 40, 15), 
                         Qt::AlignCenter, 
                         QString::number(x, 'g', 3));
    }
    
    // Draw axes
    painter.setPen(QPen(QColor("#404040"), 2));
    // X-axis
    if (c <= 0 && d >= 0) {
        QPointF start = l2g(a, 0);
        QPointF end = l2g(b, 0);
        painter.drawLine(start, end);
    }
    // Y-axis
    if (a <= 0 && b >= 0) {
        QPointF start = l2g(0, c);
        QPointF end = l2g(0, d);
        painter.drawLine(start, end);
    }
}

void Renderer::drawData(QPainter &painter) {
    if (!data || dataWidth <= 0 || dataHeight <= 0) {
        return;
    }
    
    double minValue = 0.0;
    
    // Draw colored cells
    for (int i = 0; i < dataWidth - 1; i++) {
        for (int j = 0; j < dataHeight - 1; j++) {
            double val = data[j * dataWidth + i];
            QColor color = getColor(val, minValue, maxValue);
            
            QPointF p1 = l2g(a + (b - a) * i / (dataWidth - 1), c + (d - c) * j / (dataHeight - 1));
            QPointF p2 = l2g(a + (b - a) * (i + 1) / (dataWidth - 1), c + (d - c) * j / (dataHeight - 1));
            QPointF p3 = l2g(a + (b - a) * (i + 1) / (dataWidth - 1), c + (d - c) * (j + 1) / (dataHeight - 1));
            QPointF p4 = l2g(a + (b - a) * i / (dataWidth - 1), c + (d - c) * (j + 1) / (dataHeight - 1));
            
            QPolygonF polygon;
            polygon << p1 << p2 << p3 << p4;
            
            painter.setBrush(color);
            painter.setPen(Qt::NoPen); // Убираем контур, чтобы не было видно линий сетки
            painter.drawPolygon(polygon);
        }
    }
}

void Renderer::drawResidual(QPainter &painter) {
    if (!data || !func || dataWidth <= 0 || dataHeight <= 0) {
        return;
    }
    
    // Calculate maximum residual
    double maxResidual = 0.0;
    
    for (int i = 0; i < dataWidth; i++) {
        for (int j = 0; j < dataHeight; j++) {
            double x = a + (b - a) * i / (dataWidth - 1);
            double y = c + (d - c) * j / (dataHeight - 1);
            
            double exactVal = func(x, y);
            double approxVal = data[j * dataWidth + i];
            double residual = std::abs(exactVal - approxVal);
            
            maxResidual = std::max(maxResidual, residual);
        }
    }
    
    // Draw colored cells
    for (int i = 0; i < dataWidth - 1; i++) {
        for (int j = 0; j < dataHeight - 1; j++) {
            double x = a + (b - a) * i / (dataWidth - 1);
            double y = c + (d - c) * j / (dataHeight - 1);
            
            double exactVal = func(x, y);
            double approxVal = data[j * dataWidth + i];
            double residual = std::abs(exactVal - approxVal);
            
            QColor color = getColor(residual, 0.0, maxResidual);
            
            QPointF p1 = l2g(a + (b - a) * i / (dataWidth - 1), c + (d - c) * j / (dataHeight - 1));
            QPointF p2 = l2g(a + (b - a) * (i + 1) / (dataWidth - 1), c + (d - c) * j / (dataHeight - 1));
            QPointF p3 = l2g(a + (b - a) * (i + 1) / (dataWidth - 1), c + (d - c) * (j + 1) / (dataHeight - 1));
            QPointF p4 = l2g(a + (b - a) * i / (dataWidth - 1), c + (d - c) * (j + 1) / (dataHeight - 1));
            
            QPolygonF polygon;
            polygon << p1 << p2 << p3 << p4;
            
            painter.setBrush(color);
            painter.setPen(Qt::NoPen); // Убираем контур, чтобы не было видно линий сетки
            painter.drawPolygon(polygon);
        }
    }
}

void Renderer::drawFunction(QPainter &painter) {
    if (!func) {
        return;
    }
    
    // Calculate function values on a grid
    const int nx = 100;
    const int ny = 100;
    std::vector<std::vector<double>> values(nx, std::vector<double>(ny));
    double maxVal = -INFINITY;
    double minVal = INFINITY;
    
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            double x = visibleRect.left() + visibleRect.width() * i / (nx - 1);
            double y = visibleRect.top() + visibleRect.height() * j / (ny - 1);
            
            values[i][j] = func(x, y);
            maxVal = std::max(maxVal, values[i][j]);
            minVal = std::min(minVal, values[i][j]);
        }
    }
    
    // Draw colored cells
    for (int i = 0; i < nx - 1; i++) {
        for (int j = 0; j < ny - 1; j++) {
            double x1 = visibleRect.left() + visibleRect.width() * i / (nx - 1);
            double y1 = visibleRect.top() + visibleRect.height() * j / (ny - 1);
            double x2 = visibleRect.left() + visibleRect.width() * (i + 1) / (nx - 1);
            double y2 = visibleRect.top() + visibleRect.height() * (j + 1) / (ny - 1);
            
            QColor color = getColor(values[i][j], minVal, maxVal);
            
            QPointF p1 = l2g(x1, y1);
            QPointF p2 = l2g(x2, y1);
            QPointF p3 = l2g(x2, y2);
            QPointF p4 = l2g(x1, y2);
            
            QPolygonF polygon;
            polygon << p1 << p2 << p3 << p4;
            
            painter.setBrush(color);
            painter.setPen(Qt::NoPen); // Убираем контур, чтобы не было видно линий сетки
            painter.drawPolygon(polygon);
        }
    }
    
    // Update maxValue for info display
    maxValue = maxVal;
}

void Renderer::calculateMaxValue() {
    if (!data || dataWidth <= 0 || dataHeight <= 0) {
        maxValue = 1.0;
        return;
    }
    
    double max = -INFINITY;
    for (int i = 0; i < dataWidth * dataHeight; i++) {
        max = std::max(max, data[i]);
    }
    
    maxValue = max;
} 