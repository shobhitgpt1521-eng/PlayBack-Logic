#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

#include <gst/gst.h>  // GST_SECOND

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // ---- dynamic UI ----
    auto *c = new QWidget;
    setCentralWidget(c);
    auto *v = new QVBoxLayout(c);

    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    btnBuild = new QPushButton("Build Timeline");
    btnPlay  = new QPushButton("Play");
    btnStop  = new QPushButton("Stop");
    btnTrim  = new QPushButton("Trim");
    btnFwd   = new QPushButton(">> +10s");
    speedBox = new QComboBox;
    speedBox->addItems({"0.5x","1x","2x","4x"});
    speedBox->setCurrentIndex(1);

    h->addWidget(btnFwd);
    h->addWidget(speedBox);
    h->addWidget(btnTrim);
    connect(btnTrim, &QPushButton::clicked, this, &MainWindow::openTrim);
    h->addWidget(btnBuild);
    h->addWidget(btnPlay);
    h->addWidget(btnStop);

    videoArea = new QWidget;
    videoArea->setMinimumSize(960, 540);
    videoArea->setStyleSheet("background:#111;");
    videoArea->setAttribute(Qt::WA_NativeWindow);
    (void)videoArea->winId(); // ensure native handle exists

    // Segment-aware slider
    slider = new SegmentSlider(Qt::Horizontal);
    slider->setRange(0, 0);

    status = new QLabel("Idle");

    v->addWidget(row);
    v->addWidget(videoArea);
    v->addWidget(slider);
    v->addWidget(status);

    // ---- wire components ----
    stitcher.attachPlayer(&player);
    stitcher.setVideoWinId(videoArea->winId());

    connect(btnBuild, &QPushButton::clicked, this, &MainWindow::buildTimeline);
    connect(btnPlay,  &QPushButton::clicked, this, &MainWindow::startPlayback);
    connect(btnStop,  &QPushButton::clicked, this, &MainWindow::stopPlayback);

    // seeking from UI
    connect(slider, &QSlider::sliderMoved, this, &MainWindow::onSeekMoved);

    // live progress from stitching engine → slider
    connect(&stitcher, &StitchingPlayer::globalPositionSec, this, [this](int s){
        if (!slider->isSliderDown()) slider->setValue(s);
    });

    // optional cue on segment switch
    connect(&stitcher, &StitchingPlayer::segmentChanged, this, [this](int idx){
        status->setText(QString("Playing segment %1/%2").arg(idx + 1).arg(metas.size()));
    });

    // errors / eos
    connect(&stitcher, &StitchingPlayer::errorText, this, [this](const QString& e){
        status->setText(e);
    });
    connect(&stitcher, &StitchingPlayer::reachedEOS, this, [this](){
        status->setText("EOS");
    });

    connect(&stitcher, &StitchingPlayer::segmentChanged, this, [this](int idx){
        slider->setCurrentSegment(idx);
        status->setText(QString("Playing segment %1/%2").arg(idx+1).arg(metas.size()));
    });

    // ---- exporter ----
    exporter = new FfmpegExporter(this);

    connect(btnFwd, &QPushButton::clicked, this, [this]{
        if (metas.isEmpty()) return;
        int sec = slider->value() + 10;
        if (sec > slider->maximum()) sec = slider->maximum();
        slider->setValue(sec);
        stitcher.seekGlobalNs(qint64(sec) * GST_SECOND);
    });
    connect(speedBox, &QComboBox::currentTextChanged, this, [this](const QString& t){
        // parse "Nx"
        double r = t.left(t.size()-1).toDouble();
        if (r <= 0.0) r = 1.0;
        stitcher.setRate(r);
    });
}

void MainWindow::buildTimeline() {
    // 1) select files
    auto files = FileSelector::select(dir, pattern);
    if (files.isEmpty()) { status->setText("No chunks found"); return; }

    // 2) paths -> timeline metadata
    QVector<QString> paths;
    paths.reserve(files.size());
    for (auto& f : files) paths.push_back(f.path);

    total_ns = 0;
    metas = TimelineBuilder::build(paths, &total_ns);

    // 3) load into stitcher
    stitcher.setTimeline(metas);

    // slider on virtual timeline in seconds
    const int total_sec = int(total_ns / GST_SECOND);
    slider->setRange(0, total_sec);

    // build boundary marks at each segment start and the end
    QVector<int> marks;
    marks.reserve(metas.size() + 1);
    for (const auto& m : metas) marks.push_back(int(m.offset_ns / GST_SECOND));
    marks.push_back(total_sec);
    slider->setBoundaries(marks);

    status->setText(QString("Segments=%1 Total=%2 sec")
                    .arg(metas.size()).arg(total_sec));
}

void MainWindow::startPlayback() {
    if (metas.isEmpty()) { status->setText("Build timeline first"); return; }
    stitcher.play();
    status->setText("Playing");
}

void MainWindow::stopPlayback() {
    stitcher.stop();
    status->setText("Stopped");
}

void MainWindow::onSeekMoved(int sec) {
    if (metas.isEmpty()) return;
    stitcher.seekGlobalNs(qint64(sec) * GST_SECOND);
}

void MainWindow::openTrim() {
    if (metas.isEmpty()) { status->setText("Build timeline first"); return; }
    const int total_sec = int(total_ns / GST_SECOND);

    QVector<int> marks;
    marks.reserve(metas.size()+1);
    for (const auto& m : metas) marks.push_back(int(m.offset_ns / GST_SECOND));
    marks.push_back(total_sec);

    TrimDialog dlg(total_sec, marks, this);
    if (dlg.exec() != QDialog::Accepted) return;

    const int a = dlg.startSec();
    const int b = dlg.endSec();
    const qint64 a_ns = qint64(a) * GST_SECOND;
    const qint64 b_ns = qint64(b) * GST_SECOND;

    const QString out = QDir::homePath() + "/trimmed.mkv";
    status->setText(QString("Exporting %1..%2 → %3").arg(a).arg(b).arg(out));

    if (!exporter) exporter = new FfmpegExporter(this);
    disconnect(exporter, nullptr, nullptr, nullptr);

    connect(exporter, &FfmpegExporter::progress, this, [this](float p){
        status->setText(QString("Exporting... %1%").arg(int(p*100)));
    });
    connect(exporter, &FfmpegExporter::finished, this,
            [this,out](bool ok, const QString&, const QString& err){
        status->setText(ok ? QString("Export done: %1").arg(out)
                           : QString("Export failed: %1").arg(err));
    });

    exporter->exportRange(metas, a_ns, b_ns, out, false);
}
