#include "FileSelector.h"
#include <QDir>
#include <QFileInfoList>

QVector<SegmentFile> FileSelector::select(const QString& dir, const QString& pattern) {
    QVector<SegmentFile> out;
    QDir d(dir);
    auto files = d.entryInfoList(QStringList() << pattern, QDir::Files, QDir::Name);
    for (const auto& fi : files) out.push_back({ fi.absoluteFilePath() });
    return out;
}
