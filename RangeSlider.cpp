#include "RangeSlider.h"
#include <QPainter>
#include <QMouseEvent>
#include <QStyle>

RangeSlider::RangeSlider(QWidget* parent): QWidget(parent) { setMouseTracking(true); }

void RangeSlider::setRange(int minSec, int maxSec){ minS=minSec; maxS=maxSec; a=minS; b=maxS; update(); }
void RangeSlider::setValues(int as, int bs){ a=std::max(minS,std::min(as,maxS)); b=std::max(a,std::min(bs,maxS)); update(); }

static QRect grooveRect(QWidget* w){
    QStyleOptionSlider opt;
    opt.initFrom(w);
    opt.orientation = Qt::Horizontal;     // assuming horizontal
    QRect r = w->style()->subControlRect(QStyle::CC_Slider, &opt,
                                         QStyle::SC_SliderGroove, w);
    if (!r.isValid())
        r = w->rect().adjusted(12, w->height()/2 - 4,
                               -12, -(w->height()/2 - 4));
    return r;
}


int RangeSlider::secToPx(int s, const QRect& g) const {
    if (maxS==minS) return g.left();
    double t=double(s-minS)/double(maxS-minS);
    return g.left()+int(t*g.width());
}
int RangeSlider::pxToSec(int x, const QRect& g) const {
    if (g.width()<=0) return minS;
    double t=double(x-g.left())/double(g.width());
    int s=minS+int(t*(maxS-minS));
    if (s < minS) {
        s = minS;
    }
    if (s > maxS) {
        s = maxS;
    }
    return s;

}

void RangeSlider::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing,false);
    QRect g = grooveRect(this);

    // groove
    p.setPen(Qt::NoPen); p.setBrush(QColor(60,60,60)); p.drawRect(g);

    // selected region
    QRect sel(secToPx(a,g), g.top(), secToPx(b,g)-secToPx(a,g), g.height());
    p.setBrush(QColor(30,144,255,120)); p.drawRect(sel);

    // handles
    p.setBrush(Qt::white);
    p.drawRect(QRect(secToPx(a,g)-2, g.top()-4, 4, g.height()+8));
    p.drawRect(QRect(secToPx(b,g)-2, g.top()-4, 4, g.height()+8));
}

void RangeSlider::mousePressEvent(QMouseEvent* ev){
    QRect g=grooveRect(this);
    int x=ev->pos().x(), ax=secToPx(a,g), bx=secToPx(b,g);
    drag = (std::abs(x-ax) < std::abs(x-bx)) ? Left : Right;
    mouseMoveEvent(ev);
}
void RangeSlider::mouseMoveEvent(QMouseEvent* ev){
    if (drag==None) return;
    QRect g=grooveRect(this);
    int s=pxToSec(ev->pos().x(), g);
    if (drag==Left){ a=std::min(s,b); }
    else            { b=std::max(s,a); }
    update(); emit valuesChanged(a,b);
}
void RangeSlider::mouseReleaseEvent(QMouseEvent*){ drag=None; }
