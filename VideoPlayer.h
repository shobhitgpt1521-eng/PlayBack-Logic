#pragma once
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <glib.h>
#include <QTimer>

typedef struct _GstElement GstElement;
typedef struct _GstBus     GstBus;
typedef struct _GstMessage GstMessage;

class QTimer;

class VideoPlayer : public QObject {
    Q_OBJECT
public:
    explicit VideoPlayer(QObject* parent=nullptr);
    ~VideoPlayer();

    void setWindowHandle(quintptr wid);
    bool open(const QString& path);
    void play();
    void pause();
    void stop();

    bool seekNs(qint64 t_ns);          // keep current speed
    bool setRate(double r);            // change playback speed
    double rate() const { return rate_; }

signals:
    void eos();
    void errorText(const QString&);
    void positionNs(qint64);
    void durationNs(qint64);

private:
    static gboolean bus_cb(GstBus*, GstMessage*, gpointer);
    void bindOverlay();
    void teardown();

    // pipeline
    GstElement* pipeline=nullptr;
    GstElement* filesrc=nullptr;
    GstElement* demux=nullptr;
    GstElement* parser=nullptr;
    GstElement* decoder=nullptr;
    GstElement* vconv=nullptr;
    GstElement* videosink=nullptr;

    quintptr winHandle=0;
    QTimer*  posTimer=nullptr;
    double   rate_=1.0;   // current playback rate
};
