#pragma once
#include <QString>
#include <QVector>

#include "SegmentMeta.h"

class TimelineBuilder {
public:
    // Build timeline from file paths, return per-segment metadata.
    static QVector<SegmentMeta> build(const QVector<QString>& paths, qint64* total_ns_out);
};
