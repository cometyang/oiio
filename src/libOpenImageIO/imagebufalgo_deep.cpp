/*
  Copyright 2013 Larry Gritz and the other authors and contributors.
  All Rights Reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the software's owners nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  (This is the Modified BSD License)
*/


#include <boost/bind.hpp>
#include <OpenEXR/half.h>

#include <cmath>
#include <iostream>
#include <stdexcept>

#include "OpenImageIO/imagebuf.h"
#include "OpenImageIO/imagebufalgo.h"
#include "OpenImageIO/imagebufalgo_util.h"
#include "OpenImageIO/dassert.h"
#include "OpenImageIO/thread.h"


OIIO_NAMESPACE_ENTER
{


// Helper for flatten: identify channels in the spec that are important to
// deciphering deep images. Return true if appropriate alphas were found.
static bool
find_deep_channels (const ImageSpec &spec, int &alpha_channel,
                    int &RA_channel, int &GA_channel, int &BA_channel,
                    int &R_channel, int &G_channel, int &B_channel,
                    int &Z_channel, int &Zback_channel)
{
    static const char *names[] = { "A", "RA", "GA", "BA",
                                   "R", "G", "B", "Z", "Zback", NULL };
    int *chans[] = { &alpha_channel, &RA_channel, &GA_channel, &BA_channel,
                     &R_channel, &G_channel, &B_channel, 
                     &Z_channel, &Zback_channel };
    for (int i = 0;  names[i];  ++i)
        *chans[i] = -1;
    for (int c = 0, e = int(spec.channelnames.size()); c < e; ++c) {
        for (int i = 0;  names[i];  ++i) {
            if (spec.channelnames[c] == names[i]) {
                *chans[i] = c;
                break;
            }
        }
    }
    if (Zback_channel < 0)
        Zback_channel = Z_channel;
    return (alpha_channel >= 0 ||
            (RA_channel >= 0 && GA_channel >= 0 && BA_channel >= 0));
}



// FIXME -- NOT CORRECT!  This code assumes sorted, non-overlapping samples.
// That is not a valid assumption in general. We will come back to fix this.
template<class DSTTYPE>
static bool
flatten_ (ImageBuf &dst, const ImageBuf &src, 
          ROI roi, int nthreads)
{
    if (nthreads != 1 && roi.npixels() >= 1000) {
        // Possible multiple thread case -- recurse via parallel_image
        ImageBufAlgo::parallel_image (
            boost::bind(flatten_<DSTTYPE>, boost::ref(dst), boost::cref(src),
                        _1 /*roi*/, 1 /*nthreads*/),
            roi, nthreads);
        return true;
    }

    const ImageSpec &srcspec (src.spec());
    int nc = srcspec.nchannels;

    int alpha_channel, RA_channel, GA_channel, BA_channel;
    int R_channel, G_channel, B_channel;
    int Z_channel, Zback_channel;
    if (! find_deep_channels (srcspec, alpha_channel,
                              RA_channel, GA_channel, BA_channel,
                              R_channel, G_channel, B_channel,
                              Z_channel, Zback_channel)) {
        dst.error ("No alpha channel could be identified");
        return false;
    }
    ASSERT (alpha_channel >= 0 ||
            (RA_channel >= 0 && GA_channel >= 0 && BA_channel >= 0));
    float *val = ALLOCA (float, nc);
    float &RAval (RA_channel >= 0 ? val[RA_channel] : val[alpha_channel]);
    float &GAval (GA_channel >= 0 ? val[GA_channel] : val[alpha_channel]);
    float &BAval (BA_channel >= 0 ? val[BA_channel] : val[alpha_channel]);

    for (ImageBuf::Iterator<DSTTYPE> r (dst, roi);  !r.done();  ++r) {
        int x = r.x(), y = r.y(), z = r.z();
        int samps = src.deep_samples (x, y, z);
        // Clear accumulated values for this pixel (0 for colors, big for Z)
        memset (val, 0, nc*sizeof(float));
        if (Z_channel >= 0 && samps == 0)
            val[Z_channel] = 1.0e30;
        if (Zback_channel >= 0 && samps == 0)
            val[Zback_channel] = 1.0e30;
        for (int s = 0;  s < samps;  ++s) {
            float RA = RAval, GA = GAval, BA = BAval;  // make copies
            float alpha = (RA + GA + BA) / 3.0f;
            if (alpha >= 1.0f)
                break;
            for (int c = 0;  c < nc;  ++c) {
                float v = src.deep_value (x, y, z, c, s);
                if (c == Z_channel || c == Zback_channel)
                    val[c] *= alpha;  // because Z are not premultiplied
                float a;
                if (c == R_channel)
                    a = RA;
                else if (c == G_channel)
                    a = GA;
                else if (c == B_channel)
                    a = BA;
                else
                    a = alpha;
                val[c] += (1.0f - a) * v;
            }
        }

        for (int c = roi.chbegin;  c < roi.chend;  ++c)
            r[c] = val[c];
    }

    return true;
}


bool
ImageBufAlgo::flatten (ImageBuf &dst, const ImageBuf &src,
                       ROI roi, int nthreads)
{
    if (! src.deep()) {
        // For some reason, we were asked to flatten an already-flat image.
        // So just copy it.
        return dst.copy (src);
    }

    // Construct an ideal spec for dst, which is like src but not deep.
    ImageSpec force_spec = src.spec();
    force_spec.deep = false;
    force_spec.channelformats.clear();

    if (! IBAprep (roi, &dst, &src, NULL, &force_spec, IBAprep_SUPPORT_DEEP))
        return false;
    if (dst.spec().deep) {
        dst.error ("Cannot flatten to a deep image");
        return false;
    }

    const ImageSpec &srcspec (src.spec());
    int alpha_channel, RA_channel, GA_channel, BA_channel;
    int R_channel, G_channel, B_channel, Z_channel, Zback_channel;
    if (! find_deep_channels (srcspec, alpha_channel,
                              RA_channel, GA_channel, BA_channel,
                              R_channel, G_channel, B_channel,
                              Z_channel, Zback_channel)) {
        dst.error ("No alpha channel could be identified");
        return false;
    }

    bool ok;
    OIIO_DISPATCH_TYPES (ok, "flatten", flatten_, dst.spec().format,
                         dst, src, roi, nthreads);
    return ok;
}



bool
ImageBufAlgo::deepen (ImageBuf &dst, const ImageBuf &src, float zvalue,
                      ROI roi, int nthreads)
{
    if (src.deep()) {
        // For some reason, we were asked to deepen an already-deep image.
        // So just copy it.
        return dst.copy (src);
        // FIXME: once paste works for deep files, this should really be
        // return paste (dst, roi.xbegin, roi.ybegin, roi.zbegin, roi.chbegin,
        //               src, roi, nthreads);
    }

    // Construct an ideal spec for dst, which is like src but deep.
    const ImageSpec &srcspec (src.spec());
    int nc = srcspec.nchannels;
    int zback_channel = -1;
    ImageSpec force_spec = srcspec;
    force_spec.deep = true;
    force_spec.set_format (TypeDesc::FLOAT);
    force_spec.channelformats.clear();
    for (int c = 0; c < nc; ++c) {
        if (force_spec.channelnames[c] == "Z")
            force_spec.z_channel = c;
        else if (force_spec.channelnames[c] == "Zback")
            zback_channel = c;
    }
    bool add_z_channel = (force_spec.z_channel < 0);
    if (add_z_channel) {
        // No z channel? Make one.
        force_spec.z_channel = force_spec.nchannels++;
        force_spec.channelnames.push_back ("Z");
    }

    if (! IBAprep (roi, &dst, &src, NULL, &force_spec, IBAprep_SUPPORT_DEEP))
        return false;
    if (! dst.deep()) {
        dst.error ("Cannot deepen to a flat image");
        return false;
    }

    float *pixel = OIIO_ALLOCA (float, nc);

    // First, figure out which pixels get a sample and which do not
    for (int z = roi.zbegin; z < roi.zend; ++z)
    for (int y = roi.ybegin; y < roi.yend; ++y)
    for (int x = roi.xbegin; x < roi.xend; ++x) {
        bool has_sample = false;
        src.getpixel (x, y, z, pixel);
        for (int c = 0; c < nc; ++c)
            if (c != force_spec.z_channel && c != zback_channel
                  && pixel[c] != 0.0f) {
                has_sample = true;
                break;
            }
        if (! has_sample && ! add_z_channel)
            for (int c = 0; c < nc; ++c)
                if ((c == force_spec.z_channel || c == zback_channel)
                    && (pixel[c] != 0.0f && pixel[c] < 1e30)) {
                    has_sample = true;
                    break;
                }
        if (has_sample)
            dst.set_deep_samples (x, y, z, 1);
    }

    dst.deep_alloc ();

    // Now actually set the values
    for (int z = roi.zbegin; z < roi.zend; ++z)
    for (int y = roi.ybegin; y < roi.yend; ++y)
    for (int x = roi.xbegin; x < roi.xend; ++x) {
        if (dst.deep_samples (x, y, z) == 0)
            continue;
        for (int c = 0; c < nc; ++c)
            dst.set_deep_value (x, y, z, c, 0 /*sample*/,
                                src.getchannel (x, y, z, c));
        if (add_z_channel)
            dst.set_deep_value (x, y, z, nc, 0, zvalue);
    }

    bool ok = true;
    // FIXME -- the above doesn't split into threads. Someday, it should
    // be refactored like this:
    // OIIO_DISPATCH_COMMON_TYPES2 (ok, "deepen", deepen_,
    //                              dst.spec().format, srcspec.format,
    //                              dst, src, add_z_channel, z, roi, nthreads);
    return ok;
}



}
OIIO_NAMESPACE_EXIT
