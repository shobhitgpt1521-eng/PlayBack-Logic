#pragma once
#include <QObject>
#include <QProcess>
#include "SegmentMeta.h"
#include <QDebug>
class FfmpegExporter : public QObject {
    Q_OBJECT
public:
    explicit FfmpegExporter(QObject* parent=nullptr);

    // start_ns/end_ns in nanoseconds
    void exportRange(const QVector<SegmentMeta>& metas,
                     qint64 start_ns, qint64 end_ns,
                     const QString& outPath,
                     bool frameAccurate=false);

signals:
    void progress(float ratio);                          // 0..1
    void finished(bool ok, const QString& path, const QString& err);

private:
    QString makeListFile(const QVector<SegmentMeta>& metas,
                         qint64 start_ns, qint64 end_ns,
                         qint64* base_ns_out);
    void runConcat(const QString& listPath, const QString& concatPath);
    void runTrim(const QString& concatPath, const QString& outPath,
                 double ssSec, double toSec, bool frameAccurate);
    void hookProgress(QProcess* p, double totalSec);

    QProcess *proc1=nullptr, *proc2=nullptr;
};
