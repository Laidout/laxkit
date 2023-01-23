//
//
//    The Laxkit, a windowing toolkit
//    Please consult https://github.com/Laidout/laxkit about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 3 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; If not, see <http://www.gnu.org/licenses/>.
//
//    Copyright (C) 2016 by Tom Lechner
//
//    The marked parts below are adapted from https://github.com/sloisel/numeric
//    and https://github.com/jlouthan/perspective-transform,
//    both MIT licensed.
//


#include <lax/interfaces/pathinterface.h>


#include <lax/interfaces/perspectiveinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>


using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG


namespace LaxInterfaces {



//---------------------------------- Perspective Transform Utils -----------------------------


//--------------- class Matrix (not used yet) -------------------
class Matrix
{
  public:
	int rows, cols;
	double **d;

	Matrix(const Matrix &matrix);
	Matrix(int nrows, int ncols);
	~Matrix();

	void Allocate(int nrows, int ncols);
	void Identity();
	void Transpose();
	void Invert();

	double m(int r, int c) { return d[r][c]; }
	double m(int r, int c, double nvalue) { return d[r][c]=nvalue; }
};

Matrix::Matrix(const Matrix &matrix)
{
	rows=matrix.rows;
	cols=matrix.cols;

	d=new double*[rows];
	for (int r=0; r<rows; r++) {
		d[r]=new double[cols];
		for (int c=0; c<cols; c++) {
			d[r][c]=matrix.d[r][c];
		}
	}
}

Matrix::Matrix(int nrows, int ncols)
{
	rows=nrows;
	cols=ncols;

	d=new double*[nrows];
	for (int r=0; r<rows; r++) d[r]=new double[cols];
}

Matrix::~Matrix()
{
	for (int r=0; r<rows; r++) delete[] d[r];
	delete[] d;
}

void Matrix::Identity()
{
	for (int r=0; r<rows; r++) {
		for (int c=0; c<cols; c++) {
			if (r==c) d[r][c]=1;
			else d[r][c]=0;
		}
	}
}

void Matrix::Allocate(int nrows, int ncols)
{
	if (nrows==rows && cols==ncols) return;

	for (int r=0; r<rows; r++) delete[] d[r];
	delete[] d;

	d=new double*[nrows];
	for (int r=0; r<rows; r++) d[r]=new double[cols];
}

//--------------- end class Matrix -------------------


//The following Numeric* are adapted from numeric.js:
//  https://github.com/sloisel/numeric
//MIT license:
//
// Numeric Javascript
// Copyright (C) 2011 by Sébastien Loisel
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

double **AllocateMatrix(int nrows, int ncols)
{
	double **d;

	d=new double*[nrows];
	for (int r=0; r<nrows; r++) d[r]=new double[ncols];

	return d;
}

void DeallocateMatrix(int rows, double **d)
{
	if (!d) return;
	for (int r=0; r<rows; r++) delete[] d[r];
	delete[] d;
}

/*! Copy a vector. Returns ret.
 */
double *Numeric_cloneV(int n, double *x, double *ret)
{
	if (!ret) ret=new double[n];

	for (int i=n-1; i!=-1; --i) {
		ret[i] = (x[i]);
	}
	return ret;
}

/*! Copy matrix
 */
double **Numeric_clone(int rows, int cols, double **x, double**ret)
{
	if (!ret) ret=AllocateMatrix(rows,cols);

	for (int r=0; r<rows; r++) {
		for (int c=0; c<cols; c++) {
			ret[r][c]=x[r][c];
		}
	}

	return ret;
}

/*! Make diagonal matrix.
 */
double **Numeric_diag(int n, double *d, double **ret)
{
	if (!ret) ret=AllocateMatrix(n,n);

	for (int r=0; r<n; r++) {
		for (int c=0; c<n; c++) {
			if (r==c) ret[r][c]=d[r];
			else ret[r][c]=0;
		}
	}

	return ret;
}

/*! Create identity matrix.
 */
double **Numeric_identity(int n, double **d)
{
	if (!d) d=AllocateMatrix(n,n);

	for (int r=0; r<n; r++) {
		for (int c=0; c<n; c++) {
			if (r==c) d[r][c]=1;
			else d[r][c]=0;
		}
	}

	return d;
}

/*! Compute inverse of matrix.
 * Returns a newly AllocateMatrix()'d array.
 */
double **Numeric_inv(int rows, int cols, double **a)
{
	int m=rows, n=cols;
	double **A = AllocateMatrix(m,n);
	Numeric_clone(m,n, a, A);
	double *Ai, *Aj;

	double **I = AllocateMatrix(m,m);
	Numeric_identity(m, I);
	double *Ii, *Ij;
	int i,j,k;
	double x;

	for (j=0; j<n; ++j) {
		int i0 = -1;
		int v0 = -1;

		for(i=j; i!=m; ++i) {
			k = fabs(A[i][j]);
			if(k>v0) { i0 = i; v0 = k; }
		}

		if (i0<0) throw(1);

		Aj = A[i0]; A[i0] = A[j]; A[j] = Aj;
		Ij = I[i0]; I[i0] = I[j]; I[j] = Ij;
		x = Aj[j];

		for (k=j; k!=n; ++k)    Aj[k] /= x;
		for (k=n-1; k!=-1; --k) Ij[k] /= x;
		for (i=m-1; i!=-1; --i) {
			if (i!=j) {
				Ai = A[i];
				Ii = I[i];
				x = Ai[j];
				for (k=j+1; k!=n; ++k)  Ai[k] -= Aj[k]*x;
				for (k=n-1; k>0; --k) { Ii[k] -= Ij[k]*x; --k; Ii[k] -= Ij[k]*x; }
				if (k==0) Ii[0] -= Ij[0]*x;
			}
		}
	}

	DeallocateMatrix(m, A);

	return I;
}

/*! Matrix times matrix.
 * ret should be double[rows of x][cols of y].
 * If ret==NULL, return a new double**.
 */
double **Numeric_dotMMsmall(int numxrows, int numxcols, int numycols, double **x, double **y, double **ret)
{
	int i,j,k,p,q,r;
	double *foo, *bar;
	double woo;
	int i0;

	p = numxrows; q = numxcols; r = numycols;
	double **rret=NULL;
	if (!ret) rret = new double*[p];

	for (i=p-1; i>=0; i--) {
		if (ret) foo=ret[i];
		else foo = new double[r];

		bar = x[i];
		for (k=r-1; k>=0; k--) {
			woo = bar[q-1]*y[q-1][k];
			for (j=q-2; j>=1; j-=2) {
				i0 = j-1;
				woo += bar[j]*y[j][k] + bar[i0]*y[i0][k];
			}
			if (j==0) { woo += bar[0]*y[0][k]; }
			foo[k] = woo;
		}

		if (rret) rret[i] = foo;
	}

	if (ret) return ret;
	return rret;
}

/*! Vector dot product.
 */
double Numeric_dotVV(int n, double *x, double *y)
{
	double ret=0;
	for(int i=0; i<n; i++) {
		ret += x[i]*y[i];
	}
	return ret;
};

/*! Matrix times column v = column v. Just returns ret. If ret==NULL, return new double[rows].
 */
double *Numeric_dotMV(int rows,int cols, double **x, double *y, double *ret)
{
	if (!ret) ret=new double[rows];

	for (int i=0; i<rows; i++) {
		ret[i] = Numeric_dotVV(cols, x[i],y);
	}
	return ret;
};

/*! Matrix transpose. ret should be double[cols][rows], and is returned new if ret==NULL.
 */
double **Numeric_transpose(int rows, int cols, double **x, double **ret)
{
	if (ret==NULL) ret=AllocateMatrix(cols, rows);

	for (int r=0; r<cols; r++) {
		for (int c=0; c<rows; c++) {
			ret[r][c] = x[c][r];
		}
	}

	return ret;
}


//----------------------------- PerspectiveTransform ------------------------------

/*! \class PerspectiveTransform
 *
 * Holds info about transforming vie perspective by matching 4 points before and after.
 * The original 4 don't have to be rectangular.
 */

PerspectiveTransform::PerspectiveTransform()
{
	memset(srcPts, 0, 8*sizeof(double));
	memset(dstPts, 0, 8*sizeof(double));

	from_ll.set(0,0);
	from_lr.set(1,0);
	from_ul.set(0,1);
	from_ur.set(1,1);

	ResetTransform();
}

PerspectiveTransform::PerspectiveTransform(flatpoint *nsrcPts, flatpoint *ndstPts)
{
	SetPoints(nsrcPts, ndstPts);
}

int PerspectiveTransform::SetPoints(flatpoint *nsrcPts, flatpoint *ndstPts)
{
	for (int c=0; c<4; c++) {
		srcPts[2*c  ] = nsrcPts[c].x;
		srcPts[2*c+1] = nsrcPts[c].y;
		dstPts[2*c  ] = ndstPts[c].x;
		dstPts[2*c+1] = ndstPts[c].y;
	}

	ComputeTransform();
	return 0;
}

void PerspectiveTransform::ResetTransform()
{
	to_ll = from_ll;
	to_lr = from_lr;
	to_ul = from_ul;
	to_ur = from_ur;

	coeffs[0]=coeffs[4]=coeffs[8]=1;
	coeffs[1]=coeffs[2]=coeffs[3]=coeffs[5]=coeffs[6]=coeffs[7]=0;

	coeffsInv[0]=coeffsInv[4]=coeffsInv[8]=1;
	coeffsInv[1]=coeffsInv[2]=coeffsInv[3]=coeffsInv[5]=coeffsInv[6]=coeffsInv[7]=0;
}

int PerspectiveTransform::SetFrom(flatpoint nfrom_ll, flatpoint nfrom_lr, flatpoint nfrom_ul, flatpoint nfrom_ur)
{
	from_ll=nfrom_ll;
	from_lr=nfrom_lr;
	from_ul=nfrom_ul;
	from_ur=nfrom_ur;

	return IsValid();
}

int PerspectiveTransform::SetTo(flatpoint nto_ll, flatpoint nto_lr, flatpoint nto_ul, flatpoint nto_ur)
{
	to_ll=nto_ll;
	to_lr=nto_lr;
	to_ul=nto_ul;
	to_ur=nto_ur;

	return IsValid();
}

/*! Basically valid when the 4 points form a 4 point convex shape.
 */
bool PerspectiveTransform::IsValid()
{
	flatpoint pts[3], p;

	for (int c=0; c<4; c++) {
		p.set(srcPts[2*c], srcPts[2*c+1]);
		for (int c2=0, i=0; c2<4; c2++) {
			if (c2==c) continue;
			pts[i++].set(srcPts[2*c2], srcPts[2*c2+1]);
		}
		if (point_is_in(p, pts, 3)) return false;

		p.set(dstPts[2*c], dstPts[2*c+1]);
		for (int c2=0, i=0; c2<4; c2++) {
			if (c2==c) continue;
			pts[i++].set(dstPts[2*c2], dstPts[2*c2+1]);
		}
		if (point_is_in(p, pts, 3)) return false;
	}

	return true;
}

void PerspectiveTransform::ComputeTransform()
{
	srcPts[0] = from_ll.x;
	srcPts[1] = from_ll.y;
	srcPts[2] = from_lr.x;
	srcPts[3] = from_lr.y;
	srcPts[4] = from_ul.x;
	srcPts[5] = from_ul.y;
	srcPts[6] = from_ur.x;
	srcPts[7] = from_ur.y;

	dstPts[0] =  to_ll.x;
	dstPts[1] =  to_ll.y;
	dstPts[2] =  to_lr.x;
	dstPts[3] =  to_lr.y;
	dstPts[4] =  to_ul.x;
	dstPts[5] =  to_ul.y;
	dstPts[6] =  to_ur.x;
	dstPts[7] =  to_ur.y;

	getNormalizationCoefficients(srcPts, dstPts, false);
	getNormalizationCoefficients(srcPts, dstPts, true);

	DBG cerr << "PerspectiveTransform::ComputeTransform():"<<endl;
	DBG //cerr << "coeffs:"<<endl << std::setprecision(3) << fixed; //note: needs #include <iomanip>
	DBG cerr << "coeffs:"<<endl;
	DBG for (int c=0; c<9; c++) { cerr  << "  "<<coeffs[c]<<','; if (c%3==2) cerr <<endl; }
	DBG cerr << "coeffsInv:"<<endl;
	DBG for (int c=0; c<9; c++) { cerr <<"  "<<coeffsInv[c]<<','; if (c%3==2) cerr <<endl; }
}

//The following perspective transform implementation is adapted from:
//https://github.com/jlouthan/perspective-transform
//
//The MIT License (MIT)
//
//Copyright (c) 2015 Jenny Louthan
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

double *PerspectiveTransform::getNormalizationCoefficients(double *src, double *dst, bool isInverse)
{
	double *matX;

	if(isInverse){
		matX=coeffsInv;
		double *tmp = dst;
		dst = src;
		src = tmp;
	} else {
		matX=coeffs;
	}

	double r[8][8] = {
		{src[0], src[1], 1, 0, 0, 0, -1*dst[0]*src[0], -1*dst[0]*src[1]},
		{0, 0, 0, src[0], src[1], 1, -1*dst[1]*src[0], -1*dst[1]*src[1]},
		{src[2], src[3], 1, 0, 0, 0, -1*dst[2]*src[2], -1*dst[2]*src[3]},
		{0, 0, 0, src[2], src[3], 1, -1*dst[3]*src[2], -1*dst[3]*src[3]},
		{src[4], src[5], 1, 0, 0, 0, -1*dst[4]*src[4], -1*dst[4]*src[5]},
		{0, 0, 0, src[4], src[5], 1, -1*dst[5]*src[4], -1*dst[5]*src[5]},
		{src[6], src[7], 1, 0, 0, 0, -1*dst[6]*src[6], -1*dst[6]*src[7]},
		{0, 0, 0, src[6], src[7], 1, -1*dst[7]*src[6], -1*dst[7]*src[7]}
	};

	double **matA = AllocateMatrix(8,8);
	for (int i=0; i<8; i++) {
		memcpy(matA[i], r[i], sizeof(double[8]));
	}

	//double *matB = dstPts;
	double *matB = dst;
	double **matC=NULL;
	double **Atranspose = Numeric_transpose(8,8, matA, NULL);

	int err=0;

	try {
		double **tempmat = Numeric_dotMMsmall(8,8,8, Atranspose, matA, NULL);
		matC = Numeric_inv(8,8, tempmat);
		DeallocateMatrix(8, tempmat);

	} catch(exception &e) {
		err=1;
		cerr << "PerspectiveTransform Matrix invert error! "<<e.what()<<endl;
		matX[0]=matX[4]=matX[8]=1;
		matX[1]=matX[2]=matX[3]=matX[5]=matX[6]=matX[7]=0;

	} catch(int e) {
		err=1;
		cerr << "PerspectiveTransform Matrix invert error! "<<e<<endl;
		matX[0]=matX[4]=matX[8]=1;
		matX[1]=matX[2]=matX[3]=matX[5]=matX[6]=matX[7]=0;
	}

	if (!err) {
		double **matD = Numeric_dotMMsmall(8,8,8, matC, Atranspose, NULL);
		Numeric_dotMV(8,8, matD, matB, matX);
		DeallocateMatrix(8, matD);

		//for(var i = 0; i < matX.length; i++) {
		//    matX[i] = round(matX[i]);
		//}

		matX[8] = 1;
	}

	DeallocateMatrix(8, Atranspose);
	DeallocateMatrix(8, matC);
	return matX;
}

flatpoint PerspectiveTransform::transform(double x,double y)
{
	flatpoint p;
	p.x = (this->coeffs[0]*x + this->coeffs[1]*y + this->coeffs[2]) / (this->coeffs[6]*x + this->coeffs[7]*y + 1);
	p.y = (this->coeffs[3]*x + this->coeffs[4]*y + this->coeffs[5]) / (this->coeffs[6]*x + this->coeffs[7]*y + 1);
	return p;
}

flatpoint PerspectiveTransform::transform(flatpoint p)
{
	flatpoint pp;
	pp.x = (this->coeffs[0]*p.x + this->coeffs[1]*p.y + this->coeffs[2]) / (this->coeffs[6]*p.x + this->coeffs[7]*p.y + 1);
	pp.y = (this->coeffs[3]*p.x + this->coeffs[4]*p.y + this->coeffs[5]) / (this->coeffs[6]*p.x + this->coeffs[7]*p.y + 1);
	return pp;
}

flatpoint PerspectiveTransform::transformInverse(double x,double y)
{
	flatpoint p;
	p.x = (this->coeffsInv[0]*x + this->coeffsInv[1]*y + this->coeffsInv[2]) / (this->coeffsInv[6]*x + this->coeffsInv[7]*y + 1);
	p.y = (this->coeffsInv[3]*x + this->coeffsInv[4]*y + this->coeffsInv[5]) / (this->coeffsInv[6]*x + this->coeffsInv[7]*y + 1);
	return p;
}

flatpoint PerspectiveTransform::transformInverse(flatpoint p)
{
	flatpoint pp;
	pp.x = (this->coeffsInv[0]*p.x + this->coeffsInv[1]*p.y + this->coeffsInv[2]) / (this->coeffsInv[6]*p.x + this->coeffsInv[7]*p.y + 1);
	pp.y = (this->coeffsInv[3]*p.x + this->coeffsInv[4]*p.y + this->coeffsInv[5]) / (this->coeffsInv[6]*p.x + this->coeffsInv[7]*p.y + 1);
	return pp;
}

/*! Transform an image or reverse transform(if direction==-1).
 * This will map images to bounding boxes of the corner control points.
 *
 * Return 0 for success, or 1 if transform is invalid.
 *
 * Note: only works on 8 bit bgra for now.
 */
int PerspectiveTransform::MapImage(SomeData *obj, LaxImage *initial, LaxImage *persped, int direction) //todo: , int oversample)
{
	if (!IsValid()) return 1;

	DoubleBBox box1, box2;
	for (int c=0; c<4; c++) {
		//box1.addtobounds(srcPts[2*c], srcPts[2*c+1]);
		box2.addtobounds(dstPts[2*c], dstPts[2*c+1]);
	}
	box1.setbounds(obj);

	unsigned char *buffer1 = initial->getImageBuffer(); //bgra
	unsigned char *buffer2 = persped->getImageBuffer();

	int iw = initial->w(), ih = initial->h();
	int pw = persped->w(), ph = persped->h();

	double xx,yy, px,py;
	int iix,iiy;
	double box1w = box1.boxwidth(), box1h = box1.boxheight();
	double box2w = box2.boxwidth(), box2h = box2.boxheight();
	int i1, i2;
	flatpoint o;

	if (direction == -1) {
		 //map persped back to initial

	} else {
		 //map initial to persped
		for (int y = 0; y<ph; y++) {
			for (int x = 0; x<pw; x++) {
				 // *** note: this can skip 4*, 4/, 4+- by constructing a more direct persp transform
				//map persp image space to real space
				px = x*box2w/pw + box2.minx;
				py = y*box2h/ph + box2.miny;

				//perspective inverse transform
				xx = (this->coeffsInv[0]*px + this->coeffsInv[1]*py + this->coeffsInv[2]) / (this->coeffsInv[6]*px + this->coeffsInv[7]*py + 1);
				yy = (this->coeffsInv[3]*px + this->coeffsInv[4]*py + this->coeffsInv[5]) / (this->coeffsInv[6]*px + this->coeffsInv[7]*py + 1);

				// xx,yy now in obj parent space
				o = obj->transformPointInverse(flatpoint(xx,yy));
				xx = o.x;
				yy = o.y;

				//map real space to initial image space, which is preview image that
				//fits snuggly in the bounding box of the object
				iix = (xx - box1.minx)*iw/box1w;
				iiy = (yy - box1.miny)*ih/box1h;

				i2 = 4*((ph-y-1)*pw + x);
				//i2 = 4*(y*pw + x);

				if (iix>=0 && iix<iw && iiy>=0 && iiy<ih) {
					i1 = 4*((ih-iiy-1)*iw + iix);
					//i1 = 4*(iiy*iw + iix);
					buffer2[i2  ] = buffer1[i1  ];
					buffer2[i2+1] = buffer1[i1+1];
					buffer2[i2+2] = buffer1[i1+2];
					buffer2[i2+3] = buffer1[i1+3];
				} else {
					 //out of bounds, so transparent
					buffer2[i2  ] = 0;
					buffer2[i2+1] = 0;
					buffer2[i2+2] = 0;
					buffer2[i2+3] = 0;
				}
			}
		}
	}


	initial->doneWithBuffer(buffer1);
	persped->doneWithBuffer(buffer2);

	return 0;
}

void PerspectiveTransform::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f,indent);
}

Laxkit::Attribute *PerspectiveTransform::dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context)
{
	if (!att) att = new Attribute();
	if (what == -1) {
		att->push("from_ll","(0,0)", "Lower left original");
		att->push("from_ul","(0,1)", "Upper left original");
		att->push("from_lr","(1,0)", "Lower right original");
		att->push("from_ur","(1,1)", "upper right original");
		att->push("to_ll",  "(0,0)", "Lower left transformed");
		att->push("to_ul",  "(0,1)", "Upper left transformed");
		att->push("to_lr",  "(1,0)", "Lower right transformed");
		att->push("to_ur",  "(1,1)", "upper right transformed");
		return att;
	}

	att->pushStr("from_ll", -1, "(%.10g, %.10g)", from_ll.x, from_ll.y);
	att->pushStr("from_ul", -1, "(%.10g, %.10g)", from_ul.x, from_ul.y);
	att->pushStr("from_lr", -1, "(%.10g, %.10g)", from_lr.x, from_lr.y);
	att->pushStr("from_ur", -1, "(%.10g, %.10g)", from_ur.x, from_ur.y);
	att->pushStr("to_ll", -1, "(%.10g, %.10g)", to_ll.x, to_ll.y);
	att->pushStr("to_ul", -1, "(%.10g, %.10g)", to_ul.x, to_ul.y);
	att->pushStr("to_lr", -1, "(%.10g, %.10g)", to_lr.x, to_lr.y);
	att->pushStr("to_ur", -1, "(%.10g, %.10g)", to_ur.x, to_ur.y);

	return att;
}

void PerspectiveTransform::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	char *name,*value;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(name,"from_ll")) {
			FlatvectorAttribute(value, &from_ll);

		} else if (!strcmp(name,"from_ul")) {
			FlatvectorAttribute(value, &from_ul);

		} else if (!strcmp(name,"from_lr")) {
			FlatvectorAttribute(value, &from_lr);

		} else if (!strcmp(name,"from_ur")) {
			FlatvectorAttribute(value, &from_ul);

		} else if (!strcmp(name,"to_ll")) {
			FlatvectorAttribute(value, &to_ll);

		} else if (!strcmp(name,"to_ul")) {
			FlatvectorAttribute(value, &to_ul);

		} else if (!strcmp(name,"to_lr")) {
			FlatvectorAttribute(value, &to_lr);

		} else if (!strcmp(name,"to_ur")) {
			FlatvectorAttribute(value, &to_ur);
		}
	}


	ComputeTransform();

}


//---------------------------------- End Perspective Transform Utils -----------------------------




//--------------------------- PerspectiveInterface -------------------------------------

/*! \class PerspectiveInterface
 * \ingroup interfaces
 * \brief Interface to easily adjust mouse pressure map for various purposes.
 */


PerspectiveInterface::PerspectiveInterface(anInterface *nowner, int nid, Displayer *ndp)
  : anInterface(nowner,nid,ndp)
{
	interface_flags = 0;

	edit_to   = true;
	edit_from = false;

	hover        = PERSP_None;
	showdecs     = 1;
	show_preview = true;
	show_grid    = true;
	continuous_update = true;
	dont_update_transform = false;
	needtodraw   = 1;
	needtoremap  = 1;

	dataoc       = NULL;
	data         = NULL;
	initial      = NULL;
	persped      = NULL;

	sc           = NULL; //shortcut list, define as needed in GetShortcuts()

	inpoints.rgbf(.85,.85,.85);
	outpoints.rgbf(.75,.75,.75);

	transform    = new PerspectiveTransform();
}

PerspectiveInterface::~PerspectiveInterface()
{
	if (dataoc) delete dataoc;
	if (data) { data->dec_count(); data=NULL; }
	if (sc) sc->dec_count();
	if (initial) initial->dec_count();
	if (persped) persped->dec_count();
	if (transform) transform->dec_count();
}

const char *PerspectiveInterface::whatdatatype()
{
	//return "PerspectiveData";
	return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *PerspectiveInterface::Name()
{ return _("Perspective tool"); }


//! Return new PerspectiveInterface.
/*! If dup!=NULL and it cannot be cast to PerspectiveInterface, then return NULL.
 */
anInterface *PerspectiveInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) {
		PerspectiveInterface *p = new PerspectiveInterface(NULL,id,NULL);
		dup = p;
		p->interface_flags = interface_flags;

	} else if (!dynamic_cast<PerspectiveInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

/*! Assumes dataoc has something meaningful.
 *
 * Return 0 if successfully set up, or nonzero error.
 */
int PerspectiveInterface::SetupPreviewImages()
{
	//ImageManager()->NewImage(200,200);

	if (!dataoc || !dataoc->obj) return 1;
	if (initial) initial->dec_count();
	initial = dataoc->obj->GetPreview();
	if (!initial) return 1;
	initial->inc_count();

	if (!persped) persped = ImageLoader::NewImage(200,200);

	transform->MapImage(data, initial, persped, 1);
	return 0;
}

//! Use the object at oc if it is an PerspectiveData.
int PerspectiveInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	//SomeData *ndata=dynamic_cast<PerspectiveData *>(oc->obj);
	SomeData *ndata = oc->obj;
	if (!ndata) return 0;

	if (data && data!=ndata) {
		Clear(NULL);
	}
	if (dataoc) delete dataoc;
	dataoc = oc->duplicate();

	if (data != ndata) {
		data = ndata;
		data->inc_count();
	}

	if (!dont_update_transform) {
		 //1,2,3,4  ->  ll,lr,ul,ur
		transform->from_ll = transform->to_ll = data->transformPoint(flatpoint(data->minx,data->miny)); //obj parent coords
		transform->from_lr = transform->to_lr = data->transformPoint(flatpoint(data->maxx,data->miny));
		transform->from_ul = transform->to_ul = data->transformPoint(flatpoint(data->minx,data->maxy));
		transform->from_ur = transform->to_ur = data->transformPoint(flatpoint(data->maxx,data->maxy));

		ComputeTransform();
		SetupPreviewImages();
	}


	needtodraw=1;
	return 1;
}

/*!
 * <pre>
 *  | 00  01  02 | |x|
 *  | 10  11  12 | |y|
 *  | 20  21  22 | |1|
 * </pre>
 */
flatpoint PerspectiveInterface::ComputePoint(double x,double y)
{
	return transform->transform(x,y);
}

//void PerspectiveInterface::SetPoints(flatpoint nfrom_ll, flatpoint nfrom_lr, flatpoint nfrom_ul, flatpoint nfrom_ur,
//									 flatpoint nto_ll,   flatpoint nto_lr,   flatpoint nto_ul,   flatpoint nto_ur)
//{
//	transform->to_ll=nto_ll;
//	transform->to_lr=nto_lr;
//	transform->to_ul=nto_ul;
//	transform->to_ur=nto_ur;
//
//	transform->from_ll=nfrom_ll;
//	transform->from_lr=nfrom_lr;
//	transform->from_ul=nfrom_ul;
//	transform->from_ur=nfrom_ur;
//}

void PerspectiveInterface::ResetTransform()
{
	flatpoint from_ll(data->minx, data->miny);
	flatpoint from_lr(data->maxx, data->miny);
	flatpoint from_ul(data->minx, data->maxy);
	flatpoint from_ur(data->maxx, data->maxy);

	if (interface_flags & PERSP_Parent_Space) {
		from_ll = data->transformPoint(from_ll);
		from_lr = data->transformPoint(from_lr);
		from_ul = data->transformPoint(from_ul);
		from_ur = data->transformPoint(from_ur);
	}

	transform->SetFrom(from_ll, from_lr, from_ul, from_ur);
	transform->ResetTransform();
	Modified();

	needtoremap=0;
	needtodraw=1;
}

void PerspectiveInterface::ComputeTransform()
{
	transform->ComputeTransform();
	needtoremap=0;
}

int PerspectiveInterface::UseThis(anObject *nobj, unsigned int mask)
{
	if (!nobj) return 1;

	if (dynamic_cast<PerspectiveTransform*>(nobj)) {
		if (transform) transform->dec_count();
		transform = dynamic_cast<PerspectiveTransform*>(nobj);
		transform->inc_count();

		ComputeTransform();
		SetupPreviewImages();
		needtodraw=1;
		return 0;
	}

	return 0;
}

/*! Return the object's ObjectContext to make sure that the proper context is already installed
 * before Refresh() is called.
 */
ObjectContext *PerspectiveInterface::Context()
{
	//return NULL;
	return dataoc;
}

/*! Any setup when an interface is activated, which usually means when it is added to
 * the interface stack of a viewport.
 */
int PerspectiveInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int PerspectiveInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void PerspectiveInterface::Clear(SomeData *d)
{
	if (dataoc) { delete dataoc; dataoc=NULL; }
	if (data) { data->dec_count(); data=NULL; }
}

void PerspectiveInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
}

Laxkit::MenuInfo *PerspectiveInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!menu) menu = new MenuInfo;

	if (menu->n()) menu->AddSep(_("Perspective"));

	menu->AddItem(_("Reset"), PERSP_Reset);
	return menu;
}

int PerspectiveInterface::Event(const Laxkit::EventData *data, const char *mes)
{
    if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
        int i =s->info2; //id of menu item

        if ( i == PERSP_Reset) {
			PerformAction(PERSP_Reset);
			return 0;
		}

		return 0;
	}

	return 1; //event not absorbed
}


///*! Draw some data other than the current data.
// * This is called during screen refreshes.
// */
//int PerspectiveInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
//{
//	return 1;
////	if (!ndata || dynamic_cast<PerspectiveData *>(ndata)==NULL) return 1;
////
////	PerspectiveData *bzd=data;
////	data=dynamic_cast<PerspectiveData *>(ndata);
////
////	 // store any other state we need to remember
////	 // and update to draw just the temporary object, no decorations
////	int td=showdecs,ntd=needtodraw;
////	showdecs=0;
////	needtodraw=1;
////
////	Refresh();
////
////	 //now restore the old state
////	needtodraw=ntd;
////	showdecs=td;
////	data=bzd;
////	return 1;
//}


int PerspectiveInterface::Refresh()
{

	if (needtodraw==0) return 0;
	needtodraw=0;

	if (!data) return 0;

	if (needtoremap) {
		ComputeTransform();
		if (buttondown.any() && continuous_update && initial) transform->MapImage(data, initial, persped, 1);
	}



	int lines=8;

	//we are in obj coords


	if (interface_flags & PERSP_Parent_Space) {
		Affine a(*data);
		a.Invert(); //this nullifies the obj transform
		dp->PushAndNewTransform(a.m());
		//so now we are in obj parent coords
	} else {
		dp->PushAxes();
		if (dataoc && dataoc->obj != data) {
			// we are using a substitute object for dataoc->obj
			double m[6];
			transform_invert(m, dataoc->obj->m());
			dp->PushAndNewTransform(m);
			dp->PushAndNewTransform(data->m());
		}
	}

	//original corners
	flatpoint from_ll;
	flatpoint from_lr;
	flatpoint from_ul;
	flatpoint from_ur;

	if (!edit_from) {
		from_ll.set(data->minx, data->miny);
		from_lr.set(data->maxx, data->miny);
		from_ul.set(data->minx, data->maxy);
		from_ur.set(data->maxx, data->maxy);
	} else {
		from_ll.set(transform->from_ll.x, transform->from_ll.y);
		from_lr.set(transform->from_lr.x, transform->from_lr.y);
		from_ul.set(transform->from_ul.x, transform->from_ul.y);
		from_ur.set(transform->from_ur.x, transform->from_ur.y);
	}

	if (interface_flags & PERSP_Parent_Space) {
		from_ll = data->transformPoint(from_ll);
		from_lr = data->transformPoint(from_lr);
		from_ul = data->transformPoint(from_ul);
		from_ur = data->transformPoint(from_ur);
	}

	 //computed transformed corners (not necessarily the same as the control points!)
	flatpoint to_ll = transform->transform(from_ll);
	flatpoint to_lr = transform->transform(from_lr);
	flatpoint to_ul = transform->transform(from_ul);
	flatpoint to_ur = transform->transform(from_ur);

	flatpoint l = from_ul - from_ll; //points bottom from fromp
	flatpoint r = from_ur - from_lr;
	flatpoint t = from_ur - from_ul; //points left from right
	flatpoint b = from_lr - from_ll;

	if (dataoc && initial && persped && show_preview) {
		 //draw transformed preview image
		DoubleBBox box;
		box.addtobounds(to_ll);
		box.addtobounds(to_lr);
		box.addtobounds(to_ul);
		box.addtobounds(to_ur);
		dp->imageout(persped, box.minx,box.miny, box.boxwidth(),box.boxheight());
	}

	dp->NewFG(.75,.75,.75);
	dp->LineWidthScreen(1);

	 //draw grid
	if (show_grid && transform->IsValid()) {
		flatpoint p1,p2;
		for (int c=1; c<lines; c++) {
			p1=transform->transform(from_ll+c/(float)(lines)*l);
			p2=transform->transform(from_lr+c/(float)(lines)*r);
			dp->drawline(p1,p2); //horiz line

			p1=transform->transform(from_ll+c/(float)(lines)*b);
			p2=transform->transform(from_ul+c/(float)(lines)*t);
			dp->drawline(p1,p2); //vert line
		}
	}

	dp->NewFG(.75,.75,.75);
	dp->NewBG(0.6,0.6,0.6);
	dp->LineWidthScreen(hover == PERSP_Move ? 4 : 2);

	 //outline of transformed area
	dp->drawline(to_ll, to_lr); //transformed corners
	dp->drawline(to_lr, to_ur);
	dp->drawline(to_ur, to_ul);
	dp->drawline(to_ul, to_ll);

	 //_to_ control points , draw slightly thicker to stand out more
	dp->LineWidthScreen(2);
	double thin = ScreenLine();
	dp->drawpoint(transform->to_ll, 10*thin, 0); //actual to points
	dp->drawpoint(transform->to_lr, 10*thin, 0);
	dp->drawpoint(transform->to_ul, 10*thin, 0);
	dp->drawpoint(transform->to_ur, 10*thin, 0);

	 //show hovered point
	dp->NewFG(.25,.25,.25);
	dp->LineWidthScreen(1);
	dp->drawpoint(transform->to_ll, 10*thin, hover==PERSP_ll ? 2 : 0);
	dp->drawpoint(transform->to_lr, 10*thin, hover==PERSP_lr ? 2 : 0);
	dp->drawpoint(transform->to_ul, 10*thin, hover==PERSP_ul ? 2 : 0);
	dp->drawpoint(transform->to_ur, 10*thin, hover==PERSP_ur ? 2 : 0);


	//DBG: show some transformed points
//	DBG dp->NewFG(0.,0.,1.);
//	DBG dp->drawpoint(mousep, 7, 0);
//
//	DBG dp->NewFG(1.,0.,0.);
//	DBG initialp = transform->transformInverse(mousep); //transformed -> original
//	DBG dp->drawpoint(initialp, 5, 1);
//
//	DBG dp->NewFG(0.,1.,0.);
//	DBG dp->drawpoint(transform->transform(mousep), 5, 1); //original -> transformed
//
//	DBG char str[200];
//	DBG sprintf(str, "mouse: %f,%f, invtrans: %f,%f", mousep.x,mousep.y, initialp.x,initialp.y);
//	DBG PostMessage(str);


	dp->PopAxes();
	if (!(interface_flags & PERSP_Parent_Space) && dataoc && dataoc->obj != data) {
		dp->PopAxes();
		dp->PopAxes();
	}
	return 0;
}

/*! Check for clicking down on other objects, possibly changing control to that other object.
 *
 * Return 1 for changed object to another of same type.
 * Return 2 for changed to object of another type (switched tools).
 * Return 0 for nothing found at x,y.
 */
int PerspectiveInterface::OtherObjectCheck(int x,int y,unsigned int state)
{
	if (interface_flags & PERSP_Dont_Change_Object) return 0;

	ObjectContext *oc=NULL;
	viewport->FindObject(x,y,NULL,NULL,1,&oc);
	SomeData *obj=NULL;
	if (oc && oc->obj) obj=oc->obj;

	if (obj) {
		 // found another PerspectiveData to work on.
		 // If this is primary, then it is ok to work on other images, but not click onto
		 // other types of objects.
		UseThisObject(oc);
		if (viewport) viewport->ChangeObject(oc,0,true);
		needtodraw=1;
		return 1;

	}

	return 0;
}

int PerspectiveInterface::scan(double x, double y)
{
	double grab = 10*ScreenLine();

	flatpoint p(x,y);
	flatpoint pts[4] = {
		realtoscreen(transform->to_ll), //remember to_ll, etc should be in obj parent space
		realtoscreen(transform->to_lr),
		realtoscreen(transform->to_ur),
		realtoscreen(transform->to_ul)
	};

	if ((interface_flags & PERSP_Parent_Space) || !data) {
		pts[0] = realtoscreen(transform->to_ll);
		pts[1] = realtoscreen(transform->to_lr);
		pts[2] = realtoscreen(transform->to_ur);
		pts[3] = realtoscreen(transform->to_ul);
	} else {
		pts[0] = realtoscreen(data->transformPoint(transform->to_ll));
		pts[1] = realtoscreen(data->transformPoint(transform->to_lr));
		pts[2] = realtoscreen(data->transformPoint(transform->to_ur));
		pts[3] = realtoscreen(data->transformPoint(transform->to_ul));
	}

	if (pts[0].distanceTo(p)<grab) return PERSP_ll;
	if (pts[1].distanceTo(p)<grab) return PERSP_lr;
	if (pts[2].distanceTo(p)<grab) return PERSP_ur;
	if (pts[3].distanceTo(p)<grab) return PERSP_ul;

	if (point_is_in(p, pts, 4)) return PERSP_Move;

	return 0;
}

int PerspectiveInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	int nhover=scan(x,y);
	if (nhover != PERSP_None) {
		buttondown.down(d->id,LEFTBUTTON, x,y, nhover);

		needtodraw=1;
		return 0;
	}



	 // So, was clicked outside current image or on blank space, make new one or find other one.
	int other = OtherObjectCheck(x,y,state);
	if (other==2) return 0; //control changed to some other tool
	if (other==1) {
		buttondown.down(d->id,LEFTBUTTON, x,y, PERSP_Move);
		return 0; //object changed via UseThisObject().. nothing more to do here!
	}

	 //OtherObjectCheck:
	 //  change to other type of object if not primary
	 //  change to other of same object always ok



	needtodraw=1;
	return 1; //return 0 for absorbing event, or 1 for ignoring
}

int PerspectiveInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any(d->id, LEFTBUTTON)) return 1;

	int action=0;
	buttondown.up(d->id,LEFTBUTTON, &action);
	if (action != PERSP_None && !continuous_update && initial) {
		transform->MapImage(data, initial, persped, 1);
		Modified();
	}
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

/*! This is called whenever there's been a user induced change in the transform,
 * like by dragging on screen handles.
 */
void PerspectiveInterface::Modified()
{
}


int PerspectiveInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	DBG if (data) {
	DBG 	mousep = screentoreal(x,y); //should be obj parent coords
	DBG 	needtodraw=1;
	DBG }

	DBG cerr <<" PerspectiveInterface::MouseMove "<<x<<','<<y<<endl;


	if (!buttondown.any()) {
		// update any mouse over state

		int nhover=scan(x,y);
		if (nhover!=hover) {
			hover=nhover;
			needtodraw=1;
		}
		return 0;
	}

	 //else deal with mouse dragging...
	if (hover == PERSP_None) return 0;

	int lx,ly;
	int h;
	buttondown.move(d->id, x,y, &lx,&ly);
	buttondown.getextrainfo(d->id, LEFTBUTTON, &h);

	flatpoint dv;
	if ((interface_flags & PERSP_Parent_Space) || !data) {
		dv = screentoreal(x,y) - screentoreal(lx,ly);
	} else {
		dv = data->transformPointInverse(screentoreal(x,y)) - data->transformPointInverse(screentoreal(lx,ly));
	}
	if (dv.x == 0 && dv.y == 0) return 0;

	if (h == PERSP_ll || h == PERSP_Move) transform->to_ll += dv;
	if (h == PERSP_lr || h == PERSP_Move) transform->to_lr += dv;
	if (h == PERSP_ul || h == PERSP_Move) transform->to_ul += dv;
	if (h == PERSP_ur || h == PERSP_Move) transform->to_ur += dv;

	needtoremap=1;

	if (continuous_update && data) {
		 //transform a path... do this here for testing purposes.
		 // *** move somewhere responsible later!!!
		//ComputeTransform();
		//PathsData *data = dynamic_cast<PathsData*>(dataoc->obj);
		//if (data) {

		//}
		//------------
		Modified();
	}


	needtodraw=1;
	return 0;
}

int PerspectiveInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"PerspectiveInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int PerspectiveInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1; //key not dealt with, propagate to next interface
}

Laxkit::ShortcutHandler *PerspectiveInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

    sc=new ShortcutHandler(whattype());

	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
    sc->Add(PERSP_Reset,  'z',0,0, "Reset",  _("Reset transform"),      NULL,0);
    sc->Add(PERSP_Grid,   'g',0,0, "Grid",   _("Toggle grid"),          NULL,0);
    sc->Add(PERSP_Preview,'p',0,0, "Preview",_("Toggle preview image"), NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;
}

/*! Return 0 for action performed, or nonzero for unknown action.
 */
int PerspectiveInterface::PerformAction(int action)
{
	if (action == PERSP_Reset) {
		ResetTransform();
		return 0;

	} else if (action == PERSP_Grid) {
		show_grid = !show_grid;
		PostMessage(show_grid ? _("Show grid") : _("Don't show grid"));
		needtodraw=1;
		return 0;

	} else if (action == PERSP_Preview) {
		show_preview = !show_preview;
		PostMessage(show_preview ? _("Show preview") : _("Don't show preview"));
		needtodraw=1;
		return 0;
	}

	return 1;
}


} // namespace LaxInterfaces

