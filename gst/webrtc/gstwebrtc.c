/*
 * gstwebrtc.c - Source for webrtcbin
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
#include <gst/gst.h>

#include <gstsrtpunprotect.h>
#include <gstsrtpprotect.h>
#include <gstwebrtcbin.h>

static gboolean
webrtc_init (GstPlugin * webrtc)
{
  if (!gst_srtp_unprotect_plugin_init (webrtc))
    return FALSE;

  if (!gst_srtp_protect_plugin_init (webrtc))
    return FALSE;

  if (!gst_webrtc_bin_plugin_init (webrtc))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    webrtc,
    "Webrtc plugins",
    webrtc_init, VERSION, "LGPL", "Kurento", "http://kurento.com/")
