#pragma once
#include <QDialog>
#include <QVector>

class QLabel;
class RangeSlider;

class TrimDialog : public QDialog {
    Q_OBJECT
public:
    explicit TrimDialog(int totalSec, const QVector<int>& segMarks, QWidget* parent=nullptr);

    int startSec() const { return a; }
    int endSec()   const { return b; }

private:
    RangeSlider* slider;
    QLabel*      lbl;
    int a=0,b=0;
};
