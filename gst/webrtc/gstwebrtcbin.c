/*
 * gstwebrtcbin.c - Source for webrtcbin
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
#include <gstwebrtcbin.h>

#include <stdlib.h>

#define PLUGIN_NAME "webrtcbin"

GST_DEBUG_CATEGORY_STATIC (gst_webrtc_bin_debug);
#define GST_CAT_DEFAULT gst_webrtc_bin_debug

/* Credentials constants */
#define CREDENDIALS "credentials"
#define USER "user"
#define PASSWD "passwd"

/* Candidates constants */
#define CANDIDATES "candidates"
#define CANDIDATE "candidate"
#define FOUNDATION "foundation"
#define COMPONENT "component"
#define TRANSPORT "transport"
#define PRIORITY "priority"
#define ADDRESS "address"
#define PORT "port"
#define TYPE "type"
#define SDPLINE "sdpline"

#define CANDIDATE_PREFIX "a=candidate:"

G_DEFINE_TYPE (GstWebrtcBin, gst_webrtc_bin, GST_TYPE_BIN);

/* pad templates */
static GstStaticPadTemplate gst_webrtc_bin_rtp_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate gst_webrtc_bin_rtcp_src_template =
GST_STATIC_PAD_TEMPLATE ("rtcp_src",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("application/x-rtcp")
    );

/* pad templates */
static GstStaticPadTemplate gst_webrtc_bin_rtp_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtp")
    );

static GstStaticPadTemplate gst_webrtc_rtcp_sink_template =
GST_STATIC_PAD_TEMPLATE ("rtcp_sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/x-rtcp")
    );

/* signals */
enum
{
  SIGNAL_LOCAL_CANDIDATES_GATHERED,
  SIGNAL_SET_REMOTE_DESCRIPTOR,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SEND_KEY,
  PROP_RECV_KEY,
  PROP_STUN_SERVER_IP,
  PROP_STUN_SERVER_PORT,
  PROP_LOCAL_ICE_CREDENTIALS,
  PROP_REMOTE_ICE_CREDENTIALS,
  PROP_LOCAL_ICE_CANDIDATES,
  PROP_REMOTE_ICE_CANDIDATES,
  PROP_LAST
};

static guint gst_webrtc_bin_signals[LAST_SIGNAL] = { 0 };

static void
gst_webrtc_release_object (GObject ** obj)
{
  g_object_unref (*obj);
  *obj = NULL;
}


static void
gst_webrtc_bin_srtpunprotect_pad_added (GstElement * srtpunprotect,
    GstPad * pad, gpointer data)
{
  GstElement *self = data;
  GstPadTemplate *pad_templ;
  GstPad *gp;

  pad_templ =
      gst_element_class_get_pad_template (GST_ELEMENT_CLASS (G_OBJECT_GET_CLASS
          (self)), GST_PAD_NAME (pad));

  if (pad_templ == NULL)
    return;

  gp = gst_ghost_pad_new_from_template (GST_PAD_NAME (pad), pad, pad_templ);

  g_object_unref (pad_templ);

  if (GST_STATE (srtpunprotect) >= GST_STATE_PAUSED)
    gst_pad_set_active (gp, TRUE);

  gst_element_add_pad (self, gp);
}

static void
gst_webrtc_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstWebrtcBin *webrtcbin = GST_WEBRTC_BIN (object);

  switch (prop_id) {
    case PROP_RECV_KEY:
      if (webrtcbin->srtpunprotect != NULL) {
        g_object_set_property (G_OBJECT (webrtcbin->srtpunprotect), "key",
            value);
      }
      break;
    case PROP_SEND_KEY:
      if (webrtcbin->srtpprotect != NULL) {
        g_object_set_property (G_OBJECT (webrtcbin->srtpprotect), "key", value);
      }
      break;
    case PROP_STUN_SERVER_IP:
      if (webrtcbin->agent != NULL) {
        g_object_set_property (G_OBJECT (webrtcbin->agent), "stun-server",
            value);
      }
      break;
    case PROP_STUN_SERVER_PORT:
      if (webrtcbin->agent != NULL) {
        g_object_set_property (G_OBJECT (webrtcbin->agent), "stun-server-port",
            value);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_webrtc_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstWebrtcBin *webrtcbin = GST_WEBRTC_BIN (object);

  switch (prop_id) {
    case PROP_RECV_KEY:
      if (webrtcbin->srtpunprotect != NULL) {
        g_object_get_property (G_OBJECT (webrtcbin->srtpunprotect), "key",
            value);
      }
      break;
    case PROP_SEND_KEY:
      if (webrtcbin->srtpprotect != NULL) {
        g_object_get_property (G_OBJECT (webrtcbin->srtpprotect), "key", value);
      }
      break;
    case PROP_STUN_SERVER_IP:
      if (webrtcbin->agent != NULL) {
        g_object_get_property (G_OBJECT (webrtcbin->agent), "stun-server",
            value);
      }
      break;
    case PROP_STUN_SERVER_PORT:
      if (webrtcbin->agent != NULL) {
        g_object_get_property (G_OBJECT (webrtcbin->agent), "stun-server-port",
            value);
      }
      break;
    case PROP_LOCAL_ICE_CREDENTIALS:
      if (webrtcbin->local_credentials != NULL)
        g_value_set_boxed (value, webrtcbin->local_credentials);
      break;
    case PROP_REMOTE_ICE_CREDENTIALS:
      if (webrtcbin->remote_credentials != NULL)
        g_value_set_boxed (value, webrtcbin->remote_credentials);
      break;
    case PROP_LOCAL_ICE_CANDIDATES:
      if (webrtcbin->local_candidates != NULL)
        g_value_set_boxed (value, webrtcbin->local_candidates);
      break;
    case PROP_REMOTE_ICE_CANDIDATES:
      if (webrtcbin->remote_candidates != NULL)
        g_value_set_boxed (value, webrtcbin->remote_candidates);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_webrtc_bin_dispose (GObject * object)
{
  GstWebrtcBin *self = GST_WEBRTC_BIN (object);

  g_mutex_clear (&self->credentials_lock);

  if (self->nicesink != NULL) {
    g_object_unref (self->nicesink);
    self->nicesink = NULL;
  }

  if (self->nicesrc != NULL) {
    g_object_unref (self->nicesrc);
    self->nicesrc = NULL;
  }

  if (self->srtpunprotect != NULL) {
    g_object_unref (self->srtpunprotect);
    self->srtpunprotect = NULL;
  }

  if (self->srtpprotect != NULL) {
    g_object_unref (self->srtpprotect);
    self->srtpprotect = NULL;
  }

  if (self->agent != NULL) {
    nice_agent_remove_stream (self->agent, self->stream);
    g_object_unref (self->agent);
    self->agent = NULL;
  }

  if (self->local_candidates != NULL) {
    gst_structure_free (self->local_candidates);
    self->local_candidates = NULL;
  }

  if (self->remote_candidates != NULL) {
    gst_structure_free (self->remote_candidates);
    self->remote_candidates = NULL;
  }

  if (self->local_credentials != NULL) {
    gst_structure_free (self->local_credentials);
    self->local_credentials = NULL;
  }

  if (self->remote_credentials != NULL) {
    gst_structure_free (self->remote_credentials);
    self->remote_credentials = NULL;
  }

  if (self->loop != NULL) {
    g_main_loop_quit (self->loop);
    g_main_loop_unref (self->loop);
    self->loop = NULL;
  }

  if (self->context != NULL) {
    g_main_context_unref (self->context);
    self->context = NULL;
  }

  G_OBJECT_CLASS (gst_webrtc_bin_parent_class)->dispose (object);
}

static GstStructure *
create_structure_from_candidate (NiceCandidate * cand)
{
  GstStructure *st;
  gchar addr[NICE_ADDRESS_STRING_LEN];
  guint port;
  const gchar *transport, *typ;
  gchar *sdpline;

  nice_address_to_string (&(cand->addr), addr);
  port = nice_address_get_port (&(cand->addr));

  if (g_strstr_len (addr, -1, ":") != NULL) {
    return NULL;
  }

  if (cand->transport == NICE_CANDIDATE_TRANSPORT_UDP) {
    transport = "udp";
  } else {
    return NULL;
  }

  if (cand->type == NICE_CANDIDATE_TYPE_HOST) {
    typ = "host";
  } else if (cand->type == NICE_CANDIDATE_TYPE_SERVER_REFLEXIVE) {
    typ = "srflx";
  } else {
    return NULL;
  }

  sdpline = g_strdup_printf (CANDIDATE_PREFIX "%s %d %s %d %s %d typ %s\n",
      cand->foundation, cand->component_id, transport,
      cand->priority, addr, port, typ);

  st = gst_structure_new (CANDIDATE,
      FOUNDATION, G_TYPE_STRING, cand->foundation,
      COMPONENT, G_TYPE_UINT, cand->component_id,
      TRANSPORT, G_TYPE_STRING, transport,
      PRIORITY, G_TYPE_UINT, cand->priority,
      ADDRESS, G_TYPE_STRING, addr,
      PORT, G_TYPE_UINT, port,
      TYPE, G_TYPE_STRING, typ, SDPLINE, G_TYPE_STRING, sdpline, NULL);
  g_free (sdpline);
  return st;
}

static void
gst_webrtc_bin_candidate_gathering_done (NiceAgent * agent, guint stream_id,
    GstWebrtcBin * self)
{
  GSList *l;
  GSList *lc = nice_agent_get_local_candidates (self->agent, self->stream, 1);
  gint id = 0;
  GST_DEBUG ("Gathering done %d candidates", g_slist_length (lc));

  for (l = lc; l != NULL; l = l->next) {
    NiceCandidate *cand = l->data;
    GstStructure *st;
    gchar *name;
    st = create_structure_from_candidate (cand);
    if (st != NULL) {
      name = g_strdup_printf (CANDIDATE "_%d", id);
      gst_structure_set (self->local_candidates,
          name, GST_TYPE_STRUCTURE, st, NULL);
      gst_structure_free (st);
      id++;
      g_free (name);
    }
  }
  g_slist_free_full (lc, (GDestroyNotify) nice_candidate_free);

  g_signal_emit (G_OBJECT (self),
      gst_webrtc_bin_signals[SIGNAL_LOCAL_CANDIDATES_GATHERED], 0,
      self->local_candidates);
}

static void
gst_webrtc_bin_init_local_credentials (GstWebrtcBin * self)
{
  gchar *user;
  gchar *passwd;
  nice_agent_get_local_credentials (self->agent, self->stream, &user, &passwd);
  self->local_credentials = gst_structure_new (CREDENDIALS,
      USER, G_TYPE_STRING, user, PASSWD, G_TYPE_STRING, passwd, NULL);
  g_free (user);
  g_free (passwd);
}

static void
gst_webrtc_bin_nice_recv (NiceAgent * agent, guint stream_id,
    guint component_id, guint len, gchar * buf, gpointer user_data)
{
  // Nothing to do, this callback is only for negotiation
}

static void
gst_webrtc_bin_create_nicesrc (GstWebrtcBin * self)
{
  self->nicesrc = gst_element_factory_make ("nicesrc", "nicesrc");
  if (self->nicesrc == NULL) {
    GST_ELEMENT_ERROR (self, CORE, MISSING_PLUGIN,
        ("Plugin nicesrc is missing"), NULL);
    return;
  }

  g_object_set (self->nicesrc, "agent", self->agent, "stream", self->stream,
      "component", 1, NULL);

  g_object_ref (self->nicesrc);

  if (!gst_bin_add (GST_BIN (self), self->nicesrc)) {
    g_object_unref (self->nicesrc);
  }

  if (!gst_element_link (self->nicesrc, self->srtpunprotect)) {
    GST_ELEMENT_ERROR (self, CORE, NEGOTIATION,
        ("Cannot link nicesrc with srtpunprotect"), NULL);
    return;
  }

  gst_element_sync_state_with_parent (self->nicesrc);
}

static void
gst_webrtc_bin_agent_state_changed (NiceAgent * agent, guint stream_id,
    guint component_id, guint state, GstWebrtcBin * self)
{
  GstMessage *message;
  GstStructure *st;

  st = gst_structure_new_empty ("application/x-webrtc-agent-status");

  switch (state) {
    case NICE_COMPONENT_STATE_DISCONNECTED:
      gst_structure_set (st, "agent-status", G_TYPE_STRING, "disconnected",
          NULL);
      break;
    case NICE_COMPONENT_STATE_GATHERING:
      gst_structure_set (st, "agent-status", G_TYPE_STRING, "gathering", NULL);
      break;
    case NICE_COMPONENT_STATE_CONNECTING:
      gst_structure_set (st, "agent-status", G_TYPE_STRING, "connecting", NULL);
      break;
    case NICE_COMPONENT_STATE_CONNECTED:
      gst_structure_set (st, "agent-status", G_TYPE_STRING, "connected", NULL);
      break;
    case NICE_COMPONENT_STATE_READY:
      gst_webrtc_bin_create_nicesrc (self);
      gst_structure_set (st, "agent-status", G_TYPE_STRING, "ready", NULL);
      break;
    case NICE_COMPONENT_STATE_FAILED:
      GST_ERROR_OBJECT (self, "Error connecting agent");
      gst_structure_set (st, "agent-status", G_TYPE_STRING, "failed", NULL);
      break;
  }

  message = gst_message_new_element (GST_OBJECT (self), st);
  gst_element_post_message (GST_ELEMENT (self), message);
}

static gpointer
gst_webrtc_bin_loop_thread (gpointer data)
{
  GstWebrtcBin *self = GST_WEBRTC_BIN (data);

  g_main_loop_run (self->loop);

  return NULL;
}

static void
gst_webrtc_bin_init (GstWebrtcBin * self)
{
  GThread *loop_thread;

  g_mutex_init (&self->credentials_lock);

  self->agent = NULL;
  self->nicesrc = NULL;
  self->nicesink = NULL;
  self->srtpprotect = NULL;
  self->srtpunprotect = NULL;
  self->remote_credentials = NULL;
  self->remote_candidates = NULL;
  self->local_credentials = NULL;
  self->local_candidates = NULL;
  self->agent_init = FALSE;
  self->loop = NULL;

  self->context = g_main_context_new ();
  if (self->context == NULL)
    return;

  self->loop = g_main_loop_new (self->context, TRUE);
  if (self->loop == NULL)
    return;

  loop_thread = g_thread_new ("webrtcbinctx", gst_webrtc_bin_loop_thread, self);
  g_thread_unref (loop_thread);

  self->srtpunprotect = gst_element_factory_make ("srtpunprotect",
      "srtpunprotect");
  if (self->srtpunprotect == NULL)
    return;

  self->srtpprotect = gst_element_factory_make ("srtpprotect", "srtpprotect");
  if (self->srtpprotect == NULL) {
    gst_webrtc_release_object ((GObject **) & self->srtpunprotect);
    return;
  }

  g_object_ref (self->srtpprotect);
  g_object_ref (self->srtpunprotect);

  gst_bin_add_many (GST_BIN (self), self->srtpunprotect, self->srtpprotect,
      NULL);

  g_signal_connect (self->srtpunprotect, "pad-added",
      G_CALLBACK (gst_webrtc_bin_srtpunprotect_pad_added), self);

  self->agent = nice_agent_new (self->context, NICE_COMPATIBILITY_RFC5245);

  if (self->agent == NULL)
    return;

  self->stream = nice_agent_add_stream (self->agent, 1);

  if (self->stream == 0) {
    g_object_unref (self->agent);
    self->agent = NULL;
    return;
  }

  g_object_set (self->agent, "upnp", FALSE, NULL);

  nice_agent_attach_recv (self->agent, self->stream, 1, self->context,
      gst_webrtc_bin_nice_recv, NULL);

  g_signal_connect (G_OBJECT (self->agent), "candidate-gathering-done",
      G_CALLBACK (gst_webrtc_bin_candidate_gathering_done), self);

  g_signal_connect (G_OBJECT (self->agent), "component-state-changed",
      G_CALLBACK (gst_webrtc_bin_agent_state_changed), self);

  self->local_candidates = gst_structure_new_empty (CANDIDATES);

  gst_webrtc_bin_init_local_credentials (self);
}

static gboolean
gst_webrtc_bin_create_agent (GstWebrtcBin * self)
{
  if (self->agent_init)
    return TRUE;

  self->agent_init = TRUE;

  if (!nice_agent_gather_candidates (self->agent, self->stream)) {
    g_object_unref (self->agent);
    self->agent = NULL;
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_webrtc_bin_check_status (GstWebrtcBin * self)
{
  return self->agent && self->srtpunprotect;
}

static GstStateChangeReturn
gst_webrtc_bin_change_state (GstElement * element, GstStateChange transition)
{
  GstWebrtcBin *self = GST_WEBRTC_BIN (element);

  if (transition == GST_STATE_CHANGE_NULL_TO_READY) {
    if (!gst_webrtc_bin_create_agent (self)) {
      GST_ELEMENT_ERROR (self, CORE, STATE_CHANGE,
          ("Cannot create agent"), NULL);
      return GST_STATE_CHANGE_FAILURE;
    }
  } else if (!gst_webrtc_bin_check_status (self)) {
    GST_ELEMENT_ERROR (self, CORE, STATE_CHANGE,
        ("Wrong plugin initialization"), NULL);
    return GST_STATE_CHANGE_FAILURE;
  }

  return GST_ELEMENT_CLASS (gst_webrtc_bin_parent_class)->change_state (element,
      transition);
}

static GstPad *
gst_webrtc_bin_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstWebrtcBin *self = GST_WEBRTC_BIN (element);
  GstPad *pad, *gp;

  if (self->nicesink == NULL) {
    self->nicesink = gst_element_factory_make ("nicesink", "nicesink");
    if (self->nicesink == NULL) {
      return NULL;
    }

    gst_object_ref (self->nicesink);

    g_object_set (self->nicesink, "agent", self->agent, "stream", self->stream,
        "component", 1, NULL);

    gst_bin_add (GST_BIN (self), self->nicesink);
    gst_element_sync_state_with_parent (self->nicesink);
    if (!gst_element_link (self->srtpprotect, self->nicesink)) {
      gst_webrtc_release_object ((GObject **) & self->nicesink);
      return NULL;
    }
  }

  pad = gst_element_request_pad (self->srtpprotect, templ, name, caps);

  if (pad == NULL)
    return NULL;

  gp = gst_ghost_pad_new_from_template (GST_PAD_NAME (pad), pad, templ);
  if (GST_STATE (element) >= GST_STATE_PAUSED)
    gst_pad_set_active (gp, TRUE);

  gst_element_add_pad (element, gp);

  return gp;
}

static void
gst_webrtc_bin_release_pad (GstElement * element, GstPad * gp)
{
  GstWebrtcBin *self = GST_WEBRTC_BIN (element);
  GstPad *pad;

  if (self->srtpprotect != NULL) {
    GST_DEBUG ("Relasing pad");
    pad = gst_element_get_static_pad (self->srtpprotect, GST_PAD_NAME (gp));
    if (pad != NULL) {
      gst_element_release_request_pad (self->srtpprotect, pad);
      g_object_unref (pad);
    }
  }

  gst_element_remove_pad (element, gp);
}

static void
process_sdp_line (GstStructure * candidate)
{
  const gchar *sdpline, *cand;
  gchar *token;
  gchar **tokens;
  int i;

  if (gst_structure_get_name_id (candidate) != g_quark_from_string (CANDIDATE))
    return;

  sdpline = gst_structure_get_string (candidate, SDPLINE);

  if (sdpline == NULL)
    return;

  cand = sdpline + sizeof (CANDIDATE_PREFIX) - 1;

  tokens = g_strsplit (cand, " ", 9);

  for (token = tokens[0], i = 0; token != NULL; token = tokens[++i]) {
    if (token[0] == '\0') {
      GST_WARNING ("Invalid candidate line: empty tokens");
      goto free;
    }
  }

  if (i < 8) {
    GST_WARNING ("Invalid candidate line: not enought tokens");
    goto free;
  }

  if (g_strcmp0 (tokens[2], "udp") != 0) {
    GST_WARNING ("Invalid candidate transport, only udp is supported");
    goto free;
  }

  if (g_strcmp0 (tokens[7], "host") != 0 && g_strcmp0 (tokens[7], "srflx") != 0) {
    GST_WARNING ("Invalid type: only host and srflx allowed");
    goto free;
  }

  if (!gst_structure_has_field_typed (candidate, FOUNDATION, G_TYPE_STRING))
    gst_structure_set (candidate, FOUNDATION, G_TYPE_STRING, tokens[0], NULL);

  if (!gst_structure_has_field_typed (candidate, COMPONENT, G_TYPE_UINT))
    gst_structure_set (candidate, COMPONENT, G_TYPE_UINT, atoi (tokens[1]),
        NULL);

  if (!gst_structure_has_field_typed (candidate, TRANSPORT, G_TYPE_STRING))
    gst_structure_set (candidate, TRANSPORT, G_TYPE_STRING, tokens[2], NULL);

  if (!gst_structure_has_field_typed (candidate, PRIORITY, G_TYPE_UINT))
    gst_structure_set (candidate, PRIORITY, G_TYPE_UINT, atoi (tokens[3]),
        NULL);

  if (!gst_structure_has_field_typed (candidate, ADDRESS, G_TYPE_STRING))
    gst_structure_set (candidate, ADDRESS, G_TYPE_STRING, tokens[4], NULL);

  if (!gst_structure_has_field_typed (candidate, PORT, G_TYPE_UINT))
    gst_structure_set (candidate, PORT, G_TYPE_UINT, atoi (tokens[5]), NULL);

  if (!gst_structure_has_field_typed (candidate, TYPE, G_TYPE_STRING))
    gst_structure_set (candidate, TYPE, G_TYPE_STRING, tokens[7], NULL);

free:
  g_strfreev (tokens);
}

static gboolean
check_candidate_fields (GstStructure * candidate)
{
  if (!gst_structure_has_field_typed (candidate, FOUNDATION, G_TYPE_STRING))
    return FALSE;

  if (!gst_structure_has_field_typed (candidate, COMPONENT, G_TYPE_UINT))
    return FALSE;

  if (!gst_structure_has_field_typed (candidate, TRANSPORT, G_TYPE_STRING))
    return FALSE;

  if (!gst_structure_has_field_typed (candidate, PRIORITY, G_TYPE_UINT))
    return FALSE;

  if (!gst_structure_has_field_typed (candidate, ADDRESS, G_TYPE_STRING))
    return FALSE;

  if (!gst_structure_has_field_typed (candidate, PORT, G_TYPE_UINT))
    return FALSE;

  if (!gst_structure_has_field_typed (candidate, TYPE, G_TYPE_STRING))
    return FALSE;

  return TRUE;
}

static NiceCandidate *
get_nice_candidate_from_structure (GstStructure * candidate)
{
  NiceCandidate *nice_candidate;
  NiceAddress *addr = NULL;
  NiceCandidateType type;
  const gchar *type_str, *addr_str, *foundation;
  guint port, priority;

  if (!check_candidate_fields (candidate))
    return NULL;

  type_str = gst_structure_get_string (candidate, TYPE);
  addr_str = gst_structure_get_string (candidate, ADDRESS);
  gst_structure_get_uint (candidate, PORT, &port);
  gst_structure_get_uint (candidate, PRIORITY, &priority);
  foundation = gst_structure_get_string (candidate, FOUNDATION);

  if (g_strcmp0 (type_str, "host") == 0) {
    type = NICE_CANDIDATE_TYPE_HOST;
  } else if (g_strcmp0 (type_str, "srflx") == 0) {
    type = NICE_CANDIDATE_TYPE_SERVER_REFLEXIVE;
  } else {
    GST_WARNING ("Invalid candidate type");
    return NULL;
  }

  nice_candidate = nice_candidate_new (type);

  if (nice_candidate == NULL)
    return NULL;

  addr = &(nice_candidate->addr);
  nice_address_init (addr);
  nice_address_set_from_string (addr, addr_str);
  nice_address_set_port (addr, port);

  /* Copy base address as the actual address */
  addr = &(nice_candidate->base_addr);
  nice_address_init (addr);
  nice_address_set_from_string (addr, addr_str);
  nice_address_set_port (addr, port);

  nice_candidate->priority = priority;
  g_strlcpy (nice_candidate->foundation, foundation,
      sizeof (nice_candidate->foundation));

  nice_candidate->username = NULL;
  nice_candidate->password = NULL;

  return nice_candidate;
}

static gboolean
process_candidates (GstWebrtcBin * self, GstStructure * candidates)
{
  gint i, len, id = 0;
  GSList *nice_candidates = NULL, *l;

  if (gst_structure_get_name_id (candidates) !=
      g_quark_from_string (CANDIDATES)) {
    return FALSE;
  }

  len = gst_structure_n_fields (candidates);
  if (len == 0)
    return FALSE;

  for (i = 0; i < len; i++) {
    GstStructure *candidate;
    const gchar *name;

    name = gst_structure_nth_field_name (candidates, i);
    if (name != NULL && g_str_has_prefix (name, CANDIDATE)) {
      if (gst_structure_get (candidates, name, GST_TYPE_STRUCTURE, &candidate,
              NULL)) {
        NiceCandidate *nice_candidate;
        process_sdp_line (candidate);
        nice_candidate = get_nice_candidate_from_structure (candidate);
        if (nice_candidate != NULL) {
          nice_candidates = g_slist_prepend (nice_candidates, nice_candidate);
        }
        gst_structure_free (candidate);
      }
    }
  }

  nice_agent_set_remote_candidates (self->agent, self->stream, 1,
      nice_candidates);

  g_slist_free_full (nice_candidates, (GDestroyNotify) nice_candidate_free);

  nice_candidates = nice_agent_get_remote_candidates (self->agent, self->stream,
      1);

  gst_structure_remove_all_fields (candidates);

  for (l = nice_candidates; l != NULL; l = l->next) {
    NiceCandidate *cand = l->data;
    GstStructure *st;
    gchar *name;
    st = create_structure_from_candidate (cand);
    if (st != NULL) {
      name = g_strdup_printf (CANDIDATE "_%d", id);
      gst_structure_set (candidates, name, GST_TYPE_STRUCTURE, st, NULL);
      gst_structure_free (st);
      id++;
      g_free (name);
    }
  }

  g_slist_free_full (nice_candidates, (GDestroyNotify) nice_candidate_free);

  return TRUE;
}

static gboolean
process_credentials (GstWebrtcBin * self, GstStructure * credentials)
{
  const gchar *user, *passwd;

  if (gst_structure_get_name_id (credentials) !=
      g_quark_from_string (CREDENDIALS)) {
    return FALSE;
  }

  user = gst_structure_get_string (credentials, USER);
  passwd = gst_structure_get_string (credentials, PASSWD);

  if (user == NULL || passwd == NULL)
    return FALSE;

  return nice_agent_set_remote_credentials (self->agent, self->stream, user,
      passwd);
}

static gboolean
gst_webrtc_bin_set_remote_descriptor (GstWebrtcBin * self,
    const GstStructure * candidates, const GstStructure * credentials)
{
  gboolean ret;
  GST_DEBUG ("Candidates: %P, credentials: %P", candidates, credentials);
  if (candidates == NULL || credentials == NULL)
    return FALSE;

  g_mutex_lock (&self->credentials_lock);
  if (self->remote_credentials == NULL && self->remote_candidates == NULL) {
    GstStructure *cands, *cred;

    cands = gst_structure_copy (candidates);
    cred = gst_structure_copy (credentials);

    if (!process_credentials (self, cred) || !process_candidates (self, cands)) {
      gst_structure_free (cands);
      gst_structure_free (cred);
      ret = FALSE;
    } else {
      ret = TRUE;
      self->remote_credentials = cred;
      self->remote_candidates = cands;
    }
  } else {
    ret = FALSE;
  }
  g_mutex_unlock (&self->credentials_lock);
  return ret;
}

static void
gst_webrtc_bin_class_init (GstWebrtcBinClass * gst_webrtc_bin_class)
{
  GstElementClass *gst_element_class;
  GObjectClass *gobject_class;
  gst_element_class = GST_ELEMENT_CLASS (gst_webrtc_bin_class);
  gobject_class = G_OBJECT_CLASS (gst_webrtc_bin_class);

  gst_webrtc_bin_class->set_remote_descriptor =
      GST_DEBUG_FUNCPTR (gst_webrtc_bin_set_remote_descriptor);

  gst_element_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_webrtc_bin_request_new_pad);
  gst_element_class->release_pad =
      GST_DEBUG_FUNCPTR (gst_webrtc_bin_release_pad);
  gst_element_class->change_state =
      GST_DEBUG_FUNCPTR (gst_webrtc_bin_change_state);

  gst_element_class_add_pad_template (gst_element_class,
      gst_static_pad_template_get (&gst_webrtc_bin_rtp_sink_template));
  gst_element_class_add_pad_template (gst_element_class,
      gst_static_pad_template_get (&gst_webrtc_rtcp_sink_template));
  gst_element_class_add_pad_template (gst_element_class,
      gst_static_pad_template_get (&gst_webrtc_bin_rtp_src_template));
  gst_element_class_add_pad_template (gst_element_class,
      gst_static_pad_template_get (&gst_webrtc_bin_rtcp_src_template));

  gst_element_class_set_static_metadata (gst_element_class, "Webrtc bin",
      "Source/Network",
      "Receives and sends rtp data from/to "
      "a remote webrtc peer",
      "José Antonio Santos " "<santoscadenas@kurento.com>");

  gobject_class->set_property = gst_webrtc_bin_set_property;
  gobject_class->get_property = gst_webrtc_bin_get_property;
  gobject_class->dispose = gst_webrtc_bin_dispose;

  g_object_class_install_property (gobject_class, PROP_SEND_KEY,
      g_param_spec_string ("send-key", "SendKey",
          "Cypher key", NULL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_RECV_KEY,
      g_param_spec_string ("recv-key", "RecvKey",
          "Cypher key to unprotect received data", NULL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_STUN_SERVER_IP,
      g_param_spec_string ("stun-server",
          "StunServer", "Stun Server IP Address", NULL, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_STUN_SERVER_PORT,
      g_param_spec_uint ("stun-server-port",
          "StunServerPort",
          "Stun Server Port", 1, 65536, 3478, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LOCAL_ICE_CANDIDATES,
      g_param_spec_boxed ("local-ice-candidates",
          "LocalIceCandidates",
          "Local ICE Candidates", GST_TYPE_STRUCTURE, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_REMOTE_ICE_CANDIDATES,
      g_param_spec_boxed ("remote-ice-candidates",
          "RemoteIceCandidates",
          "Remote ICE Candidates", GST_TYPE_STRUCTURE, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_LOCAL_ICE_CREDENTIALS,
      g_param_spec_boxed ("local-ice-credentials",
          "LocalIceCredentials",
          "Local ICE Credentials", GST_TYPE_STRUCTURE, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_REMOTE_ICE_CREDENTIALS,
      g_param_spec_boxed ("remote-ice-credentials",
          "RemoteIceCredentials",
          "Remote ICE Credentials", GST_TYPE_STRUCTURE, G_PARAM_READABLE));

  /* Signals initialization */
  gst_webrtc_bin_signals[SIGNAL_LOCAL_CANDIDATES_GATHERED] =
      g_signal_new ("local-candidates-gathered",
      G_TYPE_FROM_CLASS (gst_webrtc_bin_class), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstWebrtcBinClass, local_candidates_gathered), NULL,
      NULL, g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1, GST_TYPE_STRUCTURE);

  gst_webrtc_bin_signals[SIGNAL_SET_REMOTE_DESCRIPTOR] =
      g_signal_new ("set-remote-descriptor",
      G_TYPE_FROM_CLASS (gst_webrtc_bin_class),
      G_SIGNAL_ACTION | G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstWebrtcBinClass,
          set_remote_descriptor), NULL, NULL,
      g_cclosure_marshal_BOOLEAN__BOXED_BOXED, G_TYPE_BOOLEAN, 2,
      GST_TYPE_STRUCTURE, GST_TYPE_STRUCTURE);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, PLUGIN_NAME, 0, "WebrtcBin");
}

gboolean
gst_webrtc_bin_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, PLUGIN_NAME, GST_RANK_NONE,
      GST_TYPE_WEBRTC_BIN);
}
