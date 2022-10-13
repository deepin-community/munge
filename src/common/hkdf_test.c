/*****************************************************************************
 *  Written by Chris Dunlap <cdunlap@llnl.gov>.
 *  Copyright (C) 2007-2020 Lawrence Livermore National Security, LLC.
 *  Copyright (C) 2002-2007 The Regents of the University of California.
 *  UCRL-CODE-155910.
 *
 *  This file is part of the MUNGE Uid 'N' Gid Emporium (MUNGE).
 *  For details, see <https://dun.github.io/munge/>.
 *
 *  MUNGE is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.  Additionally for the MUNGE library (libmunge), you
 *  can redistribute it and/or modify it under the terms of the GNU Lesser
 *  General Public License as published by the Free Software Foundation,
 *  either version 3 of the License, or (at your option) any later version.
 *
 *  MUNGE is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  and GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  and GNU Lesser General Public License along with MUNGE.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *****************************************************************************/


#if HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <unistd.h>
#include <munge.h>
#include "crypto.h"
#include "hkdf.h"
#include "md.h"
#include "tap.h"


/*****************************************************************************
 *  Test cases from RFC 5869: HMAC-based Extract-and-Expand Key Derivation
 *    Function (HKDF), Appendix A.
 *
 *  Note that PRK is not being checked since it is internal to hkdf().
 *    But if OKM matches, PRK presumably does as well.
 *****************************************************************************/


int
hkdf_test (munge_mac_t md,
        const unsigned char *ikm,  size_t ikmlen,
        const unsigned char *salt, size_t saltlen,
        const unsigned char *info, size_t infolen,
        const unsigned char *okm,  size_t okmlen)
{
    unsigned char  buf[128];
    size_t         buflen;
    hkdf_ctx_t    *hkdfp;
    int            rv;

    hkdfp = hkdf_ctx_create ();
    if (hkdfp == NULL) {
        diag ("Failed to allocate memory for HKDF context");
        return -1;
    }
    rv = hkdf_ctx_set_md (hkdfp, md);
    if (rv == -1) {
        diag ("Failed to set HKDF message digest to md=%d", md);
        return -1;
    }
    if (ikm != NULL) {
        rv = hkdf_ctx_set_key (hkdfp, ikm, ikmlen);
        if (rv == -1) {
            diag ("Failed to set HKDF input keying material");
            return -1;
        }
    }
    if (salt != NULL) {
        rv = hkdf_ctx_set_salt (hkdfp, salt, saltlen);
        if (rv == -1) {
            diag ("Failed to set HKDF salt");
            return -1;
        }
    }
    if (info != NULL) {
        rv = hkdf_ctx_set_info (hkdfp, info, infolen);
        if (rv == -1) {
            diag ("Failed to set HKDF info");
            return -1;
        }
    }
    if (okmlen > sizeof (buf)) {
        diag ("Exceeded %zu-byte buffer with %zu bytes of "
                "HKDF output keying material", sizeof (buf), okmlen);
        return -1;
    }
    buflen = okmlen;
    rv = hkdf (hkdfp, buf, &buflen);
    if (rv == -1) {
        diag ("Failed to compute HKDF key derivation");
        return -1;
    }
    if (buflen != okmlen) {
        diag ("Expected %zu bytes of output keying material, "
                "but got %zu bytes", okmlen, buflen);
        return -1;
    }
    if (memcmp (buf, okm, okmlen) != 0) {
        diag ("Failed to match expected HKDF output keying material");
        return -1;
    }
    hkdf_ctx_destroy (hkdfp);
    return 0;
}


/*  Test Case 1: SHA-256 basic test
 */
int
hkdf_test_1 (void)
{
    const unsigned char ikm[22] = {
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
    };
    const unsigned char salt[13] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
        0x0c
    };
    const unsigned char info[10] = {
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9
    };
    const unsigned char okm[42] = {
        0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a, 0x90, 0x43, 0x4f, 0x64,
        0xd0, 0x36, 0x2f, 0x2a, 0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a, 0x5a, 0x4c,
        0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4, 0xc5, 0xbf, 0x34, 0x00, 0x72, 0x08,
        0xd5, 0xb8, 0x87, 0x18, 0x58, 0x65
    };
    const munge_mac_t md = MUNGE_MAC_SHA256;

    return hkdf_test (md, ikm, sizeof (ikm), salt, sizeof (salt),
            info, sizeof (info), okm, sizeof (okm));
}


/*  Test Case 2: SHA-256 with longer inputs/outputs
 */
int
hkdf_test_2 (void)
{
    const unsigned char ikm[80] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
        0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f
    };
    const unsigned char salt[80] = {
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
        0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
        0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
        0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
        0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf
    };
    const unsigned char info[80] = {
        0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb,
        0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb,
        0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };
    const unsigned char okm[82] = {
        0xb1, 0x1e, 0x39, 0x8d, 0xc8, 0x03, 0x27, 0xa1, 0xc8, 0xe7, 0xf7, 0x8c,
        0x59, 0x6a, 0x49, 0x34, 0x4f, 0x01, 0x2e, 0xda, 0x2d, 0x4e, 0xfa, 0xd8,
        0xa0, 0x50, 0xcc, 0x4c, 0x19, 0xaf, 0xa9, 0x7c, 0x59, 0x04, 0x5a, 0x99,
        0xca, 0xc7, 0x82, 0x72, 0x71, 0xcb, 0x41, 0xc6, 0x5e, 0x59, 0x0e, 0x09,
        0xda, 0x32, 0x75, 0x60, 0x0c, 0x2f, 0x09, 0xb8, 0x36, 0x77, 0x93, 0xa9,
        0xac, 0xa3, 0xdb, 0x71, 0xcc, 0x30, 0xc5, 0x81, 0x79, 0xec, 0x3e, 0x87,
        0xc1, 0x4c, 0x01, 0xd5, 0xc1, 0xf3, 0x43, 0x4f, 0x1d, 0x87
    };
    const munge_mac_t md = MUNGE_MAC_SHA256;

    return hkdf_test (md, ikm, sizeof (ikm), salt, sizeof (salt),
            info, sizeof (info), okm, sizeof (okm));
}


/*  Test Case 3: SHA-256 with zero-length salt/info
 */
int
hkdf_test_3 (void)
{
    const unsigned char ikm[22] = {
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
    };
    const unsigned char salt[1] = {
        0x00
    };
    const unsigned char info[1] = {
        0x00
    };
    const unsigned char okm[42] = {
        0x8d, 0xa4, 0xe7, 0x75, 0xa5, 0x63, 0xc1, 0x8f, 0x71, 0x5f, 0x80, 0x2a,
        0x06, 0x3c, 0x5a, 0x31, 0xb8, 0xa1, 0x1f, 0x5c, 0x5e, 0xe1, 0x87, 0x9e,
        0xc3, 0x45, 0x4e, 0x5f, 0x3c, 0x73, 0x8d, 0x2d, 0x9d, 0x20, 0x13, 0x95,
        0xfa, 0xa4, 0xb6, 0x1a, 0x96, 0xc8
    };
    const munge_mac_t md = MUNGE_MAC_SHA256;

    return hkdf_test (md, ikm, sizeof (ikm), salt, 0, info, 0,
            okm, sizeof (okm));
}


/*  Test Case 4: SHA-1 basic test
 */
int
hkdf_test_4 (void)
{
    const unsigned char ikm[11] = {
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
    };
    const unsigned char salt[13] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
        0x0c
    };
    const unsigned char info[10] = {
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9
    };
    const unsigned char okm[42] = {
        0x08, 0x5a, 0x01, 0xea, 0x1b, 0x10, 0xf3, 0x69, 0x33, 0x06, 0x8b, 0x56,
        0xef, 0xa5, 0xad, 0x81, 0xa4, 0xf1, 0x4b, 0x82, 0x2f, 0x5b, 0x09, 0x15,
        0x68, 0xa9, 0xcd, 0xd4, 0xf1, 0x55, 0xfd, 0xa2, 0xc2, 0x2e, 0x42, 0x24,
        0x78, 0xd3, 0x05, 0xf3, 0xf8, 0x96
    };
    const munge_mac_t md = MUNGE_MAC_SHA1;

    return hkdf_test (md, ikm, sizeof (ikm), salt, sizeof (salt),
            info, sizeof (info), okm, sizeof (okm));
}


/*  Test Case 5: SHA-1 with longer inputs/outputs
 */
int
hkdf_test_5 (void)
{
    const unsigned char ikm[80] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
        0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f
    };
    const unsigned char salt[80] = {
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
        0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
        0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
        0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
        0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf
    };
    const unsigned char info[80] = {
        0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb,
        0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb,
        0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };
    const unsigned char okm[82] = {
        0x0b, 0xd7, 0x70, 0xa7, 0x4d, 0x11, 0x60, 0xf7, 0xc9, 0xf1, 0x2c, 0xd5,
        0x91, 0x2a, 0x06, 0xeb, 0xff, 0x6a, 0xdc, 0xae, 0x89, 0x9d, 0x92, 0x19,
        0x1f, 0xe4, 0x30, 0x56, 0x73, 0xba, 0x2f, 0xfe, 0x8f, 0xa3, 0xf1, 0xa4,
        0xe5, 0xad, 0x79, 0xf3, 0xf3, 0x34, 0xb3, 0xb2, 0x02, 0xb2, 0x17, 0x3c,
        0x48, 0x6e, 0xa3, 0x7c, 0xe3, 0xd3, 0x97, 0xed, 0x03, 0x4c, 0x7f, 0x9d,
        0xfe, 0xb1, 0x5c, 0x5e, 0x92, 0x73, 0x36, 0xd0, 0x44, 0x1f, 0x4c, 0x43,
        0x00, 0xe2, 0xcf, 0xf0, 0xd0, 0x90, 0x0b, 0x52, 0xd3, 0xb4
    };
    const munge_mac_t md = MUNGE_MAC_SHA1;

    return hkdf_test (md, ikm, sizeof (ikm), salt, sizeof (salt),
            info, sizeof (info), okm, sizeof (okm));
}


/*  Test Case 6: SHA-1 with zero-length salt/info
 */
int
hkdf_test_6 (void)
{
    const unsigned char ikm[22] = {
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
    };
    const unsigned char salt[1] = {
        0x00
    };
    const unsigned char info[1] = {
        0x00
    };
    const unsigned char okm[42] = {
        0x0a, 0xc1, 0xaf, 0x70, 0x02, 0xb3, 0xd7, 0x61, 0xd1, 0xe5, 0x52, 0x98,
        0xda, 0x9d, 0x05, 0x06, 0xb9, 0xae, 0x52, 0x05, 0x72, 0x20, 0xa3, 0x06,
        0xe0, 0x7b, 0x6b, 0x87, 0xe8, 0xdf, 0x21, 0xd0, 0xea, 0x00, 0x03, 0x3d,
        0xe0, 0x39, 0x84, 0xd3, 0x49, 0x18
    };
    const munge_mac_t md = MUNGE_MAC_SHA1;

    return hkdf_test (md, ikm, sizeof (ikm), salt, 0, info, 0,
            okm, sizeof (okm));
}


/*  Test Case 7: SHA-1 with no salt and zero-length info
 */
int
hkdf_test_7 (void)
{
    const unsigned char ikm[22] = {
        0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
        0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c
    };
    const unsigned char info[1] = {
        0x00
    };
    const unsigned char okm[42] = {
        0x2c, 0x91, 0x11, 0x72, 0x04, 0xd7, 0x45, 0xf3, 0x50, 0x0d, 0x63, 0x6a,
        0x62, 0xf6, 0x4f, 0x0a, 0xb3, 0xba, 0xe5, 0x48, 0xaa, 0x53, 0xd4, 0x23,
        0xb0, 0xd1, 0xf2, 0x7e, 0xbb, 0xa6, 0xf5, 0xe5, 0x67, 0x3a, 0x08, 0x1d,
        0x70, 0xcc, 0xe7, 0xac, 0xfc, 0x48
    };
    const munge_mac_t md = MUNGE_MAC_SHA1;

    return hkdf_test (md, ikm, sizeof (ikm), NULL, 0, info, 0,
            okm, sizeof (okm));
}


int
main (int argc, char *argv[])
{
    crypto_init ();
    md_init_subsystem ();

    plan (7);

    ok (hkdf_test_1 () == 0,
            "Test Case 1: SHA-256 basic test");

    ok (hkdf_test_2 () == 0,
            "Test Case 2: SHA-256 with longer inputs/outputs");

    ok (hkdf_test_3 () == 0,
            "Test Case 3: SHA-256 with zero-length salt/info");

    ok (hkdf_test_4 () == 0,
            "Test Case 4: SHA-1 basic test");

    ok (hkdf_test_5 () == 0,
            "Test Case 5: SHA-1 with longer inputs/outputs");

    ok (hkdf_test_6 () == 0,
            "Test Case 6: SHA-1 with zero-length salt/info");

    ok (hkdf_test_7 () == 0,
            "Test Case 7: SHA-1 with no salt and zero-length info");

    done_testing ();

    crypto_fini ();

    exit (EXIT_SUCCESS);
}
