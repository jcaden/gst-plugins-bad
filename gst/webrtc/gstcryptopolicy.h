/*
 * gstcryptopolicy.h - Source for webrtcbin
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

#ifndef __GST_CRYPTO_POLICY_H__
#define __GST_CRYPTO_POLICY_H__

#include <glib-object.h>

typedef enum _GstCryptoPolicy GstCryptoPolicy;

enum _GstCryptoPolicy
{
  GST_CRYPTO_POLICY_AES_CM_128_HMAC_SHA1_80
};

#define GST_CRYPTO_POLICY_DEFAULT GST_CRYPTO_POLICY_AES_CM_128_HMAC_SHA1_80

#define GST_CRYPTO_POLICY_TYPE (gst_crypto_policy_get_type())

GType gst_crypto_policy_get_type (void);

#endif /* __GST_CRYPTO_POLICY_H__ */
