/*
 * gstcryptopolicy.c - Source for webrtcbin
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

#include "gstcryptopolicy.h"

GType
gst_crypto_policy_get_type (void)
{
  static GType mode_type = 0;
  static const GEnumValue mode_types[] = {
    {GST_CRYPTO_POLICY_AES_CM_128_HMAC_SHA1_80, "aes_cm_128_hmac_sha1_80",
        "AES_CM_128_HMAC_SHA1_80"},
    {0, NULL, NULL},
  };

  if (!mode_type) {
    mode_type = g_enum_register_static ("GstCryptoPolicy", mode_types);
  }

  return mode_type;
}
