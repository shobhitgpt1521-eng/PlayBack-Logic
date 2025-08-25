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

    // keep the same API as before
    void setVideoWinId(quintptr wid);
    void setTimeline(const QVector<SegmentMeta>& metas);
    void play();
    void stop();
    bool seekGlobalNs(qint64 t_ns);
    qint64 totalNs() const { return total_ns; }

    // wire the external player
    void attachPlayer(VideoPlayer* p);

signals:
    void reachedEOS();
    void errorText(const QString&);

private slots:
    void onPlayerEos();

private:
    // external single-file player
    VideoPlayer* player = nullptr;
    quintptr     winHandle = 0;   // stored until player is attached

    // timeline
    QStringList      paths;      // plain file paths
    QVector<qint64>  offsets;    // cumulative start times
    QVector<qint64>  durations;  // per-file durations
    qint64           total_ns = 0;
    int              curIndex = 0;

    // helpers
    void startIndex(int idx);
    int  indexForNs(qint64 t_ns, qint64* inSegNs=nullptr) const;
};
