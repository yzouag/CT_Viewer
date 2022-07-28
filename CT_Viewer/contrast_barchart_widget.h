#pragma once
#include "qwidget.h"
#include <deque>
#include <QMouseEvent>
#include <QVector>

class Contrast_Barchart_Widget : public QWidget
{
    Q_OBJECT
public:
    Contrast_Barchart_Widget(std::deque<int>* hist, int min_val, int max_val, QWidget *parent = nullptr);
    ~Contrast_Barchart_Widget();
    void initPoints(int height, int width);
    void initPoints(int height, int width, int lower, int upper);
    void setLogScale(bool useLogScale);
    void reset();

public slots:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void updatePoints(int lower, int upper);

signals:
    void thresholdUpdated(int lower, int upper);

private:
    std::deque<int>* hist;
    int n_bins;
    int min_val;
    int max_val;
    int left_paddings;
    int right_paddings;

    // points cannot exceed these two bounds
    int min_pos_x;
    int max_pos_x;

    // settings for the bar chart
    int horizontal_margin = 50;
    int vertical_margin = 40;
    int grid_margin = 15;
    int text_margin = 10;
    int text_width = 200;
    int text_height = 30;
    double cut_off = 0.00015; // scale the histogram to fit the picture

    // points setting
    int point_size = 5;
    int activePoint = -1;
    QVector<QPoint> points;

    // toggle the log scale
    bool useLogScale = false;

    void paintEvent(QPaintEvent *event);
    void drawHistogram(QPainter* p);
    void drawPoints(QPainter* p);
    int pointToValue(int pos_x);
    int valueToPoint(int val);
};

