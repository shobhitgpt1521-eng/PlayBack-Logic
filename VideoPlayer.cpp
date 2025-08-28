#include "VideoPlayer.h"
#include <QDebug>
#include <QTimer>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

static inline GstElement* mk(const char* f) { return gst_element_factory_make(f, nullptr); }

VideoPlayer::VideoPlayer(QObject* parent) : QObject(parent) {
    gst_init(nullptr, nullptr);

    posTimer = new QTimer(this);
    connect(posTimer, &QTimer::timeout, this, [this]{
        if (!pipeline) return;
        gint64 pos = 0, dur = 0;
        if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos))
            emit positionNs(pos);
        if (gst_element_query_duration(pipeline, GST_FORMAT_TIME, &dur))
            emit durationNs(dur);
    });
    posTimer->start(200);
}

VideoPlayer::~VideoPlayer() { teardown(); }

void VideoPlayer::setWindowHandle(quintptr wid) {
    winHandle = wid;
    bindOverlay();
}

bool VideoPlayer::open(const QString& path) {
    teardown();

    pipeline  = gst_pipeline_new("poc-player");
    filesrc   = mk("filesrc");
    demux     = mk("qtdemux");
    parser    = mk("h264parse");
    decoder   = mk("avdec_h264");
    vconv     = mk("videoconvert");
    videosink = mk("glimagesink");
    if (!videosink) videosink = mk("ximagesink");
    if (!videosink) videosink = mk("autovideosink");

    if (!pipeline || !filesrc || !demux || !parser || !decoder || !vconv || !videosink) {
        emit errorText("Failed to create GStreamer elements");
        teardown();
        return false;
    }

    if (g_object_class_find_property(G_OBJECT_GET_CLASS(videosink), "force-aspect-ratio"))
        g_object_set(videosink, "force-aspect-ratio", TRUE, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), filesrc, demux, parser, decoder, vconv, videosink, nullptr);

    if (!gst_element_link_many(parser, decoder, vconv, videosink, nullptr)) {
        emit errorText("Link failed: parser→decoder→vconv→sink");
        teardown(); return false;
    }
    if (!gst_element_link(filesrc, demux)) {
        emit errorText("Link failed: filesrc→qtdemux");
        teardown(); return false;
    }

    g_signal_connect(demux, "pad-added",
        G_CALLBACK(+[] (GstElement*, GstPad* pad, gpointer user) {
            auto self = static_cast<VideoPlayer*>(user);
            GstCaps* caps = gst_pad_query_caps(pad, nullptr);
            bool isVideo = false;
            if (caps) {
                const gchar* name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
                if (name && g_str_has_prefix(name, "video/")) isVideo = true;
                gst_caps_unref(caps);
            }
            if (!isVideo) return;
            GstPad* sinkPad = gst_element_get_static_pad(self->parser, "sink");
            if (!gst_pad_is_linked(sinkPad)) gst_pad_link(pad, sinkPad);
            gst_object_unref(sinkPad);
        }), this);

    GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, &VideoPlayer::bus_cb, this);
    gst_object_unref(bus);

    g_object_set(filesrc, "location", path.toUtf8().constData(), nullptr);
    bindOverlay();

    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    gst_element_get_state(pipeline, nullptr, nullptr, 3 * GST_SECOND);

    // push initial duration after preroll
    gint64 dur = 0;
    if (gst_element_query_duration(pipeline, GST_FORMAT_TIME, &dur))
        emit durationNs(dur);

    return true;
}

void VideoPlayer::play()  { if (pipeline) gst_element_set_state(pipeline, GST_STATE_PLAYING); }
void VideoPlayer::pause() { if (pipeline) gst_element_set_state(pipeline, GST_STATE_PAUSED);  }

void VideoPlayer::stop()  {
    if (pipeline) gst_element_set_state(pipeline, GST_STATE_NULL);
}

bool VideoPlayer::seekNs(qint64 t_ns) {
    if (!pipeline) return false;
    gboolean ok = gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME,
        (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
        GST_SEEK_TYPE_SET, t_ns,
        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
    if (ok) play();
    return ok;
}

void VideoPlayer::bindOverlay() {
    if (videosink && winHandle && GST_IS_VIDEO_OVERLAY(videosink)) {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(videosink), (guintptr)winHandle);
        gst_video_overlay_expose(GST_VIDEO_OVERLAY(videosink));
    }
}

void VideoPlayer::teardown() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    filesrc = demux = parser = decoder = vconv = videosink = nullptr;
}

gboolean VideoPlayer::bus_cb(GstBus*, GstMessage* msg, gpointer s) {
    auto self = static_cast<VideoPlayer*>(s);
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
        GError *err=nullptr; gchar *dbg=nullptr;
        gst_message_parse_error(msg, &err, &dbg);
        if (self) self->errorText(QString::fromUtf8(err ? err->message : "GStreamer error"));
        if (dbg)  qWarning("GST DEBUG: %s", dbg);
        g_clear_error(&err); g_free(dbg);
        break;
    }
    case GST_MESSAGE_EOS:
        if (self) emit self->eos();
        break;
    default: break;
    }
    return TRUE;
}
