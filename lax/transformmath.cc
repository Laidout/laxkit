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
//    Copyright (C) 2004-2012 by Tom Lechner
//


//-------------------- Affine Transform Utilities ---------------------

#include <lax/transformmath.h>
#include <cstring>
#include <cstdlib>


#include <iostream>
using namespace std;
#define DBG

namespace Laxkit {
	
/*! \defgroup transformmath Transform math
 *
 * The Affine class, and (essentially C) functions to manipulate double[6] arrays corresponding to
 * a 2 dimensional affine transform. These are used, for instance, in Displayer and SomeData.
 *
 * <pre>
 *      [ a  b  0 ]
 *      [ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 *
 *  screen x= ax + cy + tx  --> screen = [x',y',1] = [x,y,1] * CTM  = real * CTM
 *  screen y= bx + dy + ty
 *
 * So (transformed coord) == (original coord) * Matrix
 *
 * To demonstrate a common usage scenario,
 * Suppose you have 2 objects with transforms M1 and M2. (coord)*M1 turns an object 1 
 * coordinate into its parent space coordinates, and similarly for object 2.
 *
 * The matrix you need to transform object 1 coordinates into object 2 coordinates is then:
 *  
 *   M1 x inverse(M2)
 *
 * So the coordinate in object 2 == (object 1 coord) x M1 x inverse(M2)
 * </pre>
 */



//------------------------------- Affine ----------------------------------------
/*! \class An affine transform.
 */

Affine::Affine()
{ transform_identity(_m); }

//! If mm==nullptr, set identity.
Affine::Affine(const double *mm)
{
	if (mm) memcpy(_m,mm, 6*sizeof(double));
	else transform_identity(_m);
}

Affine::Affine(const Affine &mm)
{
	memcpy(_m, mm.m(), 6*sizeof(double));
}

Affine::Affine(double xx,double xy,double yx,double yy,double tx,double ty)
{
	_m[0]=xx;
	_m[1]=xy;
	_m[2]=yx;
	_m[3]=yy;
	_m[4]=tx;
	_m[5]=ty;
}

Affine &Affine::operator=(Affine const &mm)
{
	memcpy(_m, mm.m(), 6*sizeof(double));
	return *this;
}

Affine &Affine::operator*=(Affine const &M)
{
	double mm[6];
	transform_mult(mm,_m,M.m());
	transform_copy(_m,mm);
	return *this;
}

Affine Affine::operator*(Affine const m)
{
	Affine a(*this);
	a*=m;
	return a;
}

Affine::~Affine()
{}

void Affine::set(Affine const &a)
{ transform_copy(_m,a.m()); }

void Affine::setIdentity()
{ transform_identity(_m); }

#define EPSILON 1e-15

bool are_near(double a, double b, double eps=EPSILON) { return a-b <= eps && a-b >= -eps; }

bool Affine::isIdentity()
{
	return are_near(_m[0], 1.0, EPSILON) && are_near(_m[1], 0.0, EPSILON) &&
           are_near(_m[2], 0.0, EPSILON) && are_near(_m[3], 1.0, EPSILON) &&
           are_near(_m[4], 0.0, EPSILON) && are_near(_m[5], 0.0, EPSILON);
}

/*! Return whether the members are exactly equal. */
bool Affine::IsEqual(Affine const &m) const
{
	return _m[0] == m.m(0) &&
		   _m[1] == m.m(1) &&
		   _m[2] == m.m(2) &&
		   _m[3] == m.m(3) &&
		   _m[4] == m.m(4) &&
		   _m[5] == m.m(5);
}

/*! Return whether each member is withich epsilon of each other. */
bool Affine::IsAlmostEqual(Affine const &m, double epsilon) const
{
	return are_near(_m[0], m.m(0), epsilon) && are_near(_m[1], m.m(1), epsilon) &&
           are_near(_m[2], m.m(2), epsilon) && are_near(_m[3], m.m(3), epsilon) &&
           are_near(_m[4], m.m(4), epsilon) && are_near(_m[5], m.m(5), epsilon);
}


/*! Set angle of xaxis, and rotate y axis so it makes the original angle with the x axis.
 * Does not affect origin. Only sets rotation of axes.
 */
void Affine::setRotation(double angle)
{
	double x=norm(xaxis());
	double y=norm(yaxis());
	double aangle=-angle_full(xaxis(),yaxis());

	_m[0]= x*cos(angle);
	_m[1]=-x*sin(angle);
	_m[2]= y*cos(angle+aangle);
	_m[3]=-y*sin(angle+aangle);
}

/*! Set angle of xaxis and yaxis..
 * Does not affect origin. Only sets rotation of axes and preserves scale.
 */
void Affine::setShear(double anglex, double angley)
{
	double x = norm(xaxis());
	double y = norm(yaxis());
	
	_m[0]= x*cos(anglex);
	_m[1]=-x*sin(anglex);
	_m[2]= y*cos(angley);
	_m[3]=-y*sin(angley);
}

void Affine::setScale(double sx,double sy)
{
	flatpoint v  = xaxis();
	double l = norm(v);
	if (l) xaxis(v / (l * sx));

	v  = yaxis();
	l = norm(v);
	if (l) yaxis(v / (l * sy));
}

void Affine::setBasis(flatpoint o, flatpoint x,flatpoint y)
{
	transform_from_basis(_m, o,x,y);
}

void Affine::setBasics(double x,double y,double sx,double sy,double angle,double shear)
{
	transform_from_basics(_m, x,y,sx,sy,angle,shear);
}

void Affine::getBasics(double *x,double *y,double *sx,double *sy,double *angle,double *shear)
{
	transform_to_basics(_m, x,y,sx,sy,angle,shear);
}

//! Clear shearing and unequal scaling. Null vectors stay null vectors.
/*! Set the x and y axis to the same length, and at right angle to each other.
 *  If preserve_x==0, then preserve the x axis, else preserve y.
 *
 * Calls xaxis(transpose(xaxis())) or yaxis(-transpose(yaxis())).
 *
 * If normalize!=0, then make vectors be of unit length.
 */
void Affine::Unshear(int preserve_x, int normalize)
{
    if (preserve_x) yaxis(transpose(xaxis()));
    else xaxis(-transpose(yaxis()));

	if (normalize) {
		double d=norm(xaxis());
		if (d!=0) {
			xaxis(xaxis()/d);
			yaxis(yaxis()/d);
		}
	}
    //touchContents();
    //modtime=time(nullptr);
}

//! Make x and y both be unit vectors, but point in the same direction as before.
void Affine::Normalize()
{
	double d=norm(xaxis());
	if (d!=0) xaxis(xaxis()/d);
	d=norm(yaxis());
	if (d!=0) yaxis(yaxis()/d);
}

/*! Get magnification, where (vx,vy) is a vector outside the transform, 
 * and returned value is norm(vx,vy)/norm(transformPointInverse(vx,vy)-transformPointInverse(0,0)).
 */
double Affine::GetIMagnification(double vx, double vy)
{
	return get_imagnification(_m, vx,vy);
}

double Affine::GetIMagnification(flatpoint v)
{
	return get_imagnification(_m, v.x,v.y);
}

/*! Get magnification, where (vx,vy) is a vector in context, 
 * and returned value is norm(vx,vy)/norm(transformPoint(vx,vy)-transformPoint(0,0)).
 */
double Affine::GetMagnification(double vx, double vy)
{
	return get_magnification(_m, vx,vy);
}

double Affine::GetMagnification(flatpoint v)
{
	return get_magnification(_m, v.x,v.y);
}

void Affine::Translate(flatvector d)
{
	_m[4]+=d.x;
	_m[5]+=d.y;
}

/*! angle in radians */
void Affine::Rotate(double angle)
{
	double mm[6];
	transform_copy(mm,_m);
	transform_rotate(mm,angle);
	transform_copy(_m,mm);
}

/*! This basically translates around_point to origin, rotates, then translates back.
 */
void Affine::Rotate(double angle, flatpoint around_point)
{
	double mm[6];
	transform_copy(mm,_m);
	mm[4]-=around_point.x;
	mm[5]-=around_point.y;

	 //do a rotation
	double r[6],s[6];
	r[4]=r[5]=0;
	r[0]=cos(angle);
	r[2]=sin(angle);
	r[1]=-r[2];
	r[3]=r[0];
	transform_mult(s,mm,r);
	transform_copy(mm,s);

	mm[4]+=around_point.x;
	mm[5]+=around_point.y;
	transform_copy(_m,mm);
}

void Affine::RotatePointed(flatpoint anchor1, flatpoint anchor2, flatpoint newanchor2)
{
	double angle=angle2(newanchor2-anchor1, anchor2-anchor1);
	Rotate(angle,anchor1);
	//flatpoint a1=transformPoint(anchor1);
	//xaxis(rotate(xaxis(), angle));
	//yaxis(rotate(yaxis(), angle));
	//origin(origin() + transformPoint(anchor1)-a1);
}

//! Rotate and scale so that anchor1 stays fixed, but anchor2 is shifted to newanchor2.
void Affine::RotateScale(flatpoint anchor1, flatpoint anchor2, flatpoint newanchor2)
{
	flatpoint o,x1,x2,y1,y2,p;
	o=anchor1;
	x1=anchor2-o;
	x2=x1+(newanchor2-anchor2);
	y1=transpose(x1);
	y2=transpose(x2);

	double M[6],M2[6],N[6],T[6];
	transform_from_basis(M ,o,x1,y1);
	transform_from_basis(M2,o,x2,y2);
	transform_invert(T,M);
	transform_mult(N,T,M2);
	transform_mult(T,_m,N);
	transform_copy(_m,T);
}

/*! Scale and shear such that a random point off of (anchor2-anchor1) stays
 * the same distance from the new vector (newanchor2-anchor1).
 *
 * Basically, RotateScale() with the specified anchors. Then, AnchorShear()
 * with anchor1,newanchor2 as stationary points
 */
void Affine::Stretch(flatpoint anchor1, flatpoint anchor2, flatpoint newanchor2)
{
	flatpoint p=anchor1+transpose(anchor2-anchor1);
	p=transformPointInverse(p); //random point away from line
	double d=norm(anchor2-anchor1);

	RotateScale(anchor1,anchor2,newanchor2);

	flatpoint olda=transformPoint(p);
	p=transpose(newanchor2-anchor1);
	p.normalize();
	p*=d;
	flatpoint newa= anchor1+p;

	//AnchorShear(anchor1,newanchor2, transformPointInverse(olda),newa);
	AnchorShear(anchor1,newanchor2, olda,newa);
}

//! Transform so that anchor1 and 2 stay fixed, but anchor3 is shifted to newanchor3.
/*! Points are in this->parent space.
 */
void Affine::AnchorShear(flatpoint anchor1, flatpoint anchor2, flatpoint anchor3, flatpoint newanchor3)
{
	flatpoint o,x,y1,y2,p;
	o=anchor1;
	x=anchor2-o;
	y1=newanchor3-o;
	y2=y1+(newanchor3-anchor3);

	double M[6],  // matrix before
		   M2[6], // matrix after 
		   N[6],  // M * N = M2, or M * N * M2^-1 == I
		   T[6];  // temp matrix
	 // Basic linear algebra: Transform a generic M to M2 with N.
	 //  We know M and M2, so find N. This same N
	 //  transforms any other affine matrix. so the somedata->m() (sm,sm2) is
	 //  found with: SM * N * SM2 == I, so SM2 = (SM * N)^-1
	transform_from_basis(M ,o,x,y1);
	transform_from_basis(M2,o,x,y2);
	transform_invert(T,M);
	transform_mult(N,T,M2);
	transform_mult(T,_m,N);
	transform_copy(_m,T);
}

void Affine::Scale(double s)
{
	_m[0]*=s;
	_m[1]*=s;
	_m[2]*=s;
	_m[3]*=s;
	_m[4]*=s;
	_m[5]*=s;
}

void Affine::Scale(double sx, double sy)
{
	_m[0]*=sx;
	_m[1]*=sx;
	_m[2]*=sy;
	_m[3]*=sy;
	_m[4]*=sx;
	_m[5]*=sy;
}

//! Scale around point o of parent space.
void Affine::Scale(flatpoint o, double s)
{
	flatpoint to=transform_point_inverse(_m,o);
	Scale(s);
	origin(origin()-transform_point(_m,to)+o);
}

//! Scale around point o of parent space.
void Affine::Scale(flatpoint o,double sx,double sy)
{
	flatpoint to=transform_point_inverse(_m,o);
	_m[0]*=sx;
	_m[1]*=sx;
	_m[2]*=sy;
	_m[3]*=sy;
	origin(origin()+transform_point(_m,to)-o);
}

//! Scale as if you move anchor2 to newanchor2, while anchor1 stays the same.
void Affine::Scale(flatpoint anchor1, flatpoint anchor2, flatpoint newanchor2)
{
	double d1=norm2(   anchor2-anchor1);
	double d2=norm2(newanchor2-anchor1);
	if (!d1) return;
	Scale(anchor1,sqrt(d2/d1));
}

/*! Flips the x axis. Note that this does not flip within a bounding box.
 */
void Affine::FlipH()
{
	_m[0]=-_m[0];
	_m[1]=-_m[1];
}

/*! Flips the y axis. Note that this does not flip within a bounding box.
 */
void Affine::FlipV()
{
	_m[2]=-_m[2];
	_m[3]=-_m[3];
}

//! Flip across the axis of f1 to f2.
void Affine::Flip(flatpoint f1,flatpoint f2)
{
	double mf[6],mfinv[6],f[6];
	double t[6],tt[6];
	transform_set(f, 1,0,0,-1,0,0); //the flip transform, in y direction
	transform_from_basis(mf, f1, f2-f1, transpose(f2-f1)); //basis, p=flip1, x=flip2-flip1
	transform_invert(mfinv,mf);

	transform_mult(t, mfinv,f);
	transform_mult(tt, t,mf);
	transform_mult(t, _m,tt);
	transform_copy(_m,t);
}

//! this=this*m
void Affine::Multiply(const Affine &m)
{
	double result[6];
	transform_mult(result,_m,m.m());
	transform_copy(_m,result);
}

//! this=this*m
void Affine::Multiply(const double *m)
{
	double result[6];
	transform_mult(result,_m,m);
	transform_copy(_m,result);
}

//! this=m*this
void Affine::PreMultiply(const Affine &m)
{
	double result[6];
	transform_mult(result,m.m(),_m);
	transform_copy(_m,result);
}

//! this=m*this
void Affine::PreMultiply(const double *m)
{
	double result[6];
	transform_mult(result,m,_m);
	transform_copy(_m,result);
}

//! Return a new matrix that is the inverse of this, if possible.
Affine Affine::Inversion()
{
	Affine a(*this);
	a.Invert();
	return a;
}

//! Make this the inverse of whatever it is.
void Affine::Invert()
{
	double mm[6];
	transform_invert(mm,_m);
	transform_copy(_m,mm);
}

/*! Return whether the matrix is degenerate or not.
 */
bool Affine::IsInvertible()
{
	return (_m[0]*_m[3]-_m[1]*_m[2]) != 0;
}

flatpoint Affine::transformPoint(flatpoint p)
{
	return transform_point(_m,p);
}

flatpoint Affine::transformPointInverse(flatpoint p)
{
	return transform_point_inverse(_m,p);
}

//! Apply the transfrom, but ignore this's translation component.
flatpoint Affine::transformVector(flatpoint p)
{
	return transform_vector(_m,p);
}

flatpoint Affine::transformVectorInverse(flatpoint p)
{
	return transform_vector_inverse(_m,p);
}

void Affine::m(const double *mm)
{ memcpy(_m,mm, 6*sizeof(double)); }

void Affine::m(double xx,double xy,double yx,double yy,double tx,double ty)
{
	_m[0]=xx;
	_m[1]=xy;
	_m[2]=yx;
	_m[3]=yy;
	_m[4]=tx;
	_m[5]=ty;
}



//------------------------------- AffineStack ----------------------------------------

/*! \class AffineStack
 * Just like Affine, but adds a stack of transforms.
 */


AffineStack::AffineStack()
  : axesstack(2)
{
}

AffineStack::~AffineStack()
{
}

/*! Return value is how many levels in axesstack after pushing.
 */
int AffineStack::PushAxes()
{
    double *tctm=new double[6];
    transform_copy(tctm,_m);
    axesstack.push(tctm,2);

	return axesstack.n;
}

/*! Return value is how many levels in axesstack after pushing.
 */
int AffineStack::PushAndNewAxes(const double *m)
{
    double *tctm=new double[6];
    transform_copy(tctm,_m);
    axesstack.push(tctm,2);
	transform_copy(_m, m);

	return axesstack.n;
}

/*! Return value is how many levels in axesstack after pushing.
 */
int AffineStack::PushAndNewAxes(const Affine &m)
{
    double *tctm=new double[6];
    transform_copy(tctm,_m);
    axesstack.push(tctm,2);
	transform_copy(_m, m.m());

	return axesstack.n;
}

/*! Return value is how many levels in axesstack after pushing.
 */
int AffineStack::PushAndNewAxes(double a,double b,double c,double d,double x0,double y0)
{
    double *tctm=new double[6];
    transform_copy(tctm,_m);
    axesstack.push(tctm,2);
	m(a,b,c,d,x0,y0);

	return axesstack.n;
}

/*! Return value is how many levels still in axesstack after popping.
 * 
 * If m_ret!=nullptr, then copy the popped values into an already allocated m_ret.
 */
int AffineStack::PopAxes(double *m_ret)
{
    if (axesstack.n==0) return 0;

    if (m_ret) transform_copy(m_ret, _m);

    transform_copy(_m, axesstack.e[axesstack.n-1]);
    axesstack.remove(axesstack.n-1);

	return axesstack.n;
}

/*! Flush the stack, and set ourselves to identity.
 */
void AffineStack::ClearAxes()
{
	axesstack.flush();
	setIdentity();
}

/*! Return 1 for out of bounds. Return 0 for found, and copy that level to mm.
 */
int AffineStack::GetAxes(int which, double *mm)
{
	if (which<0 || which>=axesstack.n) return 1;
	transform_copy(mm, axesstack.e[which]);
	return 0;
}




//--------------------------------- base transform related functions -------------------------------



//! Write out the transform d to cout, or to cerr if DBG is enabled.
/*! \ingroup transformmath */
void dumpctm(const double *d, bool to_err)
{	
	DBG if (to_err) {
	DBG   cerr <<"--- dumpctm transform: ";
	DBG for (int c=0; c<6; c++) cerr<<d[c]<<(c<5?", ":"\n");
	DBG } else {
	cout <<"--- dumpctm transform: ";
	for (int c=0; c<6; c++) cout<<d[c]<<(c<5?", ":"\n");
	DBG }
}

void dumpctm_out(const double *d)
{	
	DBG if (1) {
	DBG   cout <<"--- dumpctm transform: ";
	DBG for (int c=0; c<6; c++) cerr<<d[c]<<(c<5?", ":"\n");
	DBG } else {
	cout <<"--- dumpctm transform: ";
	for (int c=0; c<6; c++) cout<<d[c]<<(c<5?", ":"\n");
	DBG }
}

//! Return identity matrix. If result==nullptr, then return a new'd double[6].
/*! \ingroup transformmath
 */
double *transform_identity(double *result)
{
	if (result == nullptr) result=new double[6];
	result[1]=result[2]=result[4]=result[5]=0;
	result[0]=result[3]=1;
	return result;
}

bool transforms_equal(const double *m1, const double *m2, double epsilon)
{
	return fabs(m1[0]-m2[0])<epsilon && fabs(m1[1]-m2[1])<epsilon && fabs(m1[2]-m2[2])<epsilon
	    && fabs(m1[3]-m2[3])<epsilon && fabs(m1[4]-m2[4])<epsilon && fabs(m1[5]-m2[5])<epsilon;
}

//! Invert m into result. If result==nullptr, then return a new double[6].
/*! \ingroup transformmath
 * <pre>
 *      [ a  b  0 ]
 *  CTM=[ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 *
 * {{d/(a*d-b*c),           (-b)/(a*d-b*c),        0},
 *  {(-c)/(a*d-b*c),        a/(a*d-b*c),           0},
 *  {(c*ty-d*tx)/(a*d-b*c), (b*tx-a*ty)/(a*d-b*c), 1}}
 * </pre>
 */
double *transform_invert(double *result,const double *m)
{
	if (result==nullptr) result=new double[6];
	double d=m[0]*m[3]-m[1]*m[2];
	result[0]=m[3]/d;
	result[1]=-m[1]/d;
	result[2]=-m[2]/d;
	result[3]=m[0]/d;
	result[4]=(m[2]*m[5]-m[4]*m[3])/d;
	result[5]=(m[1]*m[4]-m[0]*m[5])/d;
	return result;
}

int is_degenerate_transform(const double *m)
{ return m[0]*m[3]-m[1]*m[2] == 0; }

//! Multiply 2 6 member transform arrays: result = a x b.
/*! \ingroup transformmath
 * The matrices are aligned like so:
 * <pre>
 *      [ a  b  0 ]
 *      [ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 * </pre>
 *
 * If result is nullptr, then return a new double[6] with the result.
 * result should not point to the same place as m or n.
 */
double *transform_mult(double *result,const double *a,const double *b)
{
	if (result==nullptr) result=new double[6];
	result[0]=a[0]*b[0]+a[1]*b[2];
	result[1]=a[0]*b[1]+a[1]*b[3];
	result[2]=a[2]*b[0]+a[3]*b[2];
	result[3]=a[2]*b[1]+a[3]*b[3];
	result[4]=a[4]*b[0]+a[5]*b[2]+b[4];
	result[5]=a[4]*b[1]+a[5]*b[3]+b[5];
	return result;
}

/*! Return the transform T such that a*T = b.
 * Put in result, or if result==nullptr, then return new double[6].
 * Return nullptr if a is not invertable.
 */
double *transform_diff(double *result,const double *a,const double *b)
{
	if (is_degenerate_transform(a)) return nullptr;
	if (result==nullptr) result = new double[6];

	double m[6];
	transform_invert(m,a);
	transform_mult(result, m,b);

	return result;
}

//! Rotate m by angle. If m==nullptr, then return a new'd double[6] with rotation angle.
/*! \ingroup transformmath
 * If m is supplied, then m becomes rotation*m.
 */
double *transform_rotate(double *m, double angle)
{
	if (m==nullptr) m=transform_identity(nullptr);
	double r[6],s[6];
	r[4]=r[5]=0;
	r[0]=cos(angle);
	r[2]=sin(angle);
	r[1]=-r[2]; //-sin(angle);
	r[3]=r[0]; //cos(angle);
	transform_mult(s,r,m);
	transform_copy(m,s);
	return m;
}



//! Find a transform from the given flat basis. Return new double[6] if result==nullptr.
/*! \ingroup transformmath
 */
double *transform_from_basis(double *result,flatpoint o,flatpoint x,flatpoint y)
{
	if (result==nullptr) result=new double[6]; //*** how about typedef double[6] Trans; return type Trans??
	result[0]=x.x;
	result[1]=x.y;
	result[2]=y.x;
	result[3]=y.y;
	result[4]=o.x;
	result[5]=o.y;
	return result;
}

//! Decompose a transform to an origin, x axis, and y axis.
/*! \ingroup transformmath
 */
void transform_to_basis(double *m,flatpoint *o,flatpoint *x,flatpoint *y)
{
	if (x) { (*x).x=m[0]; (*x).y=m[1]; }
	if (y) { (*y).x=m[2]; (*y).y=m[3]; }
	if (o) { (*o).x=m[4]; (*o).y=m[5]; }
}

//! ***imp me! Compose a transform from a position, x scale, y scale, rotation, and "shear".
/*! \ingroup transformmath
 * Rotation is the rotation of the x axis.
 * Shear, in this case, is taken to mean the angle that the y axis differs from
 * the transpose of the x axis.
 * y scale is the scale in the y axis AFTER shear.
 *
 * create a new double[6] if result does not exist.
 *
 * This is the reverse of transform_to_basics().
 *
 * \todo *** finish me!
 */
double *transform_from_basics(double *result,double x,double y,
								double sx, //!< size of x axis
								double sy, //!< size of y axis
								double angle, //!< rotation in radians of the xaxis from (1,0)
								double shear) //!< rotation in radians of the yaxis from transpose(xaxis)
{
	if (result==nullptr) result=new double[6];

	flatpoint xx(sx,0),yy;
	if (angle!=0) xx=rotate(xx,angle,0);
	yy=transpose(xx);
	if (shear!=0) yy=rotate(yy,shear,0);
	if (!yy.isZero()) yy*=sy/norm(yy);

	result[0]=xx.x;
	result[1]=xx.y;
	result[2]=yy.x;
	result[3]=yy.y;
	result[4]=x;
	result[5]=y;

	return result;
}

//! Decompose a transform to simple measures of position, x scale, y scale, rotation, and "shear".
/*! \ingroup transformmath
 *
 * This returns values that will produce the same m when fed back into transform_from_basics().
 */
void transform_to_basics(double *m,double *x,double *y,double *sx,double *sy,double *ang,double *shear)
{
	if (x) *x=m[4];
	if (y) *y=m[5];
	if (sx) *sx=norm(flatvector(m[0],m[1]));
	if (sy) *sy=norm(flatvector(m[2],m[3]));
	if (ang) *ang=angle(flatvector(0,0),flatvector(m[0],m[1]));
	if (shear) *shear=angle(flatvector(m[2],m[3]),transpose(flatvector(m[0],m[1])));
}

//! Simple set m[]={a,b,c,d,x0,y0}
/*! \ingroup transformmath 
 * 
 * If m==nullptr, then return a new'd double[6]
 */
double *transform_set(double *m,double a,double b,double c,double d,double x0,double y0)
{
	if (m==nullptr) m=new double[6];
	m[0]=a;
	m[1]=b;
	m[2]=c;
	m[3]=d;
	m[4]=x0;
	m[5]=y0;
	return m;
}

////! Create a transform from a string.
///*! If m==nullptr, then return a new double[6], else put result in m.
// *
// * str can be something like "1 0 0 1 0 0" or 
// */
//double *transform_set(double *m, const char *str)
//{
//	if (!m) m=new double[6];
//	***maybe have this in attributes.cc instead...
//	return m;
//}

//! Simple copy transform dest[0..5]=src[0..5]. dest and src must both exist.
/*! \ingroup transformmath */
void transform_copy(double *dest,const double *src)
{
	memcpy(dest,src,6*sizeof(double));
	//dest[0]=src[0];
	//dest[1]=src[1];
	//dest[2]=src[2];
	//dest[3]=src[3];
	//dest[4]=src[4];
	//dest[5]=src[5];
}

//! Return point p transformed by the inverse of matrix m. newpoint=[x,y,1]*m^-1
/*! \ingroup transformmath 
 * <pre>
 *      [ a  b  0 ]
 *    M=[ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 * </pre>
 */
flatpoint transform_point_inverse(const double *m,flatpoint p)
{
	double mm[6];
	transform_invert(mm,m);
	return flatpoint(mm[4] + mm[0]*p.x + mm[2]*p.y, mm[5]+mm[1]*p.x+mm[3]*p.y);
}

//! Return point transformed by the inverse of matrix m. newpoint=[x,y,1]*m^-1
/*! \ingroup transformmath 
 * <pre>
 *      [ a  b  0 ]
 *    M=[ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 * </pre>
 */
flatpoint transform_point_inverse(const double *m,double x,double y)
{
	double mm[6];
	transform_invert(mm,m);
	return flatpoint(mm[4] + mm[0]*x + mm[2]*y, mm[5]+mm[1]*x+mm[3]*y);
}

//! Return point p transformed by matrix m. newpoint=[x,y,1]*m
/*! \ingroup transformmath 
 * <pre>
 *      [ a  b  0 ]
 *    M=[ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 * </pre>
 */
flatpoint transform_point(const double *m,double x,double y)
{
	return flatpoint(m[4] + m[0]*x + m[2]*y, m[5]+m[1]*x+m[3]*y);
}

//! Return point p transformed by matrix m. newpoint=[p.x,p.y,1]*m
/*! \ingroup transformmath 
 * <pre>
 *      [ a  b  0 ]
 *    M=[ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 * </pre>
 */
flatpoint transform_point(const double *m,flatpoint p)
{
	return flatpoint(m[4] + m[0]*p.x + m[2]*p.y, m[5]+m[1]*p.x+m[3]*p.y);
}

//! Return vector p transformed by matrix m. newpoint=[p.x,p.y,0]*m. Basically rotate+scale+shear, no translate.
/*! \ingroup transformmath */
flatpoint transform_vector(const double *m,flatpoint p)
{
	return flatpoint(m[0]*p.x + m[2]*p.y,m[1]*p.x+m[3]*p.y);
}

flatpoint transform_vector_inverse(const double *m,flatpoint v)
{
	double mm[6];
	transform_invert(mm,m);
	return flatpoint(mm[0]*v.x + mm[2]*v.y,mm[1]*v.x+mm[3]*v.y);
}

/*! Basically, return norm(vx,vy)/norm(transform_point_inverse(m, vx,vy)).
 */
double get_imagnification(const double *m, double vx, double vy)
{
	flatpoint v  = transform_point_inverse(m, flatpoint(vx,vy));
	flatpoint v2 = transform_point_inverse(m, flatpoint(0,0));

    return sqrt((vx*vx + vy*vy) / ((v-v2)*(v-v2)));
}

double get_imagnification(const double *m, flatpoint v)
{
	return get_magnification(m, v.x, v.y);
}

/*! Basically, return norm(vx,vy)/norm(transform_point(m, vx,vy)).
 */
double get_magnification(const double *m, double vx, double vy)
{
	flatpoint v  = transform_point(m, vx,vy);
	flatpoint v2 = transform_point(m, 0,0);

    return sqrt((vx*vx + vy*vy) / ((v-v2)*(v-v2)));
}

double get_magnification(const double *m, flatpoint v)
{
	return get_magnification(m, v.x, v.y);
}

#define DoubleToFixed(f) ((int)((f)*65536))
#define FixedToDouble(f) (((double)(f))/65536)

//! Create as possible an affine transform from M, which has 16.16 fixed point elements.
/*! If result is nullptr, then create and return a new double[6].
 *
 * This is mainly to assist in using XTransform in the XRender extension,
 * which allows full 3x3 transformations. This might not be so useful, and
 * in the future, this function might be removed.
 * <pre>
 *      [ a  b  0 ]
 *    M=[ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 * </pre>
 */
double *transform_from_3x3_fixed(double *result,int M[3][3])
{
	if (!result) result=new double[6];
	result[0]=FixedToDouble(M[0][0]);
	result[1]=FixedToDouble(M[0][1]);
	result[2]=FixedToDouble(M[1][0]);
	result[3]=FixedToDouble(M[1][1]);
	result[4]=FixedToDouble(M[2][0]);
	result[5]=FixedToDouble(M[2][1]);
	return result;
}

//! Fill result with the affine transform m.
void transform_to_3x3_fixed(int M[3][3],double *m)
{
	M[0][2]=M[1][2]=0;
	M[0][0]=DoubleToFixed(m[0]);
	M[0][1]=DoubleToFixed(m[1]);
	M[1][0]=DoubleToFixed(m[2]);
	M[1][1]=DoubleToFixed(m[3]);
	M[2][1]=DoubleToFixed(m[4]);
	M[2][1]=DoubleToFixed(m[5]);
}

//! Based on an svg transform in v, return the equivalent 6 member affine transform.
/*! If m==nullptr, then return a new double[6]. Else assume m has room for 6 doubles, 
 * and return m.
 *
 * On error, return nullptr. If m!=nullptr, it gets set to identity on error.
 *
 * \todo this needs thorough testing
 * \todo should make the reverse too, breaking down to only scale, or only translate,
 *    if possible
 */
double *svgtransform(const char *v, double *m)
{
	if (!v) return nullptr;
	double *mm=m;
	if (!m) m=new double[6];
	transform_identity(m);
	while (isspace(*v)) v++;
	int op, n;
	double t[6],d[6];
	char *e;
	double dd;
	try {
		while (*v) {
			op=0;
			if      (!strncmp(v,"matrix",   6)) { v+=6; op=1; }
			else if (!strncmp(v,"translate",9)) { v+=9; op=2; }
			else if (!strncmp(v,"scale",    5)) { v+=5; op=3; }
			else if (!strncmp(v,"rotate",   6)) { v+=6; op=4; }
			else if (!strncmp(v,"skewX",    5)) { v+=5; op=5; }
			else if (!strncmp(v,"skewY",    5)) { v+=5; op=6; }
			else break; //no more ops, but unknown string data still, watch out!

			 //skip to numbers
			while (isspace(*v)) v++;
			if (*v!='(') throw 1;
			v++;

			 //read in real, comma or whitespace separated numbers
			n=0;
			while (n<6 && *v) {
				dd=strtod(v,&e);
				if (e==v) break;;
				d[n++]=dd;
				v=e;
				while (isspace(*v) || *v==',') v++;
			}

			 //skip to next operator
			if (*v!=')') throw 2;
			v++;
			while (isspace(*v) || *v==',') v++;
			
			transform_identity(t);
			if (op==1) { //matrix
				if (n!=6) throw 3;
				transform_copy(t,d);

			} else if (op==2) { //translate(dx) (dy==0) or translate(dx,dy)
				if (n!=1 && n!=2) throw 4;
				t[4]=d[0];
				if (n==2) t[5]=d[1];

			} else if (op==3) { //scale(x,y) or scale(z)<-- same as scale(z,z)
				if (n != 1 && n != 2) throw 5;
				t[0] = d[0];
				if (n == 2) t[3] = d[1];
				else        t[3] = d[0];

			} else if (op==4) { //rotate(degrees) or rotate(degrees, x,y)
				if (n != 1 && n != 3) throw 6;
				d[0] *= -M_PI / 180;  // convert to degrees

				// if (n==3) translate(dx, dy) rotate(angle) translate(-dx, -dy)
				double dx = 0, dy = 0;
				if (n == 3) {
					dx = d[1];
					dy = d[2];
				}
				 //translate
				m[4] -= dx;
				m[5] -= dy;
				// rotate
				t[0] = cos(d[0]);
				t[2] = sin(d[0]);
				t[1] = -t[2];
				t[3] = t[0];
				transform_mult(d, m, t);
				transform_copy(m, d);
				// translate back
				m[4] += dx;
				m[5] += dy;
				op = 0;

			} else if (op==5) { //skewX(degrees)
				if (n!=1) throw 7;
				t[2]=tan(d[0]*M_PI/180);

			} else if (op==6) { //skewY(degrees)
				if (n!=1) throw 8;
				t[1]=tan(d[0]*M_PI/180);
			}
			 //apply transform t to m
			if (op) { //special exception for rotate around point
				transform_mult(d,t,m);
				transform_copy(m,d);
			}
		}
	} catch(...) {
		if (m!=mm) delete[] m; //m was created above, original m was nullptr
		else transform_identity(m);

		return nullptr;
	}
	return m;
}


} // namespace Laxkit

