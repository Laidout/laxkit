//
// New spline curve from Raph Levien.
//
// C++ port by Tom Lechner, 2019
// This file is released under the same conditions as the original stated below.
//

// 
// The original javascript can be found here:
// https://github.com/raphlinus/spline-research
// 
// Copyright 2018 Raph Levien
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//


#include <cmath>
#include <random>
#include <iostream>

#include "rspline.h"



namespace NewSpiro {


#define MOVETO    'M'
#define LINETO    'L'
#define CURVETO   'C'
#define CLOSEPATH 'Z'
#define MARK      '#'



//------------------------------- Misc funcs ------------------------------------


#define MIN(x,y) ((x) < (y) ? (x) : (y))

std::ostream &operator<<(std::ostream &os, Vec2 const &v) { return os << v.x << ',' << v.y; }


#define MathSign(X) (X > 0 ? 1 : X < 0 ? -1 : 0)

// normalize theta to -pi..pi
double mod2pi(double th)
{
	double twopi = 2 * M_PI;
	double frac = th * (1 / twopi);
	return twopi * (frac - round(frac)); 
}

double myTan(double th)
{
	if (th > M_PI / 2) {
		return tan(M_PI - th);
	} else if (th < -M_PI / 2) {
		return tan(-M_PI - th);
	} else {
		return tan(th);
	}
}

inline double hypot(double x, double y)
{
	return sqrt(x*x + y*y);
}

double MathRandom()
{
	return (double)random()/RAND_MAX;
}

void testCubicBez()
{
	double c[8];
	for (int i = 0; i < 8; i++) {
		c[i] = MathRandom();
	}
	CubicBez cb(c);
	double t = MathRandom();
	double epsilon = 1e-6;
	Vec2 xy0;
	Vec2 xy1;
	cb.eval(t, &xy0);
	cb.eval(t + epsilon, &xy1);

	std::cout << (Vec2((xy1.x - xy0.x) / epsilon, (xy1.y - xy0.y) / epsilon)) << std::endl;
	Vec2 temp;
	cb.deriv(t, &temp);
	std::cout << temp << std::endl;

	Vec2 dxy0;
	Vec2 dxy1;
	cb.deriv(t, &dxy0);
	cb.deriv(t + epsilon, &dxy1);

	std::cout << Vec2((dxy1.x - dxy0.x) / epsilon, (dxy1.y - dxy0.y) / epsilon) << std::endl;
	cb.deriv2(t, &temp);
	std::cout << temp << std::endl;
}

/// Solve tridiagonal matrix system. Destroys inputs, leaves output in x.
///
/// Solves a[i] * x[i - 1] + b[i] * x[i] + c[i] * x[i + 1] = d[i]
///
/// Inputs are array-like objects (typed arrays are good for performance).
///
/// Note: this is not necessarily the fastest, see:
/// https://en.wikibooks.org/wiki/Algorithm_Implementation/Linear_Algebra/Tridiagonal_matrix_algorithm
void tridiag(std::vector<double> &a, std::vector<double> &b, std::vector<double> &c, std::vector<double> &d, std::vector<double> &x, int len)
{
	int n = len;
	for (int i = 1; i < n; i++) {
		double m = a[i] / b[i - 1];
		b[i] -= m * c[i - 1];
		d[i] -= m * d[i - 1];
	}
	x[n - 1] = d[n - 1] / b[n - 1];
	for (int i = n - 2; i >= 0; i--) {
		x[i] = (d[i] - c[i] * x[i + 1]) / b[i];
	}
}

//void testTridiag(int n)
//{
//	double a[n];
//	double b[n];
//	double c[n];
//	double d[n];
//	double x[n];
//
//	for (int i = 0; i < n; i++) {
//		a[i] = MathRandom();
//		b[i] = 2 + MathRandom();
//		c[i] = MathRandom();
//		d[i] = MathRandom();
//		x[i] = MathRandom();
//	}
//
//	double bsave[n];
//	double dsave[n];
//	double xsave[n];
//
//	for (int i=0; i<n; i++) {
//		bsave[i] = b[i];
//		dsave[i] = d[i];
//		xsave[i] = x[i];
//	}
//
//	tridiag(a, b, c, d, x, n);
//
//	for (int i=0; i<n; i++) {
//		b[i] = bsave[i];
//		d[i] = dsave[i];
//	}
//
//	std::cout << (b[0] * x[0] + c[0] * x[1] - d[0]) << std::endl;
//	for (int i = 1; i < n - 1; i++) {
//		std::cout << (a[i] * x[i - 1] + b[i] * x[i] + c[i] * x[i + 1] - d[i]) << std::endl;
//	}
//	std::cout << (a[n - 1] * x[n - 2] + b[n - 1] * x[n - 1] - d[n - 1]) << std::endl;
//}
void testTridiag(int n)
{
	std::vector<double> a(n);
	std::vector<double> b(n);
	std::vector<double> c(n);
	std::vector<double> d(n);
	std::vector<double> x(n);

	for (int i = 0; i < n; i++) {
		a[i] = MathRandom();
		b[i] = 2 + MathRandom();
		c[i] = MathRandom();
		d[i] = MathRandom();
		x[i] = MathRandom();
	}

	std::vector<double> bsave = b;
	std::vector<double> dsave = d;
	std::vector<double> xsave = x;

	tridiag(a, b, c, d, x, n);

	b = bsave;
	d = dsave;

	std::cout << (b[0] * x[0] + c[0] * x[1] - d[0]) << std::endl;
	for (int i = 1; i < n - 1; i++) {
		std::cout << (a[i] * x[i - 1] + b[i] * x[i] + c[i] * x[i + 1] - d[i]) << std::endl;
	}
	std::cout << (a[n - 1] * x[n - 2] + b[n - 1] * x[n - 1] - d[n - 1]) << std::endl;
}

//testTridiag(10);
//testCubicBez();


double myCubicLen(double th0, double th1)
{
	double offset = 0.3 * sin(th1 * 2 - 0.4 * sin(th1 * 2));
	bool newShape = true;
	if (newShape) {
		double scale = 1.0 / (3 * 0.8);
		double len = scale * (cos(th0 - offset) - 0.2 * cos((3 * (th0 - offset))));
		return len;
	} else {
		double drive = 2.0;
		double scale = 1.0 / (3 * tanh(drive));
		double len = scale * tanh(drive * cos(th0 - offset));
		return len;
	}
}

/// Create a smooth cubic bezier. coords must be a double[8].
void myCubic(double th0, double th1, double *coords)
{
	//var coords = new Float64Array(8);
	double len0 = myCubicLen(th0, th1);
	coords[2] = cos(th0) * len0;
	coords[3] = sin(th0) * len0;

	double len1 = myCubicLen(th1, th0);
	coords[4] = 1 - cos(th1) * len1;
	coords[5] = sin(th1) * len1;
	coords[6] = 1;
}


//-------------------------------- BezPath and BezPath::BezCommand ---------------------------------

/*! \class BezPath
 * A list of cubic bezier commands, in particular moveto, lineto, curveto, closepath.
 * Also a special mark command used elsewhere.
 */

/* \class BezPath::BezCommand
 */

BezPath::BezCommand::BezCommand(const char cmd, double v1, double v2, double v3, double v4, double v5, double v6)
{
	command = cmd;
	val.resize(6);
	val[0] = v1;
	val[1] = v2;
	val[2] = v3;
	val[3] = v4;
	val[4] = v5;
	val[5] = v6;
}
BezPath::BezCommand::BezCommand(const char cmd, double v1, double v2)
{
	command = cmd;
	val.resize(2);
	val[0] = v1;
	val[1] = v2;
}

BezPath::BezCommand::BezCommand(const char cmd, double v1)
{
	command = cmd;
	val.resize(1);
	val[0] = v1;
}

BezPath::BezCommand::BezCommand(const char cmd)
{
	command = cmd;
}



// Construction mutations (builder could be separate but oh well).
void BezPath::moveto(double x, double y)
{
	commands.push_back(BezCommand(MOVETO, x, y));
}

void BezPath::lineto(double x, double y)
{
	commands.push_back(BezCommand(LINETO, x, y));
}

void BezPath::curveto(double x1, double y1, double x2, double y2, double x3, double y3)
{
	commands.push_back(BezCommand(CURVETO, x1, y1, x2, y2, x3, y3));
}

void BezPath::closepath()
{
	commands.push_back(BezCommand(CLOSEPATH));
}

void BezPath::mark(int i)
{
	commands.push_back(BezCommand(MARK, i));
}

std::string BezPath::renderSvg()
{
	std::string path = "";
	for (BezCommand &cmd : commands) {
		const char op = cmd.command;
		if (op == MOVETO) {
			path += " M" + std::to_string(cmd.val[0]) + " " + std::to_string(cmd.val[1]);

		} else if (op == LINETO) {
			path += " L" + std::to_string(cmd.val[0]) + " " + std::to_string(cmd.val[1]);

		} else if (op == CURVETO) {
			path += " C"
					+ std::to_string(cmd.val[0]) + " " + std::to_string(cmd.val[1]) + " "
					+ std::to_string(cmd.val[2]) + " " + std::to_string(cmd.val[3]) + " "
					+ std::to_string(cmd.val[4]) + " " + std::to_string(cmd.val[5]) + " ";

		} else if (op == CLOSEPATH) {
			path += "Z";
		}
	}
	return path;
}

void BezPath::hitTest(double x, double y, HitTestResult *result)
{
	result->x = x;
	result->y = y;

	double curX = 0;
	double curY = 0;
	int curMark = -1;

	for (BezCommand &cmd : commands) {
		char op = cmd.command;

		if (op == MOVETO) {
			curX = cmd.val[0];
			curY = cmd.val[1];

		} else if (op == LINETO) {
			result->accumLine(curX, curY, cmd.val[0], cmd.val[1], curMark);
			curX = cmd.val[0];
			curY = cmd.val[1];

		} else if (op == CURVETO) {
			result->accumCurve(curX, curY, cmd.val[0], cmd.val[1], cmd.val[2], cmd.val[3],
				cmd.val[4], cmd.val[5], curMark);
			curX = cmd.val[4];
			curY = cmd.val[5];

		} else if (op == MARK) {
			curMark = cmd.val[0];
		}
	}
}


//-------------------------------- HitTestResult ---------------------------------

/*! \class HitTestResult
 */

HitTestResult::HitTestResult()
{
	x = y = 0;
	bestDist = 1e12;
	bestMark = -1;
}

HitTestResult::HitTestResult(double x, double y)
{
	this->x = x;
	this->y = y;
	this->bestDist = 1e12;
	this->bestMark = -1;
}

void HitTestResult::accumulate(double dist, /*pt,*/ int mark)
{
	if (dist < this->bestDist) {
		this->bestDist = dist;
		this->bestMark = mark;
	}
}

void HitTestResult::accumLine(double x0, double y0, double x1, double y1, int mark)
{
	double dx = x1 - x0;
	double dy = y1 - y0;
	double dotp = (this->x - x0) * dx + (this->y - y0) * dy;
	double linDotp = dx * dx + dy * dy;
	double r = hypot(this->x - x0, this->y - y0);
	double rMin = r;
	r = hypot(this->x - x1, this->y - y1);
	rMin = MIN(rMin, r);
	if (dotp > 0 && dotp < linDotp) {
		double norm = (this->x - x0) * dy - (this->y - y0) * dx;
		r = fabs(norm / sqrt(linDotp));
		rMin = MIN(rMin, r);
	}
	if (rMin < this->bestDist) {
		this->bestDist = rMin;
		this->bestMark = mark;
	}
}

void HitTestResult::accumCurve(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int mark)
{
	int n = 32; // TODO: be adaptive
	double dt = 1.0 / n;
	double lastX = x0;
	double lastY = y0;

	for (int i = 0; i < n; i++) {
		double t = (i + 1) * dt;
		double mt = 1 - t;
		double x = (x0 * mt * mt + 3 * (x1 * mt * t + x2 * t * t)) * mt + x3 * t * t * t;
		double y = (y0 * mt * mt + 3 * (y1 * mt * t + y2 * t * t)) * mt + y3 * t * t * t;
		this->accumLine(lastX, lastY, x, y, mark);
		lastX = x;
		lastY = y;
	}
}


//------------------------------- Polynomial ------------------------------------

typedef unsigned int uint;

class Polynomial
{
  public:
	std::vector<double> coeff;

	Polynomial() {}

	Polynomial(std::vector<double> &c)
	{
		coeff = c;
	}

	~Polynomial() {}

	void Size(unsigned int nlen) 
	{
		if (nlen == coeff.size()) return;
		coeff.resize(nlen);
	}

	double eval(double x)
	{
		double xi = 1;
		double s = 0;
		for (uint i=0; i<coeff.size(); i++) {
			s += coeff[i] * xi;
			xi *= x;
		}
		return s;
	}

	void deriv(Polynomial &ret)
	{
		ret.Size(coeff.size()-1);
		for (uint i = 0; i < ret.coeff.size(); i++) {
			ret.coeff[i] = (i + 1) * this->coeff[i + 1];
		}
	}
};

void hermite5(double x0, double x1, double v0, double v1, double a0, double a1, Polynomial &ret)
{
	ret.Size(6);
	ret.coeff[0] = x0;
	ret.coeff[1] = v0;
	ret.coeff[2] = 0.5 * a0;
	ret.coeff[3] = -10 * x0 + 10 * x1 - 6 * v0 - 4 * v1 - 1.5 * a0 + 0.5 * a1;
	ret.coeff[4] = 15 * x0 - 15 * x1 + 8 * v0 + 7 * v1 + 1.5 * a0 - a1;
	ret.coeff[5] = -6 * x0 + 6 * x1 - 3 * v0 - 3 * v1 + -.5 * a0 + 0.5 * a1;
}


//------------------------------- Class Vec2 ------------------------------------

/*! \class Vec2
 * A simple container for 2-vectors.
 */

double Vec2::norm()
{
	return sqrt(x*x + y*y);
}

double Vec2::dot(const Vec2 &other) {
	return x * other.x + y * other.y;
}

double Vec2::cross(const Vec2 &other) {
	return x * other.y - y * other.x;
}


//------------------------------- Class CubicBez ------------------------------------

/*! \class CubicBez
 * A single cubic bezier segment defined by two endpoints, and two inner control points.
 */

CubicBez::CubicBez()
{
	for (int i=0; i<8; i++) c[i] = 0;
}

CubicBez::CubicBez(double *coords)
{
	for (int i=0; i<8; i++) c[i] = coords[i];
}

void CubicBez::weightsum(double c0, double c1, double c2, double c3, Vec2 *vec2_ret)
{
	double x = c0 * this->c[0] + c1 * this->c[2] + c2 * this->c[4] + c3 * this->c[6];
	double y = c0 * this->c[1] + c1 * this->c[3] + c2 * this->c[5] + c3 * this->c[7];
	//return new Vec2(x, y);
	vec2_ret->x = x;
	vec2_ret->y = y;
}

void CubicBez::eval(double t, Vec2 *vec2_ret)
{
	double mt = 1 - t;
	double c0 = mt * mt * mt;
	double c1 = 3 * mt * mt * t;
	double c2 = 3 * mt * t * t;
	double c3 = t * t * t;
	//return this->weightsum(c0, c1, c2, c3);
	weightsum(c0, c1, c2, c3, vec2_ret);
}

void CubicBez::deriv(double t, Vec2 *vec2_ret)
{
	double mt = 1 - t;
	double c0 = -3 * mt * mt;
	double c3 = 3 * t * t;
	double c1 = -6 * t * mt - c0;
	double c2 = 6 * t * mt - c3;
	//return this->weightsum(c0, c1, c2, c3);
	weightsum(c0, c1, c2, c3, vec2_ret);
}

void CubicBez::deriv2(double t, Vec2 *vec2_ret)
{
	double mt = 1 - t;
	double c0 = 6 * mt;
	double c3 = 6 * t;
	double c1 = 6 - 18 * mt;
	double c2 = 6 - 18 * t;
	//return this->weightsum(c0, c1, c2, c3);
	weightsum(c0, c1, c2, c3, vec2_ret);
}

double CubicBez::curvature(double t)
{
	Vec2 d;
	Vec2 d2;
	deriv(t, &d);
	deriv2(t, &d2);
	return d.cross(d2) / pow(d.norm(), 3);
}

double CubicBez::atanCurvature(double t)
{
	Vec2 d;
	Vec2 d2;
	deriv(t, &d);
	deriv2(t, &d2);
	return atan2(d.cross(d2), pow(d.norm(), 3));
}

// de Casteljau's algorithm
void CubicBez::leftHalf(CubicBez *ret)
{
	ret->c[0] = this->c[0];
	ret->c[1] = this->c[1];
	ret->c[2] = 0.5 * (this->c[0] + this->c[2]);
	ret->c[3] = 0.5 * (this->c[1] + this->c[3]);
	ret->c[4] = 0.25 * (this->c[0] + 2 * this->c[2] + this->c[4]);
	ret->c[5] = 0.25 * (this->c[1] + 2 * this->c[3] + this->c[5]);
	ret->c[6] = 0.125 * (this->c[0] + 3 * (this->c[2] + this->c[4]) + this->c[6]);
	ret->c[7] = 0.125 * (this->c[1] + 3 * (this->c[3] + this->c[5]) + this->c[7]);
}

void CubicBez::rightHalf(CubicBez *ret)
{
	ret->c[0] = 0.125 * (this->c[0] + 3 * (this->c[2] + this->c[4]) + this->c[6]);
	ret->c[1] = 0.125 * (this->c[1] + 3 * (this->c[3] + this->c[5]) + this->c[7]);
	ret->c[2] = 0.25 * (this->c[2] + 2 * this->c[4] + this->c[6]);
	ret->c[3] = 0.25 * (this->c[3] + 2 * this->c[5] + this->c[7]);
	ret->c[4] = 0.5 * (this->c[4] + this->c[6]);
	ret->c[5] = 0.5 * (this->c[5] + this->c[7]);
	ret->c[6] = this->c[6];
	ret->c[7] = this->c[7];
}




//------------------------------- TwoParamCurve ------------------------------------

/*! \class TwoParamCurve
 * Base class for two parameter curve families... What on earth does that even mean?
 */

void TwoParamCurve::computeCurvatureDerivs(double th0, double th1, CurvatureDerivs *derivs_ret)
{
	double epsilon = 1e-6;
	double scale = 2.0 / epsilon;

	double k0plus[2];
	double k0minus[2];
	computeCurvature(th0 + epsilon, th1, k0plus);
	computeCurvature(th0 - epsilon, th1, k0minus);

	//double dak0dth0 = scale * (k0plus.ak0 - k0minus.ak0);
	//double dak1dth0 = scale * (k0plus.ak1 - k0minus.ak1);
	double dak0dth0 = scale * (k0plus[0] - k0minus[0]);
	double dak1dth0 = scale * (k0plus[1] - k0minus[1]);

	double k1plus[2];
	double k1minus[2];
	computeCurvature(th0, th1 + epsilon, k1plus);
	computeCurvature(th0, th1 - epsilon, k1minus);

	double dak0dth1 = scale * (k1plus[0] - k1minus[0]);
	double dak1dth1 = scale * (k1plus[1] - k1minus[1]);

	//return {dak0dth0: dak0dth0, dak1dth0: dak1dth0, dak0dth1: dak0dth1, dak1dth1: dak1dth1};
	derivs_ret->dak0dth0 = dak0dth0;
	derivs_ret->dak1dth0 = dak1dth0;
	derivs_ret->dak0dth1 = dak0dth1;
	derivs_ret->dak1dth1 = dak1dth1;
}



//------------------------------ MyCurve ------------------------------------

/*! \class MyCurve
 */

std::vector<Vec2> MyCurve::render(double th0, double th1) //render supposed to return 3n-1 Vec2 points
{
	double c[8];
	myCubic(th0, th1, c);
	std::vector<Vec2> pts = { Vec2(c[2], c[3]), Vec2(c[4], c[5]) };
	//return [new Vec2(c[2], c[3]), new Vec2(c[4], c[5])];
	return pts;
}

/// Render as a 4-parameter curve with optional adjusted endpoint curvatures.
std::vector<Vec2> MyCurve::render4Quintic(double th0, double th1, bool hasK0, double k0, bool hasK1, double k1)
{
	////let cb = new CubicBez(myCubic(th0, th1));
	//let cb = this->convCubic(this->render4Cubic(th0, th1, k0, k1));
	//---
	CubicBez cb;
	this->convCubic(this->render4Cubic(th0, th1, hasK0, k0, hasK1, k1), cb);


	// compute second deriv tweak to match curvature
	//function curvAdjust(t, th, k) {
	auto curvAdjust = [&](double t, double th, bool hasK, double k) {
		if (!hasK) return Vec2(0, 0);
		double c = cos(th);
		double s = sin(th);
		Vec2 d2;
		cb.deriv2(t, &d2);
		double d2cross = d2.y * c - d2.x * s;
		Vec2 d;
		cb.deriv(t, &d);
		double ddot = d.x * c + d.y * s;
		// TODO: if ddot = 0, cusp, no adjustment
		double oldK = d2cross / (ddot * ddot);
		double kAdjust = k - oldK;
		double aAdjust = kAdjust * (ddot * ddot);
		return Vec2(-s * aAdjust, c * aAdjust);
	};

	Vec2 a0 = curvAdjust(0,  th0, hasK0, k0);
	Vec2 a1 = curvAdjust(1, -th1, hasK1, k1);
	Polynomial hx;
	hermite5(0, 0, 0, 0, a0.x, a1.x, hx);
	Polynomial hy;
	hermite5(0, 0, 0, 0, a0.y, a1.y, hy);
	Polynomial hxd;
	hx.deriv(hxd);
	Polynomial hyd;
	hy.deriv(hyd);

	// This really would be cleaner if we had arbitrary deCasteljau...
	CubicBez c0;
	cb.leftHalf(&c0);
	CubicBez c1;
	cb.rightHalf(&c1);
	CubicBez cs[4];
	//let cs = [c0.leftHalf(), c0.rightHalf(), c1.leftHalf(), c1.rightHalf()];
	c0.leftHalf (&(cs[0]));
	c0.rightHalf(&(cs[1]));
	c1.leftHalf (&(cs[2]));
	c1.rightHalf(&(cs[3]));

	std::vector<Vec2> result;
	double scale = 1./12;
	for (int i = 0; i < 4; i++) {
		double t = 0.25 * i;
		double t1 = t + 0.25;
		double *c = cs[i].c;
		double x0 = hx.eval(t);
		double y0 = hy.eval(t);
		double x1 = x0 + scale * hxd.eval(t);
		double y1 = y0 + scale * hyd.eval(t);
		double x3 = hx.eval(t1);
		double y3 = hy.eval(t1);
		double x2 = x3 - scale * hxd.eval(t1);
		double y2 = y3 - scale * hyd.eval(t1);
		if (i != 0) {
			result.push_back(Vec2(c[0] + x0, c[1] + y0));
		}
		result.push_back(Vec2(c[2] + x1, c[3] + y1));
		result.push_back(Vec2(c[4] + x2, c[5] + y2));
	}
	return result;
}

void MyCurve::convCubic(const std::vector<Vec2> &pts, CubicBez &bez)
{
	bez.c[2] = pts[0].x;
	bez.c[3] = pts[0].y;
	bez.c[4] = pts[1].x;
	bez.c[5] = pts[1].y;
	bez.c[6] = 1;
}

// Ultimately we want to exactly match the endpoint curvatures (probably breaking
// into two cubic segments), but for now, just approximate...
std::vector<Vec2> MyCurve::render4Cubic(double th0, double th1, bool hasK0, double k0, bool hasK1, double k1)
{
	CubicBez cb;
	myCubic(th0, th1, cb.c);
	std::vector<Vec2> result;

	auto deriv_scale = [&](double t, double th, bool hasK, double k) {
		if (!hasK) return 1/3.;
		double c = cos(th);
		double s = sin(th);
		Vec2 d;
		cb.deriv(t, &d);
		Vec2 d2;
		cb.deriv2(t, &d2);
		double d2cross = d2.y * c - d2.x * s;
		double ddot = d.x * c + d.y * s;
		double oldK = d2cross / (ddot * ddot);
		// fudge to avoid divide-by-zero
		if (fabs(oldK) < 1e-6) oldK = 1e-6;
		double ratio = k / oldK;
		// TODO: fine tune this dodgy formula
		//let scale = ratio < 1 ? 1/2 - ratio/6 : 1/(3*ratio);
		double scale = 1/(2 + ratio);
		return scale;
	};

	double scale0 = deriv_scale(0, th0, hasK0, k0);
	Vec2 d0;
	cb.deriv(0, &d0);
	result.push_back(Vec2(d0.x * scale0, d0.y * scale0));
	Vec2 d1;
	cb.deriv(1, &d1);
	double scale1 = deriv_scale(1, -th1, hasK1, k1);
	result.push_back(Vec2(1 - d1.x * scale1, - d1.y * scale1));
	return result;
}

//shared_ptr<BezPath> ...
std::vector<Vec2> MyCurve::render4(double th0, double th1, bool hasK0, double k0, bool hasK1, double k1)
{
	if (!hasK0 && !hasK1) {
		return this->render(th0, th1);
	}
	return this->render4Quintic(th0, th1, hasK0, k0, hasK1, k1);
}

/// curvature_ret is a double[2]
void MyCurve::computeCurvature(double th0, double th1, double *curvature_ret)
{
	CubicBez cb;
	myCubic(th0, th1, cb.c);

	auto curv = [&](double t, double th)
	{
		double c = cos(th);
		double s = sin(th);
		Vec2 d2;
		cb.deriv2(t, &d2);
		double d2cross = d2.y * c - d2.x * s;
		Vec2 d;
		cb.deriv(t, &d);
		double ddot = d.x * c + d.y * s;
		return atan2(d2cross, ddot * fabs(ddot));
	};

	//let ak0 = cb.atanCurvature(0);
	//let ak1 = cb.atanCurvature(1);
	double ak0 = curv(0, th0);
	double ak1 = curv(1, -th1);

	//return {ak0: ak0, ak1: ak1};
	curvature_ret[0] = ak0;
	curvature_ret[1] = ak1;
}

double MyCurve::endpointTangent(double th)
{
	// Same value as parabola:
	//return Math.atan(2 * Math.tan(th)) - th;

	return 0.5 * sin(2 * th);
}




//------------------------------ TwoParamSpline ------------------------------------

/*! \class TwoParamSpline
 */

TwoParamSpline::TwoParamSpline(std::shared_ptr<TwoParamCurve> curve, std::vector<Vec2> &ctrlPts)
{
	this->curve = curve;
	this->ctrlPts = ctrlPts;

	hasStartTh = hasEndTh = false;
	startTh = endTh = 0;
}

/// Determine initial tangent angles, given array of Vec2 control points.
void TwoParamSpline::initialThs()
{
	if (ths.size() != ctrlPts.size()) {
		ths.resize(ctrlPts.size());
	}

	for (uint i = 1; i < ths.size() - 1; i++) {
		double dx0 = this->ctrlPts[i].x - this->ctrlPts[i - 1].x;
		double dy0 = this->ctrlPts[i].y - this->ctrlPts[i - 1].y;
		double l0 = hypot(dx0, dy0);
		double dx1 = this->ctrlPts[i + 1].x - this->ctrlPts[i].x;
		double dy1 = this->ctrlPts[i + 1].y - this->ctrlPts[i].y;
		double l1 = hypot(dx1, dy1);
		double th0 = atan2(dy0, dx0);
		double th1 = atan2(dy1, dx1);
		double bend = mod2pi(th1 - th0);
		double th = mod2pi(th0 + bend * l0 / (l0 + l1));
		ths[i] = th;
		if (i == 1) { ths[0] = th0; }
		if (i == ths.size() - 2) { ths[i + 1] = th1; }
	}
	if (hasStartTh) {
		ths[0] = this->startTh;
	}
	if (hasEndTh) {
		ths[ths.size() - 1] = this->endTh;
	}
}

/// Get tangent angles relative to endpoints, and chord length.
void TwoParamSpline::getThs(int i, double *th0_ret, double *th1_ret, double *chord_ret)
{
	double dx = this->ctrlPts[i + 1].x - this->ctrlPts[i].x;
	double dy = this->ctrlPts[i + 1].y - this->ctrlPts[i].y;
	double th =  atan2(dy, dx);
	double th0 = mod2pi(this->ths[i] - th);
	double th1 = mod2pi(th - this->ths[i + 1]);
	double chord = hypot(dx, dy);

	*th0_ret = th0;
	*th1_ret = th1;
	*chord_ret = chord;
}

double TwoParamSpline::computeErr(double th0,double th1,double chord, double *ak0, double th0_1, double th1_1, double chord_1, double *ak1)
{
	// rescale tangents by geometric mean of chordlengths
	double ch0 = sqrt(chord);
	double ch1 = sqrt(chord_1);
	double a0  = atan2(sin(ak0[1]) * ch1, cos(ak0[1]) * ch0);
	double a1  = atan2(sin(ak1[0]) * ch0, cos(ak1[0]) * ch1);
	return a0 - a1;
}

/// Crawl towards a curvature continuous solution. Returns error amount.
double TwoParamSpline::iterDumb(int iter)
{
	int n = this->ctrlPts.size();
	double th0, th1, chord;

	// Fix endpoint tangents; we rely on iteration for this to converge
	if (!this->hasStartTh) {
		this->getThs(0, &th0, &th1, &chord);
		this->ths[0] += this->curve->endpointTangent(th1) - th0;
	}

	//if (this->endTh === null) {
	if (!this->hasEndTh) {
		this->getThs(n - 2, &th0, &th1, &chord);
		this->ths[n - 1] -= this->curve->endpointTangent(th0) - th1;
	}
	if (n < 3) return 0;

	double absErr = 0;
	double x[n - 2];
	this->getThs(0, &th0, &th1, &chord);
	double ak0[2];
	this->curve->computeCurvature(th0, th1, ak0);
	//console.log('');
	
	double epsilon = 1e-3;

	for (int i = 0; i < n - 2; i++) {
		double th0_1, th1_1, chord_1; 
		this->getThs(i + 1, &th0_1, &th1_1, &chord_1);
		double ak1[2];
		this->curve->computeCurvature(th0_1, th1_1, ak1);
		double err = computeErr(th0,th1,chord, ak0, th0_1,th1_1,chord_1, ak1);
		absErr += fabs(err);

		double ak0p[2];
		this->curve->computeCurvature(th0, th1 + epsilon, ak0p);
		double ak1p[2];
		this->curve->computeCurvature(th0_1 - epsilon, th1_1, ak1p);
		double errp = computeErr(th0,th1,chord, ak0p, th0_1,th1_1,chord_1, ak1p);
		double derr = (errp - err) * (1 / epsilon);
		//console.log(err, derr, ak0, ak1, ak0p, ak1p);
		x[i] = err / derr;

		//ths0 = ths1;
		th0 = th0_1;
		th1 = th1_1;
		chord = chord_1;

		//ak0 = ak1;
		ak0[0] = ak1[0];
		ak0[1] = ak1[1];
	}

	for (int i = 0; i < n - 2; i++) {
		double scale = tanh(0.25 * (iter + 1));
		this->ths[i + 1] += scale * x[i];
	}

	return absErr;
}

///// Perform one step of a Newton solver.
//// Not yet implemented
//TwoParamSpline::iterate() {
//	let n = this->ctrlPts.length;
//	if (n < 3) return;
//	var a = new Float64Array(n - 2);
//	var b = new Float64Array(n - 2);
//	var c = new Float64Array(n - 2);
//	var d = new Float64Array(n - 2);
//	var x = new Float64Array(n - 2);
//
//	let ths0 = this->getThs(0);
//	var last_ak = this->curve.computeCurvature(ths0.th0, ths0.th1);
//	var last_dak = this->curve.computeCurvatureDerivs(ths0.th0, ths0.th1);
//	var last_a = Math.hypot(this->ctrlPts[1].x - this->ctrlPts[0].x,
//		this->ctrlPts[1].y - this->ctrlPts[0].y);
//	for (var i = 0; i < n - 2; i++) {
//		let ths = this->getThs(i + 1);
//		let ak = this->curve.computeCurvature(ths.th0, ths.th1);
//		let dak = this->curve.computeCurvatureDerivs(ths.th0, ths.th1);
//		var a = Math.hypot(this->ctrlPts[i + 2].x - this->ctrlPts[i + 1].x,
//			this->ctrlPts[i + 2].y - this->ctrlPts[i + 1].y);
//		let c0 = Math.cos(last_ak.ak1);
//		let s0 = Math.sin(last_ak.ak1);
//		let c1 = Math.cos(ak.ak0);
//		let s1 = Math.sin(ak.ak0);
//
//		// TODO: fill in derivatives properly
//		d[i] = a * s0 * c1 - last_a * s1 * c0;
//
//		last_ak = ak;
//		last_dak = dak;
//		last_a = a;
//	}
//
//	tridiag(a, b, c, d, x);
//	for (var i = 0; i < n - 2; i++) {
//		this->ths[i + 1] -= x[i];
//	}
//}

/// Return an SVG path string.
std::string TwoParamSpline::renderSvg()
{
	std::vector<Vec2> &c = this->ctrlPts;
	if (c.size() == 0) { return ""; }
	std::string path = "M" + std::to_string(c[0].x) + " " + std::to_string(c[0].y);
	std::string cmd = " C";

	double th0;
	double th1;
	double chord;

	for (uint i = 0; i < c.size() - 1; i++) {
		this->getThs(i, &th0, &th1, &chord);
		std::vector<Vec2> render = this->curve->render(th0, th1);
		double dx = c[i + 1].x - c[i].x;
		double dy = c[i + 1].y - c[i].y;

		for (uint j = 0; j < render.size(); j++) {
			Vec2 &pt = render[j];
			double x = c[i].x + dx * pt.x - dy * pt.y;
			double y = c[i].y + dy * pt.x + dx * pt.y;
			path += cmd + std::to_string(x) + " " + std::to_string(y);
			cmd = " ";
		}
		path += " " + std::to_string(c[i + 1].x) + " " + std::to_string(c[i + 1].y);
	}

	return path;

}


//------------------------------ Spline ------------------------------------

/*! \class Spline
 *  Handles more general cases, including corners.
 */

Spline::Spline()
{
	isClosed = false;
	this->curve = std::make_shared<MyCurve>();
}

Spline::Spline(std::vector<ControlPoint> &ctrlPts, bool isClosed)
{
	this->ctrlPts = ctrlPts;
	this->isClosed = isClosed;
	this->curve = std::make_shared<MyCurve>();
}

ControlPoint &Spline::pt(int i, int start)
{
	int length = ctrlPts.size();
	return ctrlPts[(i + start + length) % length];
}

int Spline::StartIndex()
{
	if (!isClosed) {
		return 0;
	}
	for (uint i = 0; i < ctrlPts.size(); i++) {
		ControlPoint &pt = ctrlPts[i];
		if (pt.ty == 'c' || pt.hasLth) {
			return i;
		}
	}
	// Path is all-smooth and closed.
	return 0;
}

void Spline::solve()
{
	int start = StartIndex();
	int length = ctrlPts.size() - (isClosed ? 0 : 1);
	int i = 0;

	while (i < length) {
		ControlPoint &ptI = pt(i, start);
		ControlPoint &ptI1 = pt(i + 1, start);

		if ((i + 1 == length || ptI1.ty == 'c') && !ptI.hasRth && !ptI1.hasLth) {
			//corner
			double dx = ptI1.pt.x - ptI.pt.x;
			double dy = ptI1.pt.y - ptI.pt.y;
			double th = atan2(dy, dx);
			ptI.rth = th;
			ptI1.lth = th;
			i += 1;

		} else {
			// We have a curve.
			std::vector<Vec2> innerPts;
			innerPts.push_back(ptI.pt);
			int j = i + 1;
			while (j < length + 1) {
				ControlPoint &ptJ = this->pt(j, start);
				innerPts.push_back(ptJ.pt);
				j += 1;
				if (ptJ.ty == 'c' || ptJ.hasLth) {
					break;
				}
			}

			//console.log(innerPts);
			TwoParamSpline inner(this->curve, innerPts);
			inner.startTh    = this->pt(i, start).rth;
			inner.hasStartTh = this->pt(i, start).hasRth;
			inner.endTh    = this->pt(j - 1, start).lth;
			inner.hasEndTh = this->pt(j - 1, start).hasLth;
			int nIter = 10;
			inner.initialThs();

			for (int k = 0; k < nIter; k++) {
				inner.iterDumb(k);
			}

			double th0, th1, chord;
			for (int k = i; k + 1 < j; k++) {
				this->pt(k, start).rth = inner.ths[k - i];
				this->pt(k + 1, start).lth = inner.ths[k + 1 - i];
				// Record curvatures (for blending, not all will be used)
				inner.getThs(k - i, &th0, &th1, &chord);
				double aks[2];
				this->curve->computeCurvature(th0, th1, aks);
				this->pt(k, start).rAk = aks[0];
				this->pt(k + 1, start).lAk = aks[1];
			}

			i = j - 1;
		}
	}
}

double Spline::chordLen(int i)
{
	Vec2 ptI = this->pt(i, 0).pt;
	Vec2 ptI1 = this->pt(i + 1, 0).pt;
	return hypot(ptI1.x - ptI.x, ptI1.y - ptI.y);
}

// Determine whether a control point requires curvature blending, and if so,
// the blended curvature. To be invoked after solving.
void Spline::computeCurvatureBlending()
{
	for (ControlPoint &pt : this->ctrlPts) {
		pt.hasKBlend = false;
		//pt.kBlend = null;
	}

	int length = this->ctrlPts.size() - (this->isClosed ? 0 : 1);
	for (int i = 0; i < length; i++) {
		ControlPoint &pt = this->pt(i, 0);
		if (pt.ty == 's' && pt.hasLth) {
			//double thresh = M_PI / 2 - 1e-6;
			//if (Math.abs(pt.rAk) > thresh || Math.abs(pt.lAk) > thresh) {
			//	// Don't blend reversals. We might reconsider this, but punt for now.
			//	continue;
			//}
			if (MathSign(pt.rAk) != MathSign(pt.lAk)) {
				pt.kBlend = 0;
				pt.hasKBlend = true;
			} else {
				double rK = myTan(pt.rAk) / this->chordLen(i - 1);
				double lK = myTan(pt.lAk) / this->chordLen(i);
				pt.kBlend = 2 / (1 / rK + 1 / lK);
				pt.hasKBlend = true;
			}
		}
	}
}

void Spline::renderWithFunctions(
							std::function<void(double x,double y)> moveto,
							std::function<void(double x,double y)> lineto,
							std::function<void(double x1,double y1,double x2,double y2,double x3,double y3)> curveto,
							std::function<void()> closepath
							)
{
	if (this->ctrlPts.size() == 0) {
		return;
	}

	// **** something wrong with end point smooth on all smooth closed path. when all auto smooth, end point is corner.
	// if any are manually tangent, then corner behaves right.
	// Manual smoothing of the end point works.

	ControlPoint &pt0 = this->ctrlPts[0];
	moveto(pt0.pt.x, pt0.pt.y);
	int length = this->ctrlPts.size() - (this->isClosed ? 0 : 1);

	for (int i = 0; i < length; i++) {
		//path->mark(i);
		ControlPoint &ptI  = this->pt(i, 0);
		ControlPoint &ptI1 = this->pt(i + 1, 0);
		double dx = ptI1.pt.x - ptI.pt.x;
		double dy = ptI1.pt.y - ptI.pt.y;
		double chth  = atan2(dy, dx);
		double chord = hypot(dy, dx);
		double th0 = mod2pi(ptI.rth - chth);
		double th1 = mod2pi(chth - ptI1.lth);

		// Apply curvature blending
		//let k0 = ptI.kBlend !== null ? ptI.kBlend * chord : null;
		//let k1 = ptI1.kBlend !== null ? ptI1.kBlend * chord : null;
		//--
		double k0 = 0;
		bool hasK0 = false;
		if (ptI.hasKBlend) {
			k0 = ptI.kBlend * chord;
			hasK0 = true;
		}
		double k1 = 0;
		bool hasK1 = false;
		if (ptI1.hasKBlend) {
			k1 = ptI1.kBlend * chord;
			hasK1 = true;
		}

		std::vector<Vec2> render = this->curve->render4(th0, th1, hasK0, k0, hasK1, k1);
		std::vector<double> c;
		for (uint j = 0; j < render.size(); j++) {
			Vec2 &pt = render[j];
			c.push_back(ptI.pt.x + dx * pt.x - dy * pt.y);
			c.push_back(ptI.pt.y + dy * pt.x + dx * pt.y);
		}
		c.push_back(ptI1.pt.x);
		c.push_back(ptI1.pt.y);
		for (uint j = 0; j < c.size(); j += 6) {
			curveto(c[j], c[j + 1], c[j + 2], c[j + 3], c[j + 4], c[j + 5]);
		}
	}
	if (this->isClosed) {
		closepath();
	}
}

std::shared_ptr<BezPath> Spline::render()
{
	std::shared_ptr<BezPath> path = std::make_shared<BezPath>();

	renderWithFunctions(
			[&](double x,double y) { path->moveto(x,y); },
			[&](double x,double y) { path->lineto(x,y); },
			[&](double x1,double y1, double x2,double y2, double x3,double y3) { path->curveto(x1,y1, x2,y2, x3,y3); },
			[&]() { path->closepath(); }
			);

	return path;
}

std::string Spline::renderSvg()
{
	std::shared_ptr<BezPath> path = this->render();
	
	return path->renderSvg();
}


void Spline::Clear()
{
	ctrlPts.clear();
}


//------------------------------ ControlPoint ------------------------------------

/*! \class ControlPoint
 */

//ControlPoint::ControlPoint(Vec2 pt)
//{
//	pt = pt;
//	ty = 0;
//	hasLth = false;
//	lth = 0;
//	hasRth = false;
//	rth = 0;
//}

/// ControlPoint is a lot like `Knot` but has no UI, is used for spline solving.
ControlPoint::ControlPoint(Vec2 pt, double ty, bool hasLth, double lth, bool hasRth, double rth)
{
	this->pt = pt;
	this->ty = ty;
	this->hasLth = hasLth;
	this->lth = lth;
	this->hasRth = hasRth;
	this->rth = rth;
}

} //namespace NewSpiro


