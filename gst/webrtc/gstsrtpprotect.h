/*
 * gstsrtpprotect.h - Source for webrtcbin
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

#ifndef __GST_SRTP_PROTECT_H__
#define __GST_SRTP_PROTECT_H__

#include <gst/gst.h>
#include <gstcryptopolicy.h>
#include <srtp/srtp.h>

G_BEGIN_DECLS
#define GST_TYPE_SRTP_PROTECT \
  (gst_srtp_protect_get_type())
#define GST_SRTP_PROTECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SRTP_PROTECT, GstSrtpProtect))
#define GST_SRTP_PROTECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SRTP_PROTECT, GstSrtpProtectClass))
#define GST_IS_SRTP_PROTECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SRTP_PROTECT))
#define GST_IS_SRTP_PROTECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SRTP_PROTECT))
#define GST_SRTP_PROTECT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_SRTP_PROTECT, GstSrtpProtectClass))
typedef struct _GstSrtpProtect GstSrtpProtect;
typedef struct _GstSrtpProtectClass GstSrtpProtectClass;

struct _GstSrtpProtectClass
{
  GstElementClass parent_class;
};

struct _GstSrtpProtect
{
  GstElement parent;

  GstCryptoPolicy policy;
  srtp_t srtp_session;
  GstPad *rtp_sink;
  GstPad *rtcp_sink;
  GstPad *src;
  guint8 *key;
  gchar *base64key;
};

GType gst_srtp_protect_get_type (void);

gboolean gst_srtp_protect_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* __GST_SRTP_PROTECT_H__ */
