#pragma once
#include <QString>
#include <QVector>

struct SegmentFile {
    QString path;
};

class FileSelector {
public:
    // dir like "/tmp/cam0", pattern like "cam0_*.mkv"
    static QVector<SegmentFile> select(const QString& dir, const QString& pattern = "cam0_*.mkv");
};
