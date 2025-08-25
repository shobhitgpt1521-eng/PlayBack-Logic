#pragma once
#include <QMainWindow>
#include <QVector>
#include <QString>

#include "FileSelector.h"
#include "TimelineBuilder.h"
#include "SegmentMeta.h"
#include "VideoPlayer.h"
#include "StitchingPlayer.h"

class QPushButton; class QSlider; class QLabel; class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);

private slots:
    void buildTimeline();
    void startPlayback();
    void stopPlayback();
    void onSeekMoved(int sec);

private:
    // dynamic UI
    QWidget*     videoArea = nullptr;
    QPushButton* btnBuild  = nullptr;
    QPushButton* btnPlay   = nullptr;
    QPushButton* btnStop   = nullptr;
    QSlider*     slider    = nullptr;
    QLabel*      status    = nullptr;

    // data
    QVector<SegmentMeta> metas;
    qint64               total_ns = 0;

    // components
    VideoPlayer     player;    // single-file player
    StitchingPlayer stitcher;  // stitching controller

    // config
    QString dir = "/tmp/cam0";
    QString pattern = "cam0_*.mkv";
};
