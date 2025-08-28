#pragma once
#include <QWidget>
#include <QSlider>
#include <QStyleOptionSlider>
#include <QStyle>
#include <QPainter>
class RangeSlider : public QWidget {
    Q_OBJECT
public:
    explicit RangeSlider(QWidget* parent=nullptr);

    void setRange(int minSec, int maxSec);
    void setValues(int aSec, int bSec);   // a<=b
    int  minSec() const { return minS; }
    int  maxSec() const { return maxS; }
    int  aSec()   const { return a; }
    int  bSec()   const { return b; }

signals:
    void valuesChanged(int aSec, int bSec);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    QSize minimumSizeHint() const override { return {200, 36}; }

private:
    int minS=0, maxS=0, a=0, b=0;
    enum Drag { None, Left, Right } drag=None;
    int pxToSec(int x, const QRect& groove) const;
    int secToPx(int s, const QRect& groove) const;
};
