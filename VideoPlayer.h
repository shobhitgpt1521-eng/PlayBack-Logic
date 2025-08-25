#pragma once
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <glib.h>

typedef struct _GstElement GstElement;
typedef struct _GstBus     GstBus;
typedef struct _GstMessage GstMessage;

class VideoPlayer : public QObject {
    Q_OBJECT
public:
    explicit VideoPlayer(QObject* parent=nullptr);
    ~VideoPlayer();

    void setWindowHandle(quintptr wid);     // Qt native child window
    bool open(const QString& path);         // build pipeline for file and preroll
    void play();
    void pause();
    void stop();
    bool seekNs(qint64 t_ns);               // GST_FORMAT_TIME

signals:
    void eos();
    void errorText(const QString&);

private:
    // pipeline
    GstElement* pipeline   = nullptr;
    GstElement* filesrc    = nullptr;
    GstElement* demux      = nullptr;   // qtdemux
    GstElement* parser     = nullptr;   // h264parse
    GstElement* decoder    = nullptr;   // avdec_h264
    GstElement* vconv      = nullptr;   // videoconvert
    GstElement* videosink  = nullptr;   // glimagesink / ximagesink
    quintptr    winHandle  = 0;

    // helpers
    void bindOverlay();
    void teardown();

    // bus
    static gboolean bus_cb(GstBus*, GstMessage*, gpointer self);
};
