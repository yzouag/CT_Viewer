#include "contrast_barchart_widget.h"
#include <QPainter>
#include <QDebug>

Contrast_Barchart_Widget::Contrast_Barchart_Widget(std::deque<int>* hist, int min_val, int max_val, QWidget* parent)
{
    this->hist = hist;
    this->min_val = min_val;
    this->max_val = max_val;

    // append and prepend 0 to histogram, for example the bar chart is (-3500, 3500)
    // but the histogram statistics range (-3100, 3100), so we need to append 4 zeros
    int lower_bound = floor(this->min_val / 500.0) * 5;
    int upper_bound = ceil(this->max_val / 500.0) * 5;
    this->left_paddings = floor(min_val / 100.0) - lower_bound;
    this->right_paddings = upper_bound - ceil(max_val / 100.0);
    for (int i = 0; i < this->left_paddings; i++) {
        this->hist->push_front(0);
    }
    for (int i = 0; i < this->right_paddings; i++) {
        this->hist->push_back(0);
    }

    // update the size of the histogram
    this->n_bins = hist->size();
}


Contrast_Barchart_Widget::~Contrast_Barchart_Widget()
{
}

// add hover points to the vector
void Contrast_Barchart_Widget::initPoints(int height, int width)
{
    int rect_width = (width - 2 * horizontal_margin - 2 * grid_margin) / (n_bins - 1);
    this->min_pos_x = horizontal_margin + grid_margin + left_paddings * rect_width;
    this->max_pos_x = width - horizontal_margin - grid_margin - right_paddings * rect_width;

    // add leftmost point, lower bound point, upper bound point, rightmost point 
    this->points.append(QPoint(horizontal_margin, height - vertical_margin - grid_margin));
    this->points.append(QPoint(this->min_pos_x, height - vertical_margin - grid_margin));
    this->points.append(QPoint(this->max_pos_x, vertical_margin + grid_margin));
    this->points.append(QPoint(width - horizontal_margin, vertical_margin + grid_margin));
}

void Contrast_Barchart_Widget::initPoints(int height, int width, int lower, int upper)
{
    int rect_width = (width - 2 * horizontal_margin - 2 * grid_margin) / (n_bins - 1);
    this->min_pos_x = horizontal_margin + grid_margin + left_paddings * rect_width;
    this->max_pos_x = width - horizontal_margin - grid_margin - right_paddings * rect_width;

    // add leftmost point, lower bound point, upper bound point, rightmost point 
    this->points.append(QPoint(horizontal_margin, height - vertical_margin - grid_margin));
    this->points.append(QPoint(valueToPoint(lower), height - vertical_margin - grid_margin));
    this->points.append(QPoint(valueToPoint(upper), vertical_margin + grid_margin));
    this->points.append(QPoint(width - horizontal_margin, vertical_margin + grid_margin));
}

void Contrast_Barchart_Widget::setLogScale(bool useLogScale)
{
    this->useLogScale = useLogScale;
    update();
}

void Contrast_Barchart_Widget::reset()
{
    this->points[1] = QPoint(this->min_pos_x, this->points[1].y());
    this->points[2] = QPoint(this->max_pos_x, this->points[2].y());
    emit thresholdUpdated(min_val, max_val);
    update();
}

void Contrast_Barchart_Widget::mousePressEvent(QMouseEvent* e)
{
    this->activePoint = -1;
    for (int i = 1; i < points.size(); i++) {
        qreal distance = QLineF(e->pos(), points.at(i)).length();
        if (distance < 2 * point_size) {
            this->activePoint = i;
        }
    }
    if (this->activePoint != -1) {
        update();
    }
}

void Contrast_Barchart_Widget::mouseMoveEvent(QMouseEvent * e)
{
    if (this->activePoint != -1) {
        if (e->pos().x() < this->points.at(this->activePoint+1).x() && e->pos().x() > this->points.at(this->activePoint - 1).x()
            && e->pos().x() >= this->min_pos_x && e->pos().x() <= this->max_pos_x ) 
        {
            this->points[activePoint] = QPoint(e->pos().x(), this->points[activePoint].y());
            
            // transform the point position to intensity value and send the signal to ct_contrast_widget
            int lower = pointToValue(this->points.at(1).x());
            int upper = pointToValue(this->points.at(2).x());
            emit thresholdUpdated(lower, upper);
        }
    }
    update();
}

void Contrast_Barchart_Widget::mouseReleaseEvent(QMouseEvent * e)
{
    this->activePoint = -1;
    update();
}

void Contrast_Barchart_Widget::updatePoints(int lower, int upper)
{
    this->points[1] = QPoint(valueToPoint(lower), this->points[1].y());
    this->points[2] = QPoint(valueToPoint(upper), this->points[2].y());
    update();
}

void Contrast_Barchart_Widget::paintEvent(QPaintEvent * event)
{
    // create the painter
    QPainter p(this);
    drawHistogram(&p);
    drawPoints(&p);
}

void Contrast_Barchart_Widget::drawHistogram(QPainter* p)
{
    // clear the widget
    p->setBrush(QBrush(QColor(255, 255, 255)));
    p->drawRect(-1, -1, this->width() + 1, this->height() + 1);

    // add the axis
    p->setPen(QPen(QColor(0, 0, 0), 2));
    p->drawLine(horizontal_margin, vertical_margin, horizontal_margin, this->height() - vertical_margin);
    p->drawLine(horizontal_margin, this->height() - vertical_margin, this->width() - horizontal_margin, this->height() - vertical_margin);

    // draw grids
    p->setPen(QPen(QColor(240, 240, 240, 180), 2));
    p->drawLine(horizontal_margin, grid_margin + vertical_margin, this->width() - horizontal_margin, grid_margin + vertical_margin);
    p->drawLine(horizontal_margin, this->height() / 2, this->width() - horizontal_margin, this->height() / 2);
    p->drawLine(horizontal_margin, this->height() - vertical_margin - grid_margin, this->width() - horizontal_margin, this->height() - grid_margin - vertical_margin);

    int lower_bound = floor(min_val / 500.0);
    int upper_bound = ceil(max_val / 500.0);
    int grid_horizontal_distance = (this->width() - 2 * horizontal_margin - 2 * grid_margin) / (upper_bound - lower_bound);
    for (int i = 0; i < upper_bound - lower_bound + 1; i++) {
        p->drawLine(horizontal_margin + grid_margin + i * grid_horizontal_distance, vertical_margin, horizontal_margin + grid_margin + i * grid_horizontal_distance, this->height() - vertical_margin);
    }

    // draw axis label
    p->setPen(QPen(QColor(0, 0, 0), 2));
    p->drawText(horizontal_margin - 10, vertical_margin + grid_margin, tr("1"));
    p->drawText(horizontal_margin - 20, this->height() / 2, tr("0.5"));
    p->drawText(horizontal_margin - 10, this->height() - vertical_margin - grid_margin, tr("0"));

    for (int i = 0; i < upper_bound - lower_bound + 1; i++) {
        QString text = QString::fromStdString(std::to_string((lower_bound + i) * 500));
        QRect textBox = QRect(horizontal_margin + grid_margin - 15 + i * grid_horizontal_distance, this->height() - vertical_margin + 5, 30, 10);
        p->drawText(textBox, Qt::AlignCenter, text);
    }

    // draw gradient bar
    QLinearGradient gradient(horizontal_margin + grid_margin, this->height() - vertical_margin, this->width() - horizontal_margin, this->height() - vertical_margin);
    double startPoint = (this->points.at(1).x() - horizontal_margin - grid_margin) / (double)(this->width() - 2 * horizontal_margin -grid_margin);
    double endPoint = (this->points.at(2).x() - horizontal_margin - grid_margin) / (double)(this->width() - 2 * horizontal_margin - grid_margin);
    gradient.setColorAt(0, Qt::black);
    gradient.setColorAt(startPoint, Qt::black);
    gradient.setColorAt(endPoint, Qt::white);
    gradient.setColorAt(1, Qt::white);
    p->setPen(QColor(255, 255, 255, 0));
    p->setBrush(QBrush(gradient));
    QRect gradient_rect(horizontal_margin + grid_margin, this->height() - vertical_margin - grid_margin, this->width() - horizontal_margin - grid_margin, grid_margin);
    p->drawRect(gradient_rect);

    // draw text
    p->setPen(QPen(QColor(0, 0, 0), 4));
    p->setFont(QFont("Times", 10));
    QRect textBox = QRect(this->width() / 2 - text_width / 2, this->height() - vertical_margin + text_margin, text_width, text_height);
    p->drawText(textBox, Qt::AlignCenter, tr("Image Intensity"));

    // this is y axis title
    p->rotate(-90);
    p->drawText(QPoint(-this->height() / 2 - text_width / 3, text_margin * 2), tr("Index to Color Map"));
    p->rotate(90);

    // draw histogram
    p->setPen(QPen(QColor(0, 0, 0), 1));
    p->setBrush(QBrush(QColor(25, 181, 254), Qt::SolidPattern));
    int rect_width = (this->width() - 2 * horizontal_margin - 2 * grid_margin) / (n_bins - 1);

    for (int i = 0; i < this->hist->size(); i++) {
        int height = 0;
        if (this->useLogScale) {
            height = (this->hist->at(i) == 0) ? 0 : std::log(this->hist->at(i)) * 10;
        }
        else {
            height = this->hist->at(i) * cut_off;
        }
        QRect rect(horizontal_margin + grid_margin - rect_width / 2 + i * rect_width, this->height() - vertical_margin - grid_margin - height, rect_width, height);
        p->drawRect(rect);
    }

    // cover the upper margin so that the bar will not touch the ceil
    p->setPen(QPen(QColor(255, 255, 255, 0), 1));
    p->setBrush(QBrush(QColor(255, 255, 255), Qt::SolidPattern));
    QRect rect(0, 0, this->width(), vertical_margin);
    p->drawRect(rect);
}

void Contrast_Barchart_Widget::drawPoints(QPainter* painter)
{
    painter->setPen(QPen(QColor(0, 0, 0),2));
    painter->setBrush(Qt::red);
    for (int i = 1; i < points.size()-1; ++i) {
        QPoint pos = points.at(i);
        painter->drawEllipse(QRectF(pos.x() - point_size,pos.y() - point_size, point_size * 2, point_size * 2));
    }
    if (this->activePoint != -1) {
        painter->setBrush(Qt::yellow);
        QPoint pos = points.at(this->activePoint);
        painter->drawEllipse(QRectF(pos.x() - point_size, pos.y() - point_size, point_size * 2, point_size * 2));
    }
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(Qt::red, 0, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawPolyline(points);
}

int Contrast_Barchart_Widget::pointToValue(int pos_x)
{
    return (int)((pos_x - this->min_pos_x) / (double)((this->max_pos_x - this->min_pos_x)) * (this->max_val - this->min_val)) + min_val;
}

int Contrast_Barchart_Widget::valueToPoint(int val)
{
    return (int)((val - this->min_val)/(double)((this->max_val - this->min_val)) * (this->max_pos_x - this->min_pos_x)) + this->min_pos_x;
}