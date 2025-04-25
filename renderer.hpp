#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <QWidget>
#include <QColor>
#include <QGradient>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include "function_types.h"

enum class what_to_paint;

class Renderer : public QWidget {
    Q_OBJECT

public:
    Renderer(QWidget *parent = nullptr);
    
    // Set data and parameters
    void setData(double *data, int width, int height);
    void setBoundaries(double a, double b, double c, double d);
    void setZoom(double factor, QPointF center = QPointF());
    void setRenderMode(what_to_paint mode);
    void setFunction(double (*f)(double, double));
    void setApproximation(double *approx, int width, int height);
    void setVisualizationDetail(int mx, int my);
    
    double getMaxValue() const;
    QPointF l2g(double x, double y) const;
    QPointF g2l(double x, double y) const;
    // QPointF g2l(double x, double y) const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // Data
    double *data;                // Data to visualize
    double *approximation;       // Approximation data
    int dataWidth, dataHeight;   // Dimensions of data
    int visualizationWidth;      // Visualization grid width
    int visualizationHeight;     // Visualization grid height
    double maxValue;             // Maximum value for scaling
    
    // Boundaries
    double a, b, c, d;           // Logical boundaries
    
    // Zoom
    double zoomFactor;           // Current zoom factor
    QPointF zoomCenter;          // Center of zoom
    QRectF visibleRect;          // Currently visible rectangle in logical coordinates
    
    // Visualization
    what_to_paint mode;          // Current visualization mode
    double (*func)(double, double); // Original function
    
    // Colors and gradients
    QLinearGradient standardGradient;    // Standard gradient (blue-green-red)
    QLinearGradient residualGradient;    // Residual gradient (green-purple)
    QLinearGradient approximationGradient; // Approximation gradient (cyan-orange)
    
    // Private methods
    void updateVisibleRect();
    void setupGradients();
    QColor getColor(double value, double min, double max);
    void drawGrid(QPainter &painter);
    void drawData(QPainter &painter);
    void drawResidual(QPainter &painter);
    void drawFunction(QPainter &painter);
    void calculateMaxValue();
};

#endif // RENDERER_HPP 