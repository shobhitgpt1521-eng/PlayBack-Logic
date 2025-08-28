#include "FfmpegExporter.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>
#include <algorithm>

//static inline double ns_to_sec(qint64 ns){ return ns / 1e9; }

FfmpegExporter::FfmpegExporter(QObject* parent): QObject(parent) {}

QString FfmpegExporter::makeListFile(const QVector<SegmentMeta>& metas,
                                     qint64 start_ns, qint64 end_ns,
                                     qint64* base_ns_out)
{
    QDir().mkpath("/tmp");
    const QString listPath = "/tmp/ffconcat_list.txt";
    QFile f(listPath);
    if (!f.open(QIODevice::WriteOnly|QIODevice::Truncate)) return {};
    QTextStream ts(&f);
    ts << "ffconcat version 1.0\n";

    bool firstWritten=false;
    qint64 base_ns=0;

    for (const auto &m : metas) {
        const qint64 segStart = m.offset_ns;
        const qint64 segEnd   = m.offset_ns + m.duration_ns;
        if (segEnd <= start_ns) continue;
        if (segStart >= end_ns) break;

        if (!firstWritten) { base_ns = segStart; firstWritten = true; }
        const QString abs = QFileInfo(m.path).absoluteFilePath();
        ts << "file '" << QString(abs).replace('\'',"\\'") << "'\n";
    }
    f.close();
    if (base_ns_out) *base_ns_out = base_ns;
    return listPath;
}


void FfmpegExporter::hookProgress(QProcess* p, double totalSec){
    connect(p, &QProcess::readyReadStandardError, this, [this,p,totalSec](){
        const QString s = QString::fromUtf8(p->readAllStandardError());
        static QRegularExpression re(R"(time=(\d+):(\d+):(\d+)\.(\d+))");
        QRegularExpressionMatch m; double last=-1;
        auto it = re.globalMatch(s);
        while (it.hasNext()){
            m = it.next();
            int h=m.captured(1).toInt(), mi=m.captured(2).toInt(), se=m.captured(3).toInt();
            int cs=m.captured(4).left(2).toInt();
            last = h*3600 + mi*60 + se + cs/100.0;
        }
        if (last>=0 && totalSec>0) emit progress(std::min(1.0, last/totalSec));
    });
}

void FfmpegExporter::runConcat(const QString& listPath, const QString& concatPath){
    if (proc1) { proc1->kill(); proc1->deleteLater(); }
    proc1 = new QProcess(this);
    proc1->setProgram("ffmpeg");
    proc1->setArguments({
        "-y",
        "-f","concat","-safe","0",
        "-i", listPath,
        "-c","copy",
        concatPath
    });
    proc1->start();
}

void FfmpegExporter::runTrim(const QString& concatPath, const QString& outPath,
                             double ssSec, double toSec, bool frameAccurate)
{
    if (proc2) { proc2->kill(); proc2->deleteLater(); }
    proc2 = new QProcess(this);
    proc2->setProcessChannelMode(QProcess::MergedChannels);

    const double durSec = std::max(0.001, toSec - ssSec);
    QStringList args; args << "-hide_banner" << "-y";

    if (!frameAccurate) {
        // Fast, keyframe-aligned
        args << "-ss" << QString::number(ssSec, 'f', 3)
             << "-i"  << concatPath
             << "-t"  << QString::number(durSec, 'f', 3)
             << "-c"  << "copy";
    } else {
        // Frame-accurate: place -ss after -i and re-encode
        args << "-i"  << concatPath
             << "-ss" << QString::number(ssSec, 'f', 3)
             << "-t"  << QString::number(durSec, 'f', 3)
             << "-c:v" << "libx264" << "-preset" << "veryfast" << "-crf" << "18"
             << "-c:a" << "copy"
             << "-fflags" << "+genpts";
    }
    args << outPath;

    hookProgress(proc2, std::max(0.1, toSec-ssSec));

    proc2->setProgram("ffmpeg");
    proc2->setArguments(args);
    connect(proc2, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this,outPath](int code,QProcess::ExitStatus st){
        emit finished(code==0 && st==QProcess::NormalExit, outPath,
                      code==0 ? QString() : "ffmpeg trim failed");
    });
    proc2->start();
}

void FfmpegExporter::exportRange(const QVector<SegmentMeta>& metas,
                                 qint64 start_ns, qint64 end_ns,
                                 const QString& outPath,
                                 bool frameAccurate)
{
    if (metas.isEmpty() || start_ns>=end_ns) {
        emit finished(false, outPath, "invalid range"); return;
    }

    qint64 base_ns = 0;
    const QString listPath   = makeListFile(metas, start_ns, end_ns, &base_ns);
    if (listPath.isEmpty()) { emit finished(false,outPath,"cannot write list"); return; }
    const QString concatPath = "/tmp/ffconcat_full.mkv";

    if (proc1) { proc1->kill(); proc1->deleteLater(); }
    proc1 = new QProcess(this);
    proc1->setProcessChannelMode(QProcess::MergedChannels);

    runConcat(listPath, concatPath);

    connect(proc1, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int code, QProcess::ExitStatus st){
        qDebug() << "[concat] finished code=" << code << "status=" << st;
        if (!(code==0 && st==QProcess::NormalExit)) {
            emit finished(false, outPath, "ffmpeg concat failed"); return;
        }
        // Seek relative to first included segment
        const double ss_rel = (start_ns - base_ns) / 1e9;
        const double dur    = (end_ns   - start_ns) / 1e9;
        runTrim(concatPath, outPath, ss_rel, ss_rel + dur, frameAccurate);
    });
}

