/*
 * gstwebrtcbin.h - Source for webrtcbin
 *
 * Copyright (C) 2013 Kurento
 *   Contact: Jose Antonio Santos Cadenas <santoscadenas@kurento.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_WEBRTC_BIN_H__
#define __GST_WEBRTC_BIN_H__

#include <gst/gst.h>
#include <nice/nice.h>

G_BEGIN_DECLS
#define GST_TYPE_WEBRTC_BIN \
  (gst_webrtc_bin_get_type())
#define GST_WEBRTC_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_WEBRTC_BIN, GstWebrtcBin))
#define GST_WEBRTC_BIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_WEBRTC_BIN, GstWebrtcBinClass))
#define GST_IS_WEBRTC_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_WEBRTC_BIN))
#define GST_IS_WEBRTC_BIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_WEBRTC_BIN))
#define GST_WEBRTC_BIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_WEBRTC_BIN, GstWebrtcBinClass))
typedef struct _GstWebrtcBin GstWebrtcBin;
typedef struct _GstWebrtcBinClass GstWebrtcBinClass;

struct _GstWebrtcBinClass
{
  GstBinClass parent_class;

  /* Local candidates gathered */
  void (*local_candidates_gathered) (GstWebrtcBin * self,
      GstStructure * candidates);

    gboolean (*set_remote_descriptor) (GstWebrtcBin * self,
      const GstStructure * candidates, const GstStructure * credentials);
};

struct _GstWebrtcBin
{
  GstBin parent;

  GstElement *nicesrc;
  GstElement *srtpunprotect;
  GstElement *nicesink;
  GstElement *srtpprotect;

  NiceAgent *agent;
  guint stream;
  gboolean agent_init;

  GMutex credentials_lock;
  GstStructure *local_credentials;
  GstStructure *remote_credentials;

  GstStructure *local_candidates;
  GstStructure *remote_candidates;

  GMainContext *context;
  GMainLoop *loop;
};

GType gst_webrtc_bin_get_type (void);

gboolean gst_webrtc_bin_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_WEBRTC_BIN_H__ */
