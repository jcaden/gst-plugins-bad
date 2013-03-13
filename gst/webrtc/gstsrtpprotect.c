/*
 * gstsrtpprotect.c - Source for webrtcbin
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
#include <gstsrtpprotect.h>

#define PLUGIN_NAME "srtpprotect"

#define DEFAULT_CRYPTO GST_CRYPTO_POLICY_AES_CM_128_HMAC_SHA1_80

GST_DEBUG_CATEGORY_STATIC (gst_srtp_protect_debug);
#define GST_CAT_DEFAULT gst_srtp_protect_debug

G_DEFINE_TYPE (GstSrtpProtect, gst_srtp_protect, GST_TYPE_ELEMENT);

/* pad templates */
static GstStaticPadTemplate gst_srtp_protect_rtp_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate gst_srtp_protect_rtcp_sink_template =
GST_STATIC_PAD_TEMPLATE ("rtcp_sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtcp")
    );

static GstStaticPadTemplate gst_srtp_protect_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-srtp")
    );

enum
{
  PROP_0,
  PROP_KEY,
  PROP_POLICY,
  PROP_LAST
};

static void
dispose_base64key (GstSrtpProtect * srtp)
{
  if (srtp->base64key != NULL) {
    g_free (srtp->base64key);
    srtp->base64key = NULL;
  }
}

static void
dispose_key (GstSrtpProtect * srtp)
{
  if (srtp->key != NULL) {
    g_free (srtp->key);
    srtp->key = NULL;
  }
}

static void
create_random_key (GstSrtpProtect * srtp)
{
  guint8 key[30];
  crypto_get_random (key, 30);
  srtp->key = (guint8 *) g_strdup ((gchar *) key);
  srtp->base64key = g_base64_encode (key, (gsize) 30);
}

static void
init_srtp_session (GstSrtpProtect * srtp)
{
  srtp_policy_t policy;

  memset (&policy, 0, sizeof (policy));
  switch (srtp->policy) {
    case GST_CRYPTO_POLICY_AES_CM_128_HMAC_SHA1_80:
      crypto_policy_set_aes_cm_128_hmac_sha1_80 (&policy.rtp);
      crypto_policy_set_aes_cm_128_hmac_sha1_80 (&policy.rtcp);
      break;
  }
  policy.ssrc.type = ssrc_any_outbound;
  policy.ssrc.value = 0;

  policy.key = srtp->key;
  srtp_create (&srtp->srtp_session, &policy);
}

static void
update_srtp_session (GstSrtpProtect * srtp)
{
  srtp_dealloc (srtp->srtp_session);

  init_srtp_session (srtp);
}

static void
gst_srtp_protect_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSrtpProtect *srtp = GST_SRTP_PROTECT (object);
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
gst_srtp_protect_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSrtpProtect *srtp = GST_SRTP_PROTECT (object);

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

static GstFlowReturn
gst_srtp_protect_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstMapInfo info;
  GstMemory *mem;
  gsize size;
  GstSrtpProtect *self = GST_SRTP_PROTECT (parent);
  buf = gst_buffer_make_writable (buf);

  gst_buffer_map (buf, &info, GST_MAP_WRITE);

  if (info.size <= 0) {
    GST_WARNING ("Invalid buffer");
    gst_buffer_unmap (buf, &info);
    gst_buffer_unref (buf);
    return GST_FLOW_OK;
  }

  if (SRTP_MAX_TAG_LEN > info.maxsize - info.size) {
    // Add more space to the buffer to be able to host the autentication tag
    size = info.size;
    gst_buffer_unmap (buf, &info);

    mem = gst_allocator_alloc (NULL, SRTP_MAX_TAG_LEN, NULL);

    gst_buffer_append_memory (buf, mem);
    gst_buffer_map (buf, &info, GST_MAP_WRITE);

    info.size = size;
  }
  // Ensure that the srtp structure does not change during decription
  GST_OBJECT_LOCK (self);
  if (g_str_has_prefix (GST_PAD_NAME (pad), "rtcp")) {
    if (srtp_protect_rtcp (self->srtp_session, info.data,
            (gint *) & info.size) != 0) {
      GST_OBJECT_UNLOCK (self);
      GST_WARNING ("Invalid rtcp buffer");
      gst_buffer_unmap (buf, &info);
      gst_buffer_unref (buf);
      return GST_FLOW_OK;
    }
  } else {
    if (srtp_protect (self->srtp_session, info.data, (gint *) & info.size) != 0) {
      GST_OBJECT_UNLOCK (self);
      GST_WARNING ("Invalid rtp buffer");
      gst_buffer_unmap (buf, &info);
      gst_buffer_unref (buf);
      return GST_FLOW_OK;
    }
  }
  GST_OBJECT_UNLOCK (self);
  size = info.size;

  gst_buffer_unmap (buf, &info);
  gst_buffer_set_size (buf, size);

  return gst_pad_push (self->src, buf);
}

static void
gst_srtp_protect_dispose (GObject * object)
{
  GstSrtpProtect *srtp = GST_SRTP_PROTECT (object);
  dispose_base64key (srtp);
  dispose_key (srtp);
  srtp_dealloc (srtp->srtp_session);

  G_OBJECT_CLASS (gst_srtp_protect_parent_class)->dispose (object);
}

static void
gst_srtp_protect_init (GstSrtpProtect * self)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (self);

  self->policy = DEFAULT_CRYPTO;
  create_random_key (self);
  init_srtp_session (self);

  self->rtp_sink = NULL;
  self->rtcp_sink = NULL;

  self->src =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "src"), "src");

  gst_element_add_pad (GST_ELEMENT_CAST (self), self->src);
}

static GstPad *
gst_srtp_protect_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstSrtpProtect *self = GST_SRTP_PROTECT (element);
  GstPad **pad;

  if (g_str_has_prefix (GST_PAD_TEMPLATE_NAME_TEMPLATE (templ), "rctp")) {
    pad = &self->rtcp_sink;
  } else {
    pad = &self->rtp_sink;
  }

  *pad = gst_pad_new_from_template (templ, name);

  gst_pad_set_chain_function (*pad, GST_DEBUG_FUNCPTR (gst_srtp_protect_chain));

  if (GST_STATE (element) >= GST_STATE_PAUSED) {
    gst_pad_set_active (*pad, TRUE);
  }
  gst_element_add_pad (element, *pad);

  return *pad;
}

static void
gst_srtp_protect_release_pad (GstElement * element, GstPad * pad)
{
  GstSrtpProtect *self = GST_SRTP_PROTECT (element);

  if (self->rtp_sink == pad) {
    self->rtp_sink = NULL;
  } else if (self->rtcp_sink == pad) {
    self->rtcp_sink = NULL;
  }

  gst_element_remove_pad (element, pad);
}

static void
gst_srtp_protect_class_init (GstSrtpProtectClass * gst_srtp_protect_class)
{
  GstElementClass *gst_element_class;
  GObjectClass *gobject_class;
  gst_element_class = GST_ELEMENT_CLASS (gst_srtp_protect_class);
  gobject_class = G_OBJECT_CLASS (gst_srtp_protect_class);

  srtp_init ();

  gst_element_class_add_pad_template (gst_element_class,
      gst_static_pad_template_get (&gst_srtp_protect_rtp_sink_template));
  gst_element_class_add_pad_template (gst_element_class,
      gst_static_pad_template_get (&gst_srtp_protect_src_template));
  gst_element_class_add_pad_template (gst_element_class,
      gst_static_pad_template_get (&gst_srtp_protect_rtcp_sink_template));

  gst_element_class_set_static_metadata (gst_element_class, "Srtp Protect",
      "Security/Protecter/SRTP",
      "Protects rtp and rtcp streams",
      "José Antonio Santos " "<santoscadenas@kurento.com>");

  gobject_class->set_property = gst_srtp_protect_set_property;
  gobject_class->get_property = gst_srtp_protect_get_property;
  gobject_class->dispose = gst_srtp_protect_dispose;

  g_object_class_install_property (gobject_class, PROP_POLICY,
      g_param_spec_enum ("policy", "Policy",
          "Cypher algorithm policy",
          GST_CRYPTO_POLICY_TYPE,
          GST_CRYPTO_POLICY_DEFAULT, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_KEY,
      g_param_spec_string ("key", "Key",
          "Cypher key", NULL, G_PARAM_READWRITE));

  gst_element_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_srtp_protect_request_new_pad);
  gst_element_class->release_pad =
      GST_DEBUG_FUNCPTR (gst_srtp_protect_release_pad);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, PLUGIN_NAME, 0, "Srtpprotect");
}

gboolean
gst_srtp_protect_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, PLUGIN_NAME, GST_RANK_NONE,
      GST_TYPE_SRTP_PROTECT);
}
