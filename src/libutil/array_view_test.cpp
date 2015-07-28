/*
  Copyright 2014 Larry Gritz, et al. All Rights Reserved.

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

#include <iostream>

#include "OpenImageIO/strided_ptr.h"
#include "OpenImageIO/array_view.h"
#include "OpenImageIO/image_view.h"
#include "OpenImageIO/unittest.h"

OIIO_NAMESPACE_USING;



void test_array_view ()
{
    static float A[] = { 0, 1, 0, 2, 0, 3, 0, 4, 0, 5 };
    array_view<float> a (A);
    OIIO_CHECK_EQUAL (a.size(), 10);
    OIIO_CHECK_EQUAL (a[0], 0.0f);
    OIIO_CHECK_EQUAL (a[1], 1.0f);
    OIIO_CHECK_EQUAL (a[2], 0.0f);
    OIIO_CHECK_EQUAL (a[3], 2.0f);
    array_view<float>::const_iterator i = a.begin();
    OIIO_CHECK_EQUAL (*i, 0.0f);
    ++i;  OIIO_CHECK_EQUAL (*i, 1.0f);
}



void test_const_strided_ptr ()
{
    static const float A[] = { 0, 1, 0, 2, 0, 3, 0, 4, 0, 5 };

    // Make sure it works with unit stride
    strided_ptr<const float> a (A);
    OIIO_CHECK_EQUAL (*a, 0.0f);
    OIIO_CHECK_EQUAL (a[0], 0.0f);
    OIIO_CHECK_EQUAL (a[1], 1.0f);
    OIIO_CHECK_EQUAL (a[2], 0.0f);
    OIIO_CHECK_EQUAL (a[3], 2.0f);

    // All the other tests are with stride of 2 elements
    a = strided_ptr<const float> (&A[1], 2*sizeof(A[0]));
    OIIO_CHECK_EQUAL (*a, 1.0f);
    OIIO_CHECK_EQUAL (a[0], 1.0f);
    OIIO_CHECK_EQUAL (a[1], 2.0f);
    OIIO_CHECK_EQUAL (a[2], 3.0f);
    OIIO_CHECK_EQUAL (a[3], 4.0f);

    ++a;  OIIO_CHECK_EQUAL (*a, 2.0f);
    a++;  OIIO_CHECK_EQUAL (*a, 3.0f);
    ++a;  OIIO_CHECK_EQUAL (*a, 4.0f);
    --a;  OIIO_CHECK_EQUAL (*a, 3.0f);
    a--;  OIIO_CHECK_EQUAL (*a, 2.0f);
    a += 2;  OIIO_CHECK_EQUAL (*a, 4.0f);
    a -= 2;  OIIO_CHECK_EQUAL (*a, 2.0f);
    a = a + 2;  OIIO_CHECK_EQUAL (*a, 4.0f);
    a = a - 2;  OIIO_CHECK_EQUAL (*a, 2.0f);
}



void test_strided_ptr ()
{
    static float A[] = { 0, 1, 0, 2, 0, 3, 0, 4, 0, 5 };

    // Make sure it works with unit stride
    strided_ptr<float> a (A);
    OIIO_CHECK_EQUAL (*a, 0.0f);
    OIIO_CHECK_EQUAL (a[0], 0.0f);
    OIIO_CHECK_EQUAL (a[1], 1.0f);
    OIIO_CHECK_EQUAL (a[2], 0.0f);
    OIIO_CHECK_EQUAL (a[3], 2.0f);

    // All the other tests are with stride of 2 elements
    a = strided_ptr<float> (&A[1], 2*sizeof(A[0]));
    OIIO_CHECK_EQUAL (*a, 1.0f);
    OIIO_CHECK_EQUAL (a[0], 1.0f);
    OIIO_CHECK_EQUAL (a[1], 2.0f);
    OIIO_CHECK_EQUAL (a[2], 3.0f);
    OIIO_CHECK_EQUAL (a[3], 4.0f);

    ++a;  OIIO_CHECK_EQUAL (*a, 2.0f);
    a++;  OIIO_CHECK_EQUAL (*a, 3.0f);
    ++a;  OIIO_CHECK_EQUAL (*a, 4.0f);
    --a;  OIIO_CHECK_EQUAL (*a, 3.0f);
    a--;  OIIO_CHECK_EQUAL (*a, 2.0f);
    a += 2;  OIIO_CHECK_EQUAL (*a, 4.0f);
    a -= 2;  OIIO_CHECK_EQUAL (*a, 2.0f);
    a = a + 2;  OIIO_CHECK_EQUAL (*a, 4.0f);
    a = a - 2;  OIIO_CHECK_EQUAL (*a, 2.0f);

    *a = 14.0; OIIO_CHECK_EQUAL (*a, 14.0f);
}



void test_array_view_strided ()
{
    static const float A[] = { 0, 1, 0, 2, 0, 3, 0, 4, 0, 5 };
    array_view_strided<const float> a (&A[1], 5, 2*sizeof(float));
    OIIO_CHECK_EQUAL (a.size(), 5);
    OIIO_CHECK_EQUAL (a[0], 1.0f);
    OIIO_CHECK_EQUAL (a[1], 2.0f);
    OIIO_CHECK_EQUAL (a[2], 3.0f);
    OIIO_CHECK_EQUAL (a[3], 4.0f);
    array_view_strided<const float>::const_iterator i = a.begin();
    OIIO_CHECK_EQUAL (*i, 1.0f);
    ++i;  OIIO_CHECK_EQUAL (*i, 2.0f);

}



void test_array_view_strided_mutable ()
{
    static float A[] = { 0, 1, 0, 2, 0, 3, 0, 4, 0, 5 };
    array_view_strided<float> a (&A[1], 5, 2*sizeof(float));
    OIIO_CHECK_EQUAL (a.size(), 5);
    OIIO_CHECK_EQUAL (a[0], 1.0f);
    OIIO_CHECK_EQUAL (a[1], 2.0f);
    OIIO_CHECK_EQUAL (a[2], 3.0f);
    OIIO_CHECK_EQUAL (a[3], 4.0f);
    array_view_strided<float>::iterator i = a.begin();
    OIIO_CHECK_EQUAL (*i, 1.0f);
    ++i;  OIIO_CHECK_EQUAL (*i, 2.0f);

}



void test_image_view ()
{
    const int X = 4, Y = 3, C = 3, Z = 1;
    static const float IMG[Z][Y][X][C] = {   // 4x3 2D image with 3 channels
        {{{0,0,0},  {1,0,1},  {2,0,2},  {3,0,3}},
         {{0,1,4},  {1,1,5},  {2,1,6},  {3,1,7}},
         {{0,2,8},  {1,2,9},  {2,2,10}, {3,2,11}}}
    };

    image_view<const float> I ((const float *)IMG, 3, 4, 3);
    for (int y = 0, i = 0;  y < Y;  ++y) {
        for (int x = 0;  x < X;  ++x, ++i) {
            OIIO_CHECK_EQUAL (I(x,y)[0], x);
            OIIO_CHECK_EQUAL (I(x,y)[1], y);
            OIIO_CHECK_EQUAL (I(x,y)[2], i);
        }
    }
}



void test_image_view_mutable ()
{
    const int X = 4, Y = 3, C = 3, Z = 1;
    static float IMG[Z][Y][X][C] = {   // 4x3 2D image with 3 channels
        {{{0,0,0},  {0,0,0},  {0,0,0},  {0,0,0}},
         {{0,0,0},  {0,0,0},  {0,0,0},  {0,0,0}},
         {{0,0,0},  {0,0,0},  {0,0,0},  {0,0,0}}}
    };

    image_view<float> I ((float *)IMG, 3, 4, 3);
    for (int y = 0, i = 0;  y < Y;  ++y) {
        for (int x = 0;  x < X;  ++x, ++i) {
            I(x,y)[0] = x;
            I(x,y)[1] = y;
            I(x,y)[2] = i;
        }
    }

    for (int y = 0, i = 0;  y < Y;  ++y) {
        for (int x = 0;  x < X;  ++x, ++i) {
            OIIO_CHECK_EQUAL (I(x,y)[0], x);
            OIIO_CHECK_EQUAL (I(x,y)[1], y);
            OIIO_CHECK_EQUAL (I(x,y)[2], i);
        }
    }
}



int main (int argc, char *argv[])
{
    test_array_view ();
    test_const_strided_ptr ();
    test_strided_ptr ();
    test_array_view_strided ();
    test_array_view_strided_mutable ();
    test_image_view ();
    test_image_view_mutable ();

    return unit_test_failures;
}

