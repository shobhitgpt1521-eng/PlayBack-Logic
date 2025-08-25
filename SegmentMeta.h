#pragma once
#include <QString>
#include <QtGlobal>

struct SegmentMeta {
    QString path;
    qint64  duration_ns = 0;
    qint64  offset_ns   = 0;
};
