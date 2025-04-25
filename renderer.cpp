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
      visualizationWidth(100),   // Значение по умолчанию
      visualizationHeight(100),  // Значение по умолчанию
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
    
    // Update maxValue based on the current mode
    if (mode == what_to_paint::residual && func != nullptr) {
        // Recalculate residual immediately
        double hx = (b - a) / (dataWidth - 1);
        double hy = (d - c) / (dataHeight - 1);
        double maxResidual = 0.0;
        
        for (int i = 0; i < dataWidth - 1; i++) {
            for (int j = 0; j < dataHeight - 1; j++) {
                // Get node values
                double node1 = data[j * dataWidth + i];
                double node2 = data[j * dataWidth + i + 1];
                double node3 = data[(j + 1) * dataWidth + i + 1];
                double node4 = data[(j + 1) * dataWidth + i];
                
                // Lower triangle point
                double x_low = a + hx * (i + 2.0/3.0);
                double y_low = c + hy * (j + 1.0/3.0);
                double exact_low = func(x_low, y_low);
                double approx_low = (node1 + node2 + node3) / 3.0;
                double residual_low = std::fabs(exact_low - approx_low);
                
                // Upper triangle point
                double x_up = a + hx * (i + 1.0/3.0);
                double y_up = c + hy * (j + 2.0/3.0);
                double exact_up = func(x_up, y_up);
                double approx_up = (node1 + node3 + node4) / 3.0;
                double residual_up = std::fabs(exact_up - approx_up);
                
                maxResidual = std::max(maxResidual, std::max(residual_low, residual_up));
            }
        }
        
        maxValue = maxResidual;
    } else {
        calculateMaxValue();
    }
    
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
    
    // For non-residual modes, recalculate the maxValue normally
    if (mode != what_to_paint::residual && data && dataWidth > 0 && dataHeight > 0) {
        calculateMaxValue();
    }
    
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
    // Standard gradient (синий - голубой - зеленый - желтый - красный)
    standardGradient.setStart(0, 0);
    standardGradient.setFinalStop(1, 0);
    standardGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    standardGradient.setColorAt(0.0, QColor("#0000FF")); // Синий
    standardGradient.setColorAt(0.25, QColor("#00AAFF")); // Голубой
    standardGradient.setColorAt(0.5, QColor("#00FF00")); // Зеленый
    standardGradient.setColorAt(0.75, QColor("#FFFF00")); // Желтый
    standardGradient.setColorAt(1.0, QColor("#FF0000")); // Красный
    
    // Residual gradient (зеленый - желтый - оранжевый - красный)
    residualGradient.setStart(0, 0);
    residualGradient.setFinalStop(1, 0);
    residualGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    residualGradient.setColorAt(0.0, QColor("#00AA00")); // Зеленый (минимальная погрешность)
    residualGradient.setColorAt(0.3, QColor("#AAFF00")); // Светло-зеленый
    residualGradient.setColorAt(0.6, QColor("#FFAA00")); // Оранжевый
    residualGradient.setColorAt(1.0, QColor("#FF0000")); // Красный (максимальная погрешность)
    
    // Approximation gradient (сохраняем, но не используем)
    approximationGradient.setStart(0, 0);
    approximationGradient.setFinalStop(1, 0);
    approximationGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    approximationGradient.setColorAt(0.0, QColor("#00FFFF")); // Голубой
    approximationGradient.setColorAt(1.0, QColor("#FFA500")); // Оранжевый
}

QColor Renderer::getColor(double value, double min, double max) {
    double d;
    // Если диапазон значений очень мал (константная функция), используем искусственный разброс
    if (std::fabs(max-min) < 1e-6) {
        // Для константных функций создаем искусственный градиент
        // Проверка: если значение близко к 1, это вероятно функция f(x,y) = 1
        bool isConstantOne = (std::fabs(value - 1.0) < 1e-6);
        
        if (mode == what_to_paint::function) {
            if (isConstantOne) {
                // Для f(x,y) = 1 используем фиксированный средний цвет (зеленый)
                min = 0.0;
                max = 1.0;
                value = 0.5; // Зеленый (средний цвет в градиенте)
            } else {
                // Для других константных функций используем полный градиент
                min = 0.0;
                max = 1.0;
                value = 0.5; // Середина градиента
            }
        } else if (mode == what_to_paint::approximation) {
            if (isConstantOne) {
                // Для аппроксимации f(x,y) = 1 используем тот же зеленый
                min = 0.0;
                max = 1.0;
                value = 0.5; // Зеленый (средний цвет в градиенте)
            } else {
                min = 0.0;
                max = 1.0;
                value = 0.5; // Середина градиента
            }
        } else {
            // Для погрешности (residual) - нулевая погрешность, используем начало градиента
            min = 0.0;
            max = 1.0;
            value = 0.0; // Начало градиента (зеленый для минимальной погрешности)
        }
        d = 1.0;
    } else {
        d = max - min;
    }
    
    double normalizedValue = (value - min) / d;
    normalizedValue = qBound(0.0, normalizedValue, 1.0);
    
    // Choose appropriate gradient based on mode
    QLinearGradient *gradient;
    switch (mode) {
        case what_to_paint::function:
        case what_to_paint::approximation:  // Используем один и тот же градиент для функции и аппроксимации
            gradient = &standardGradient;
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
    
    // Используем параметры детализации визуализации, как и в drawFunction()
    const int nx = visualizationWidth;
    const int ny = visualizationHeight;
    
    // Создаем массив для интерполированных данных
    std::vector<std::vector<double>> values(nx, std::vector<double>(ny));
    
    // Проверяем, являются ли данные константными
    double firstValue = data[0];
    bool isConstantData = true;
    
    for (int i = 0; i < dataWidth * dataHeight && isConstantData; i++) {
        if (std::fabs(data[i] - firstValue) > 1e-16) {
            isConstantData = false;
        }
    }
    
    // Интерполируем данные на визуализационную сетку
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            // Преобразуем индексы сетки визуализации в координаты области
            double x = visibleRect.left() + visibleRect.width() * i / (nx - 1);
            double y = visibleRect.top() + visibleRect.height() * j / (ny - 1);
            
            // Находим соответствующую ячейку в исходной сетке данных
            double relX = (x - a) / (b - a);
            double relY = (y - c) / (d - c);
            
            if (relX < 0) relX = 0;
            if (relX > 1) relX = 1;
            if (relY < 0) relY = 0;
            if (relY > 1) relY = 1;
            
            double dataX = relX * (dataWidth - 1);
            double dataY = relY * (dataHeight - 1);
            
            int dataX_floor = static_cast<int>(dataX);
            int dataY_floor = static_cast<int>(dataY);
            
            dataX_floor = std::min(dataX_floor, dataWidth - 2);
            dataY_floor = std::min(dataY_floor, dataHeight - 2);
            
            double fracX = dataX - dataX_floor;
            double fracY = dataY - dataY_floor;
            
            // Билинейная интерполяция
            double v00 = data[dataY_floor * dataWidth + dataX_floor];
            double v10 = data[dataY_floor * dataWidth + (dataX_floor + 1)];
            double v01 = data[(dataY_floor + 1) * dataWidth + dataX_floor];
            double v11 = data[(dataY_floor + 1) * dataWidth + (dataX_floor + 1)];
            
            double v0 = v00 * (1 - fracX) + v10 * fracX;
            double v1 = v01 * (1 - fracX) + v11 * fracX;
            
            values[i][j] = v0 * (1 - fracY) + v1 * fracY;
        }
    }
    
    // Находим min и max значения
    double maxVal = -INFINITY;
    double minVal = INFINITY;
    
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            maxVal = std::max(maxVal, values[i][j]);
            minVal = std::min(minVal, values[i][j]);
        }
    }
    
    // Если данные константны, делаем то же самое, что и в drawFunction()
    if (isConstantData) {
        bool isConstantOne = (std::fabs(firstValue - 1.0) < 1e-16);
        
        if (isConstantOne) {
            // Для data = 1 во всех точках используем однородный зеленый цвет
            // Но это должно соответствовать drawFunction(), поэтому не меняем данные
            for (int i = 0; i < nx; i++) {
                for (int j = 0; j < ny; j++) {
                    values[i][j] = 0.5; // Зеленый цвет в градиенте
                }
            }
            minVal = 0.0;
            maxVal = 1.0;
        } else {
            // Для других константных данных создаем искусственный градиент,
            // как в drawFunction()
            for (int i = 0; i < nx; i++) {
                for (int j = 0; j < ny; j++) {
                    double relX = static_cast<double>(i) / (nx - 1);
                    double relY = static_cast<double>(j) / (ny - 1);
                    
                    values[i][j] = relX * relY + (1 - relX) * (1 - relY);
                }
            }
            minVal = 0.0;
            maxVal = 1.0;
        }
    }
    
    // Обновление maxValue для отображения
    maxValue = maxVal;
    
    // Отрисовка ячеек, как в drawFunction()
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
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(polygon);
        }
    }
}

void Renderer::drawResidual(QPainter &painter) {
    if (!data || !func || dataWidth <= 0 || dataHeight <= 0) {
        return;
    }
    
    // Используем параметры детализации визуализации
    const int nx = visualizationWidth;
    const int ny = visualizationHeight;
    
    double hx = (b - a) / (dataWidth - 1);
    double hy = (d - c) / (dataHeight - 1);
    double maxResidual = 0.0;
    
    // Проверяем, является ли функция константной
    bool isConstantFunction = true;
    double firstFuncValue = func(a, c);
    
    // Проверка нескольких точек для определения константности функции
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            double x = a + (b - a) * i / 4.0;
            double y = c + (d - c) * j / 4.0;
            if (std::fabs(func(x, y) - firstFuncValue) > 1e-16) {
                isConstantFunction = false;
                break;
            }
        }
        if (!isConstantFunction) break;
    }
    
    // Сначала вычислим погрешность на исходной сетке данных
    std::vector<std::vector<double>> originalResidual;
    originalResidual.resize(dataWidth, std::vector<double>(dataHeight));
    
    // Проверяем, совпадают ли значения функции и аппроксимации
    bool zeroResidual = true;
    
    // Расчет максимальной погрешности по ТЗ на исходной сетке
    for (int i = 0; i < dataWidth - 1; i++) {
        for (int j = 0; j < dataHeight - 1; j++) {
            // Получение значений в узлах
            double node1 = data[j * dataWidth + i]; // (i,j)
            double node2 = data[j * dataWidth + i + 1]; // (i+1,j)
            double node3 = data[(j + 1) * dataWidth + i + 1]; // (i+1,j+1)
            double node4 = data[(j + 1) * dataWidth + i]; // (i,j+1)
            
            // Нижний треугольник - точка с координатами (i+2/3, j+1/3)
            double x_low = a + hx * (i + 2.0/3.0);
            double y_low = c + hy * (j + 1.0/3.0);
            double exact_low = func(x_low, y_low);
            
            // Линейная интерполяция для нижнего треугольника
            double approx_low = (node1 + node2 + node3) / 3.0;
            double residual_low = std::fabs(exact_low - approx_low);
            
            // Верхний треугольник - точка с координатами (i+1/3, j+2/3)
            double x_up = a + hx * (i + 1.0/3.0);
            double y_up = c + hy * (j + 2.0/3.0);
            double exact_up = func(x_up, y_up);
            
            // Линейная интерполяция для верхнего треугольника
            double approx_up = (node1 + node3 + node4) / 3.0;
            double residual_up = std::fabs(exact_up - approx_up);
            
            // Сохраняем значения погрешности
            originalResidual[i][j] = std::max(residual_low, residual_up);
            
            if (residual_low > 1e-16 || residual_up > 1e-16) {
                zeroResidual = false;
            }
            
            maxResidual = std::max(maxResidual, std::max(residual_low, residual_up));
        }
    }
    
    // Теперь создаём сетку погрешности с нужной детализацией
    std::vector<std::vector<double>> residualMatrix(nx, std::vector<double>(ny));
    
    // Интерполируем исходную погрешность на новую сетку
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            // Соответствующие координаты в области функции
            double x = a + (b - a) * i / (nx - 1);
            double y = c + (d - c) * j / (ny - 1);
            
            // Находим соответствующую ячейку в исходной сетке данных
            int dataX = static_cast<int>((x - a) / hx);
            int dataY = static_cast<int>((y - c) / hy);
            
            // Ограничиваем индексы
            dataX = std::min(std::max(dataX, 0), dataWidth - 2);
            dataY = std::min(std::max(dataY, 0), dataHeight - 2);
            
            // Для простоты берем значение погрешности из найденной ячейки оригинальной сетки
            residualMatrix[i][j] = originalResidual[dataX][dataY];
        }
    }
    
    // Если функция константная и погрешность нулевая или близка к нулю
    if ((isConstantFunction || zeroResidual) && maxResidual < 1e-16) {
        // Используем искусственный градиент для визуализации
        for (int i = 0; i < nx; i++) {
            for (int j = 0; j < ny; j++) {
                double relX = static_cast<double>(i) / (nx - 1);
                double relY = static_cast<double>(j) / (ny - 1);
                
                // Создаем псевдопогрешность, которая будет просто близка к нулю
                // Это позволит нам увидеть зеленый цвет (минимальная погрешность)
                residualMatrix[i][j] = (relX * relY) * 1e-17;
            }
        }
        maxResidual = 1e-17;
    }
    
    // Обновление maxValue с вычисленной максимальной погрешностью
    maxValue = maxResidual;
    
    // Проверяем, являются ли невязки константными
    double firstValue = residualMatrix[0][0];
    bool isConstantResidual = true;
    
    for (int i = 0; i < nx && isConstantResidual; i++) {
        for (int j = 0; j < ny && isConstantResidual; j++) {
            if (std::fabs(residualMatrix[i][j] - firstValue) > 1e-16) {
                isConstantResidual = false;
            }
        }
    }
    
    // Если невязки константны, проверяем, равны ли они 1
    if (isConstantResidual) {
        bool isConstantOne = (std::fabs(firstValue - 1.0) < 1e-16);
        
        if (isConstantOne) {
            // Для residual = 1 во всех точках используем однородный зеленый цвет
            for (int i = 0; i < nx; i++) {
                for (int j = 0; j < ny; j++) {
                    residualMatrix[i][j] = 0.5; // Используем значение 0.5 для получения зеленого цвета
                }
            }
        } else {
            // Для других константных невязок создаем искусственный градиент
            for (int i = 0; i < nx; i++) {
                for (int j = 0; j < ny; j++) {
                    // Используем координаты для создания псевдоградиента
                    double relX = static_cast<double>(i) / (nx - 1);
                    double relY = static_cast<double>(j) / (ny - 1);
                    
                    // Создаем интересный паттерн на основе координат
                    residualMatrix[i][j] = relX * relY + (1 - relX) * (1 - relY);
                }
            }
        }
    }
    
    // Отрисовка ячеек по сетке визуализации
    for (int i = 0; i < nx - 1; i++) {
        for (int j = 0; j < ny - 1; j++) {
            // Вычисляем координаты точек в математической области
            double x1 = a + (b - a) * i / (nx - 1);
            double y1 = c + (d - c) * j / (ny - 1);
            double x2 = a + (b - a) * (i + 1) / (nx - 1);
            double y2 = c + (d - c) * (j + 1) / (ny - 1);
            
            // Преобразуем в экранные координаты
            QPointF p1 = l2g(x1, y1);
            QPointF p2 = l2g(x2, y1);
            QPointF p3 = l2g(x2, y2);
            QPointF p4 = l2g(x1, y2);
            
            // Цвет для этой ячейки
            QColor color = getColor(residualMatrix[i][j], 0.0, maxResidual);
            
            // Рисуем квадрат
            QPolygonF quad;
            quad << p1 << p2 << p3 << p4;
            
            painter.setBrush(color);
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(quad);
        }
    }
}

void Renderer::drawFunction(QPainter &painter) {
    if (!func) {
        return;
    }
    
    // Используем параметры детализации визуализации
    const int nx = visualizationWidth;
    const int ny = visualizationHeight;
    std::vector<std::vector<double>> values(nx, std::vector<double>(ny));
    double maxVal = -INFINITY;
    double minVal = INFINITY;
    
    bool isConstantFunction = true; // Флаг для определения константной функции
    double firstValue = 0.0;
    
    // Вычисляем значения функции и проверяем, является ли она константной
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            double x = visibleRect.left() + visibleRect.width() * i / (nx - 1);
            double y = visibleRect.top() + visibleRect.height() * j / (ny - 1);
            
            values[i][j] = func(x, y);
            
            if (i == 0 && j == 0) {
                firstValue = values[i][j];
            } else if (std::fabs(values[i][j] - firstValue) > 1e-6) {
                isConstantFunction = false;
            }
            
            maxVal = std::max(maxVal, values[i][j]);
            minVal = std::min(minVal, values[i][j]);
        }
    }
    
    // Если функция константная, проверяем, является ли она f(x,y) = 1
    if (isConstantFunction) {
        bool isConstantOne = (std::fabs(firstValue - 1.0) < 1e-6);
        
        if (isConstantOne) {
            // Для f(x,y) = 1 используем однородный зеленый цвет
            for (int i = 0; i < nx; i++) {
                for (int j = 0; j < ny; j++) {
                    values[i][j] = 0.5; // Используем значение 0.5 для получения зеленого цвета
                }
            }
            minVal = 0.0;
            maxVal = 1.0;
        } else {
            // Для других константных функций создаем искусственный градиент
            for (int i = 0; i < nx; i++) {
                for (int j = 0; j < ny; j++) {
                    // Используем координаты для создания псевдоградиента
                    double relX = static_cast<double>(i) / (nx - 1);
                    double relY = static_cast<double>(j) / (ny - 1);
                    
                    // Создаем интересный паттерн на основе координат
                    values[i][j] = relX * relY + (1 - relX) * (1 - relY);
                }
            }
            minVal = 0.0;
            maxVal = 1.0;
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

void Renderer::setVisualizationDetail(int mx, int my) {
    visualizationWidth = mx;
    visualizationHeight = my;
    update(); // Перерисовка с новой детализацией
} 