/*
Light editng of Raph Levien's spiro curve to work within Laxkit by Tom Lechner, 2019.
This file released under same terms as the original: your choice of Apache 2 or MIT.

Original license follows:
ppedit - A pattern plate editor for Spiro splines.
Copyright (C) 2007 Raph Levien

Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
<LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
option. This file may not be copied, modified, or distributed
except according to those terms.

*/

#pragma once


namespace Laxkit {


struct bezctx
{
    void (*moveto)(bezctx *bc, double x, double y, int is_open);
    void (*lineto)(bezctx *bc, double x, double y);
    void (*quadto)(bezctx *bc, double x1, double y1, double x2, double y2);
    void (*curveto)(bezctx *bc, double x1, double y1, double x2, double y2, double x3, double y3);
    void (*mark_knot)(bezctx *bc, int knot_idx);
};

 /* Possible values of the "ty" field. */
enum class SPIRO : char {
	CORNER =  'v',
	G4 =      'o',
	G2 =      'c',
	LEFT =    '[',
	RIGHT =   ']',
	ANCHOR =  'a',
	HANDLE =  'h',
     /* For a closed contour add an extra cp with a ty set to */
	END =     'z',
     /* For an open contour the first cp must have a ty set to*/
	OPEN_CONTOUR =  '{',
     /* For an open contour the last cp must have a ty set to */
	END_OPEN_CONTOUR =  '}'
};

typedef struct
{
    double x;
    double y;
    char ty;
} spiro_cp;

typedef struct spiro_seg_s spiro_seg;

spiro_seg *
run_spiro(const spiro_cp *src, int n);

void
free_spiro(spiro_seg *s);

void
spiro_to_bpath(const spiro_seg *s, int n, bezctx *bc);

double get_knot_th(const spiro_seg *s, int i);

void bezctx_moveto(bezctx *bc, double x, double y, int is_open);
void bezctx_lineto(bezctx *bc, double x, double y);
void bezctx_quadto(bezctx *bc, double x1, double y1, double x2, double y2);
void bezctx_curveto(bezctx *bc, double x1, double y1, double x2, double y2, double x3, double y3);
void bezctx_mark_knot(bezctx *bc, int knot_idx);


} //namespace Laxkit

