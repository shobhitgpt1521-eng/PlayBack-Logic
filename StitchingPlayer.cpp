#include "StitchingPlayer.h"
#include "VideoPlayer.h"
#include <QDebug>
#include <gst/gst.h>

StitchingPlayer::StitchingPlayer(QObject* parent) : QObject(parent) {}

void StitchingPlayer::attachPlayer(VideoPlayer* p) {
    player = p;
    if (!player) return;

    if (winHandle) player->setWindowHandle(winHandle);

    connect(player, SIGNAL(eos()), this, SLOT(onPlayerEos()));
    connect(player, SIGNAL(errorText(QString)), this, SIGNAL(errorText(QString)));

    connect(player, &VideoPlayer::positionNs, this, [this](gint64 inSegNs){
        if (offsets.isEmpty()) return;
        const qint64 base = (curIndex >= 0 && curIndex < offsets.size()) ? offsets[curIndex] : 0;
        const qint64 global = base + inSegNs;
        emit globalPositionSec(int(global / GST_SECOND));
    });

    // apply current rate to newly attached player
    player->setRate(desiredRate_);
}

void StitchingPlayer::setVideoWinId(quintptr wid) {
    winHandle = wid;
    if (player) player->setWindowHandle(wid);
}

void StitchingPlayer::setTimeline(const QVector<SegmentMeta>& metas) {
    paths.clear(); offsets.clear(); durations.clear(); total_ns = 0; curIndex = 0;
    paths.reserve(metas.size());
    offsets.reserve(metas.size());
    durations.reserve(metas.size());

    for (const auto& m : metas) {
        paths     << m.path;
        offsets   << m.offset_ns;
        durations << m.duration_ns;
        total_ns   = m.offset_ns + m.duration_ns;
    }
}

void StitchingPlayer::setRate(double r) {
    desiredRate_ = (r == 0.0 ? 1.0 : r);
    if (player) player->setRate(desiredRate_);
}

void StitchingPlayer::startIndex(int idx) {
    if (!player || paths.isEmpty()) return;
    if (idx < 0) idx = 0;
    if (idx > (int)paths.size()-1) idx = (int)paths.size()-1;
    curIndex = idx;

    emit segmentChanged(curIndex);

    if (!player->open(paths[curIndex])) {
        emit errorText(QString("Failed to open %1").arg(paths[curIndex]));
        return;
    }
    player->setRate(desiredRate_);
    qDebug() << "[Stitching] playing ->" << paths[curIndex];
    player->play();
}

void StitchingPlayer::play() { startIndex(0); }

void StitchingPlayer::stop() { if (player) player->stop(); }

int StitchingPlayer::indexForNs(qint64 t_ns, qint64* inSegNs) const {
    int n = offsets.size();
    if (n == 0) { if (inSegNs) *inSegNs = 0; return 0; }
    int lo=0, hi=n-1, ans=0;
    while (lo<=hi) {
        int mid=(lo+hi)/2;
        if (offsets[mid] <= t_ns) { ans=mid; lo=mid+1; } else hi=mid-1;
    }
    if (inSegNs) *inSegNs = t_ns - offsets[ans];
    return ans;
}

bool StitchingPlayer::seekGlobalNs(qint64 t_ns) {
    if (!player || paths.isEmpty()) return false;
    if (t_ns < 0) t_ns = 0;
    if (t_ns >= total_ns) t_ns = total_ns - 1;

    qint64 inSeg = 0;
    int idx = indexForNs(t_ns, &inSeg);

    if (idx != curIndex) startIndex(idx);
    return player->seekNs(inSeg);
}

void StitchingPlayer::onPlayerEos() {
    int next = curIndex + 1;
    if (next < paths.size()) startIndex(next);
    else emit reachedEOS();
}
