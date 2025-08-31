#pragma once
#include <QObject>
#include <QStringList>
#include <QVector>
#include <QtGlobal>
#include "SegmentMeta.h"

// fwd decl
class VideoPlayer;

class StitchingPlayer : public QObject {
    Q_OBJECT
public:
    explicit StitchingPlayer(QObject* parent=nullptr);

    void setVideoWinId(quintptr wid);
    void setTimeline(const QVector<SegmentMeta>& metas);
    void play();
    void stop();
    bool seekGlobalNs(qint64 t_ns);
    qint64 totalNs() const { return total_ns; }

    // new
    void setRate(double r);                 // implemented in .cpp
    void attachPlayer(VideoPlayer* p);

signals:
    void reachedEOS();
    void errorText(const QString&);
    void globalPositionSec(int sec);
    void segmentChanged(int index);

private slots:
    void onPlayerEos();

private:
    // external single-file player
    VideoPlayer* player = nullptr;
    quintptr     winHandle = 0;
    double       desiredRate_ = 1.0;

    // timeline
    QStringList      paths;
    QVector<qint64>  offsets;
    QVector<qint64>  durations;
    qint64           total_ns = 0;
    int              curIndex = 0;

    void startIndex(int idx);
    int  indexForNs(qint64 t_ns, qint64* inSegNs=nullptr) const;
};
