#include "TrimDialog.h"
#include "RangeSlider.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>

TrimDialog::TrimDialog(int totalSec, const QVector<int>&, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Trim Selection");
    auto *v=new QVBoxLayout(this);

    lbl = new QLabel("Start: 0s  End: 0s  Dur: 0s", this);
    slider = new RangeSlider(this);
    slider->setRange(0, totalSec);
    slider->setValues(0, totalSec);

    connect(slider, &RangeSlider::valuesChanged, this, [this](int s, int e){
        a=s; b=e; int d=(e> s)?(e-s):0;
        lbl->setText(QString("Start: %1s  End: %2s  Dur: %3s").arg(s).arg(e).arg(d));
    });

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);

    v->addWidget(lbl);
    v->addWidget(slider);
    v->addWidget(bb);
}
