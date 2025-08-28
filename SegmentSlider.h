#pragma once
#include <QSlider>
#include <QVector>
#include <QStyleOptionSlider>
#include <QStyle>
#include <QPainter>

class SegmentSlider : public QSlider {
    Q_OBJECT
public:
    explicit SegmentSlider(Qt::Orientation o, QWidget* parent=nullptr)
        : QSlider(o, parent) {}

    void setBoundaries(const QVector<int>& seconds) { // sorted, in seconds
        boundaries = seconds; update();
    }
    void setCurrentSegment(int idx) { curSeg = idx; update(); }

protected:
    void paintEvent(QPaintEvent* e) override {
        QSlider::paintEvent(e);
        if (boundaries.isEmpty() || maximum() <= minimum()) return;

        QStyleOptionSlider opt; initStyleOption(&opt);
        QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt,
                                               QStyle::SC_SliderGroove, this);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);

        // draw all ticks
        p.setPen(QPen(Qt::gray, 1, Qt::SolidLine));
        for (int i = 0; i < boundaries.size(); ++i) {
            const int s = boundaries[i];
            double t = double(s - minimum()) / double(maximum() - minimum());
            int x = groove.left() + int(t * groove.width());
            // highlight current segment boundaries
            if (i == curSeg || i == curSeg + 1) p.setPen(QPen(Qt::red, 2));
            p.drawLine(QPoint(x, groove.top()), QPoint(x, groove.bottom()));
            if (i == curSeg || i == curSeg + 1) p.setPen(QPen(Qt::gray, 1));
        }
    }

private:
    QVector<int> boundaries;
    int curSeg = -1;
};
