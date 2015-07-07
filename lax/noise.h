/*
 * OpenSimplex (Simplectic) Noise in C++.
 *
 * This C++ class ported from C by Tom Lechner, July 2015 based on:
 *
 * Ported by Stephen M. Cameron from Kurt Spencer's java implementation
 * 
 * v1.1 (October 5, 2014)
 * - Added 2D and 4D implementations.
 * - Proper gradient sets for all dimensions, from a
 *   dimensionally-generalizable scheme with an actual
 *   rhyme and reason behind it.
 * - Removed default permutation array in favor of
 *   default seed.
 * - Changed seed-based constructor to be independent
 *   of any particular randomization library, so results
 *   will be the same when ported to other languages.
 *
 *
 * July 5, 2015 by Tom Lechner:
 * This file (and the one it was ported from) licensed as follows:
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org>
 *
 */



#ifndef OPEN_SIMPLEX_NOISE_H__
#define OPEN_SIMPLEX_NOISE_H__
#include <stdint.h>


namespace Laxkit {


//------------------------------- OpenSimplexNoise ------------------------------------------

class OpenSimplexNoise
{
  private:
	int16_t perm[256];
	int16_t permGradIndex3D[256];

	double extrapolate2(int xsb, int ysb, double dx, double dy);
	double extrapolate3(int xsb, int ysb, int zsb, double dx, double dy, double dz);
	double extrapolate4(int xsb, int ysb, int zsb, int wsb, double dx, double dy, double dz, double dw);

  public:
	OpenSimplexNoise(long seed=0);
    void Seed(long seed);
    
    static void Initialize(int do2d, int do3d, int do4d);
    static void Finalize  (int do2d, int do3d, int do4d);
    static int IsInitialized(int which);
        
	void NoiseImage(unsigned char *data, int depth, int width, int height, int feature_size);
        
    double Evaluate(double x, double y);
    double Evaluate(double x, double y, double z);
    double Evaluate(double x, double y, double z, double w);

};

} //namespace Laxkit

#endif

