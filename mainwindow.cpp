#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

#include <gst/gst.h>  // for GST_SECOND

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // ---- dynamic UI ----
    auto *c = new QWidget; setCentralWidget(c);
    auto *v = new QVBoxLayout(c);

    auto *row = new QWidget; auto *h = new QHBoxLayout(row);
    btnBuild = new QPushButton("Build Timeline");
    btnPlay  = new QPushButton("Play");
    btnStop  = new QPushButton("Stop");
    h->addWidget(btnBuild); h->addWidget(btnPlay); h->addWidget(btnStop);

    videoArea = new QWidget; videoArea->setMinimumSize(960, 540);
    videoArea->setStyleSheet("background:#111;");
    videoArea->setAttribute(Qt::WA_NativeWindow);
    (void)videoArea->winId(); // ensure native handle exists

    slider = new QSlider(Qt::Horizontal); slider->setRange(0, 0);
    status = new QLabel("Idle");

    v->addWidget(row);
    v->addWidget(videoArea);
    v->addWidget(slider);
    v->addWidget(status);

    // ---- wire components ----
    // Give the video window handle to the stitcher (it forwards to player)
    stitcher.attachPlayer(&player);
    stitcher.setVideoWinId(videoArea->winId());

    // UI events
    connect(btnBuild, &QPushButton::clicked, this, &MainWindow::buildTimeline);
    connect(btnPlay,  &QPushButton::clicked, this, &MainWindow::startPlayback);
    connect(btnStop,  &QPushButton::clicked, this, &MainWindow::stopPlayback);
    connect(slider,   &QSlider::sliderMoved, this, &MainWindow::onSeekMoved);

    // Error/EOS messages from stitching layer
    connect(&stitcher, &StitchingPlayer::errorText, this, [this](const QString& e){
        status->setText(e);
    });
    connect(&stitcher, &StitchingPlayer::reachedEOS, this, [this](){
        status->setText("EOS");
    });
}

void MainWindow::buildTimeline() {
    // 1) select files
    auto files = FileSelector::select(dir, pattern);
    if (files.isEmpty()) { status->setText("No chunks found"); return; }

    // 2) paths -> timeline metadata
    QVector<QString> paths; paths.reserve(files.size());
    for (auto& f : files) paths.push_back(f.path);

    total_ns = 0;
    metas = TimelineBuilder::build(paths, &total_ns);

    // 3) load into stitcher
    stitcher.setTimeline(metas);

    // slider is on virtual timeline in seconds
    slider->setRange(0, int(total_ns / GST_SECOND));
    status->setText(QString("Segments=%1 Total=%2 sec")
                    .arg(metas.size()).arg(int(total_ns / GST_SECOND)));
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
