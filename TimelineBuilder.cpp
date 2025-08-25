#include "TimelineBuilder.h"
#include <QUrl>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <gst/pbutils/gstdiscoverer.h>
static qint64 discover_ns(const QString& path) {
    GstDiscoverer* disc = gst_discoverer_new(5 * GST_SECOND, nullptr);
    if (!disc) return 0;
    auto uri = QUrl::fromLocalFile(path).toString();
    GError* err = nullptr;
    GstDiscovererInfo* info =
        gst_discoverer_discover_uri(disc, uri.toUtf8().constData(), &err);
    qint64 ns = info ? gst_discoverer_info_get_duration(info) : 0;
    if (info) gst_discoverer_info_unref(info);
    if (err)  g_error_free(err);
    g_object_unref(disc);
    return ns;
}

QVector<SegmentMeta> TimelineBuilder::build(const QVector<QString>& paths,
                                            qint64* total_ns_out) {
    QVector<SegmentMeta> metas;
    metas.reserve(paths.size());
    qint64 acc = 0;
    for (const auto& p : paths) {
        qint64 dur = discover_ns(p);
        SegmentMeta sm;
        sm.path        = p;
        sm.duration_ns = dur;
        sm.offset_ns   = acc;
        metas.push_back(sm);   // avoid initializer-list to satisfy QVector + C++11
        acc += dur;
    }
    if (total_ns_out) *total_ns_out = acc;
    return metas;
}
