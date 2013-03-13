/*
 * gstsrtpunprotect.c - Source for webrtcbin
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

#include <config.h>
#include <gstsrtpunprotect.h>

#define PLUGIN_NAME "srtpunprotect"

#define DEFAULT_CRYPTO GST_CRYPTO_POLICY_AES_CM_128_HMAC_SHA1_80

GST_DEBUG_CATEGORY_STATIC (gst_srtp_unprotect_debug);
#define GST_CAT_DEFAULT gst_srtp_unprotect_debug

G_DEFINE_TYPE (GstSrtpUnprotect, gst_srtp_unprotect, GST_TYPE_ELEMENT);

/* pad templates */
static GstStaticPadTemplate gst_srtp_unprotect_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-srtp")
    );

static GstStaticPadTemplate gst_srtp_unprotect_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate gst_srtp_unprotect_rtcp_src_template =
GST_STATIC_PAD_TEMPLATE ("rtcp_src",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtcp")
    );

enum
{
  PROP_0,
  PROP_KEY,
  PROP_POLICY,
  PROP_LAST
};

static void
dispose_base64key (GstSrtpUnprotect * srtp)
{
  if (srtp->base64key != NULL) {
    g_free (srtp->base64key);
    srtp->base64key = NULL;
  }
}

static void
dispose_key (GstSrtpUnprotect * srtp)
{
  if (srtp->key != NULL) {
    g_free (srtp->key);
    srtp->key = NULL;
  }
}

static void
create_random_key (GstSrtpUnprotect * srtp)
{
  guint8 key[30];
  crypto_get_random (key, 30);
  srtp->key = (guint8 *) g_strdup ((gchar *) key);
  srtp->base64key = g_base64_encode (key, (gsize) 30);
}

static void
init_srtp_session (GstSrtpUnprotect * srtp)
{
  srtp_policy_t policy;

  memset (&policy, 0, sizeof (policy));
  switch (srtp->policy) {
    case GST_CRYPTO_POLICY_AES_CM_128_HMAC_SHA1_80:
      crypto_policy_set_aes_cm_128_hmac_sha1_80 (&policy.rtp);
      crypto_policy_set_aes_cm_128_hmac_sha1_80 (&policy.rtcp);
      break;
  }
  policy.ssrc.type = ssrc_any_inbound;
  policy.ssrc.value = 0;

  policy.key = srtp->key;
  srtp_create (&srtp->srtp_session, &policy);
}

static void
update_srtp_session (GstSrtpUnprotect * srtp)
{
  srtp_dealloc (srtp->srtp_session);

  init_srtp_session (srtp);
}

static void
gst_srtp_unprotect_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSrtpUnprotect *srtp = GST_SRTP_UNPROTECT (object);
  gsize key_len;

  switch (prop_id) {
    case PROP_POLICY:
      GST_OBJECT_LOCK (object);
      srtp->policy = g_value_get_enum (value);
      update_srtp_session (srtp);
      GST_OBJECT_UNLOCK (object);
      break;
    case PROP_KEY:
      GST_OBJECT_LOCK (object);
      dispose_base64key (srtp);
      srtp->base64key = g_value_dup_string (value);
      dispose_key (srtp);
      srtp->key = g_base64_decode (srtp->base64key, &key_len);
      update_srtp_session (srtp);
      GST_OBJECT_UNLOCK (object);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_srtp_unprotect_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSrtpUnprotect *srtp = GST_SRTP_UNPROTECT (object);

  switch (prop_id) {
    case PROP_POLICY:
      GST_OBJECT_LOCK (srtp);
      g_value_set_enum (value, srtp->policy);
      GST_OBJECT_UNLOCK (srtp);
      break;
    case PROP_KEY:
      GST_OBJECT_LOCK (srtp);
      g_value_set_string (value, srtp->base64key);
      GST_OBJECT_UNLOCK (srtp);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
forward_sticky_events (GstPad * pad, GstEvent ** event, gpointer user_data)
{
  GstPad *src = user_data;

  gst_pad_push_event (src, gst_event_ref (*event));

  return TRUE;
}

static void
gst_srtp_unprotect_create_rtp_src (GstSrtpUnprotect * self)
{
  GstElementClass *element_class;
  if (self->rtp_src != NULL)
    return;

  element_class = GST_ELEMENT_CLASS (G_OBJECT_GET_CLASS (self));
  self->rtp_src =
      gst_pad_new_from_template (gst_element_class_get_pad_template
      (element_class, "src"), "src");

  gst_pad_set_active (self->rtp_src, TRUE);
  gst_pad_sticky_events_foreach (self->sink, forward_sticky_events,
      self->rtp_src);

  gst_element_add_pad (GST_ELEMENT (self), self->rtp_src);
}

static void
gst_srtp_unprotect_create_rtcp_src (GstSrtpUnprotect * self)
{
  GstElementClass *element_class;
  if (self->rtcp_src != NULL)
    return;

  element_class = GST_ELEMENT_CLASS (G_OBJECT_GET_CLASS (self));
  self->rtcp_src =
      gst_pad_new_from_template (gst_element_class_get_pad_template
      (element_class, "rtcp_src"), "rtcp_src");

  gst_pad_set_active (self->rtcp_src, TRUE);
  gst_pad_sticky_events_foreach (self->sink, forward_sticky_events,
      self->rtcp_src);

  gst_element_add_pad (GST_ELEMENT (self), self->rtcp_src);
}

static GstFlowReturn
gst_srtp_unprotect_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  gboolean is_rtcp = FALSE;
  GstMapInfo info;
  gsize size;
  GstSrtpUnprotect *self = GST_SRTP_UNPROTECT (parent);
  buf = gst_buffer_make_writable (buf);

  gst_buffer_map (buf, &info, GST_MAP_WRITE);

  if (info.size <= 0) {
    GST_WARNING ("Invalid buffer");
    gst_buffer_unmap (buf, &info);
    gst_buffer_unref (buf);
    return GST_FLOW_OK;
  }
  // Ensure that the srtp structure does not change during decription
  GST_OBJECT_LOCK (self);
  if (srtp_unprotect (self->srtp_session, info.data, (gint *) & info.size) != 0) {
    if (srtp_unprotect_rtcp (self->srtp_session, info.data,
            (gint *) & info.size) != 0) {
      GST_OBJECT_UNLOCK (self);
      GST_WARNING ("Invalid buffer");
      gst_buffer_unmap (buf, &info);
      gst_buffer_unref (buf);
      return GST_FLOW_OK;
    }
    is_rtcp = TRUE;
  }

  GST_OBJECT_UNLOCK (self);
  size = info.size;

  gst_buffer_unmap (buf, &info);
  gst_buffer_set_size (buf, size);

  if (is_rtcp) {
    gst_srtp_unprotect_create_rtcp_src (self);
    return gst_pad_push (self->rtcp_src, buf);
  } else {
    gst_srtp_unprotect_create_rtp_src (self);
    return gst_pad_push (self->rtp_src, buf);
  }
}

static void
gst_srtp_unprotect_dispose (GObject * object)
{
  GstSrtpUnprotect *srtp = GST_SRTP_UNPROTECT (object);
  dispose_base64key (srtp);
  dispose_key (srtp);
  srtp_dealloc (srtp->srtp_session);

  G_OBJECT_CLASS (gst_srtp_unprotect_parent_class)->dispose (object);
}

static void
gst_srtp_unprotect_init (GstSrtpUnprotect * self)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (self);

  self->policy = DEFAULT_CRYPTO;
  create_random_key (self);
  init_srtp_session (self);

  self->sink =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "sink"), "sink");
  gst_pad_set_chain_function (self->sink,
      GST_DEBUG_FUNCPTR (gst_srtp_unprotect_chain));

  self->rtp_src = NULL;
  self->rtcp_src = NULL;

  gst_element_add_pad (GST_ELEMENT_CAST (self), self->sink);
}

static void
gst_srtp_unprotect_class_init (GstSrtpUnprotectClass * gst_srtp_unprotect_class)
{
  GstElementClass *gst_element_class;
  GObjectClass *gobject_class;
  gst_element_class = GST_ELEMENT_CLASS (gst_srtp_unprotect_class);
  gobject_class = G_OBJECT_CLASS (gst_srtp_unprotect_class);

  srtp_init ();

  gst_element_class_add_pad_template (gst_element_class,
      gst_static_pad_template_get (&gst_srtp_unprotect_sink_template));
  gst_element_class_add_pad_template (gst_element_class,
      gst_static_pad_template_get (&gst_srtp_unprotect_src_template));
  gst_element_class_add_pad_template (gst_element_class,
      gst_static_pad_template_get (&gst_srtp_unprotect_rtcp_src_template));

  gst_element_class_set_static_metadata (gst_element_class, "Srtp Unprotect",
      "Security/Unprotecter/SRTP",
      "Unprotects rtp and rtcp streams",
      "José Antonio Santos " "<santoscadenas@kurento.com>");

  gobject_class->set_property = gst_srtp_unprotect_set_property;
  gobject_class->get_property = gst_srtp_unprotect_get_property;
  gobject_class->dispose = gst_srtp_unprotect_dispose;

  g_object_class_install_property (gobject_class, PROP_POLICY,
      g_param_spec_enum ("policy", "Policy",
          "Cypher algorithm policy",
          GST_CRYPTO_POLICY_TYPE,
          GST_CRYPTO_POLICY_DEFAULT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_KEY,
      g_param_spec_string ("key", "Key",
          "Cypher key", NULL, G_PARAM_READWRITE));

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, PLUGIN_NAME, 0, "Srtpunprotect");
}

gboolean
gst_srtp_unprotect_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, PLUGIN_NAME, GST_RANK_NONE,
      GST_TYPE_SRTP_UNPROTECT);
}
