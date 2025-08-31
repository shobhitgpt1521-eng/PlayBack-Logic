#pragma once
#include <QMainWindow>
#include <QVector>
#include <QString>

#include "FileSelector.h"
#include "TimelineBuilder.h"
#include "SegmentMeta.h"
#include "VideoPlayer.h"
#include "StitchingPlayer.h"
#include "SegmentSlider.h"
#include "TrimDialog.h"
#include "FfmpegExporter.h"
#include <QDir>

class QPushButton; class QSlider; class QLabel; class QWidget; class QComboBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);

private slots:
    void buildTimeline();
    void startPlayback();
    void stopPlayback();
    void onSeekMoved(int sec);
    void openTrim();

private:
    // dynamic UI
    QWidget*     videoArea = nullptr;
    QPushButton* btnBuild  = nullptr;
    QPushButton* btnPlay   = nullptr;
    QPushButton* btnStop   = nullptr;

    QLabel*      status    = nullptr;
    SegmentSlider* slider = nullptr;
    // data
    QVector<SegmentMeta> metas;
    qint64               total_ns = 0;

    // components
    VideoPlayer     player;    // single-file player
    StitchingPlayer stitcher;  // stitching controller

    // config
    QString dir = "/tmp/cam0";
    QString pattern = "cam0_*.mkv";
    //trim
    QPushButton* btnTrim = nullptr;
    FfmpegExporter* exporter = nullptr;
    QPushButton* btnFwd=nullptr;     // +10s
    QComboBox*  speedBox=nullptr;    // 0.5x..4x
};
