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


#pragma once


#include <string>
#include <vector>
#include <memory>
#include <functional>


namespace NewSpiro {


//------------------------------ HitTestResult ------------------------------------

class HitTestResult
{
  public:
	double x,y;
	double bestDist = 1e12;
	int bestMark = -1;

	HitTestResult();
	HitTestResult(double x, double y);
	void accumulate(double dist, /*pt,*/ int mark);
	void accumLine(double x0, double y0, double x1, double y1, int mark);
	void accumCurve(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int mark);
};

//------------------------------ BezPath ------------------------------------

class BezPath
{
  public:
	class BezCommand
	{
	  public:
		char command;
		std::vector<double> val;
		double operator[](int i) { return val[i]; }

		BezCommand(const char cmd, double v1, double v2, double v3, double v4, double v5, double v6);
		BezCommand(const char cmd, double v1, double v2);
		BezCommand(const char cmd, double v1);
		BezCommand(const char cmd);
		~BezCommand() {}
	};

	std::vector<BezCommand> commands;

	BezPath() {}
	~BezPath() {}
	void moveto(double x, double y);
	void lineto(double x, double y);
	void curveto(double x1, double y1, double x2, double y2, double x3, double y3);
	void closepath();
	void mark(int i);
	std::string renderSvg();
	void hitTest(double x, double y, HitTestResult *result);
};


//------------------------------ Vec2 ------------------------------------

class Vec2
{
  public:
	double x;
	double y;
	
	Vec2() { x = y = 0; }

	Vec2(double x, double y)
	{
		this->x = x;
		this->y = y;
	}

	double norm();
	double dot(const Vec2 &other);
	double cross(const Vec2 &other);
};


//------------------------------ CubicBez ------------------------------------

class CubicBez
{
  public:
	/// Array of coordinate values [x0, y0, x1, y1, x2, y2, x3, y3].
	double c[8];

	CubicBez();
	CubicBez(double *coords);
	void weightsum(double c0, double c1, double c2, double c3, Vec2 *vec2_ret);
	void eval(double t, Vec2 *vec2_ret);
	void deriv(double t, Vec2 *vec2_ret);
	void deriv2(double t, Vec2 *vec2_ret);
	double curvature(double t);
	double atanCurvature(double t);
	void leftHalf(CubicBez *ret);
	void rightHalf(CubicBez *ret);
};



//------------------------------ CurvatureDerivs ------------------------------------

struct CurvatureDerivs
{
	double dak0dth0;
	double dak1dth0;
	double dak0dth1;
	double dak1dth1;
};


//------------------------------ TwoParamCurve ------------------------------------

class TwoParamCurve
{
  public:
	/// Compute curvature.
	///
	/// Result is an object with ak0 and ak1 (arctan of curvature at endpoints).
	/// Quadrant is significant - a value outside -pi/2 to pi/2 means a reversal
	/// of direction.
	///
	/// curvature_ret is a double[2]
	virtual void computeCurvature(double th0, double th1, double *curvature_ret) = 0;


	/// Render the curve, providing an array of _interior_ cubic bezier
	/// control points only. Return value is an array of 3n-1 Vec2's.
	virtual std::vector<Vec2> render(double th0, double th1) = 0;
	virtual std::vector<Vec2> render4(double th0, double th1, bool hasK0, double k0, bool hasK1, double k1) = 0;

	/// Get endpoint condition.
	///
	/// Return tangent at endpoint given next-to-endpoint tangent.
	virtual double endpointTangent(double th) = 0;


	/// Compute curvature derivatives.
	///
	/// Result is an object with dak0dth0 and friends.
	/// Default implementation is approximate through central differencing, but
	/// curves can override.
	void computeCurvatureDerivs(double th0, double th1, CurvatureDerivs *derivs_ret);
};


//------------------------------ MyCurve ------------------------------------

class MyCurve : public TwoParamCurve
{
  public:
	virtual std::vector<Vec2> render(double th0, double th1);
	virtual void computeCurvature(double th0, double th1, double *curvature_ret);
	virtual double endpointTangent(double th);

	void convCubic(const std::vector<Vec2> &pts, CubicBez &bez);
	std::vector<Vec2> render4Quintic(double th0, double th1, bool hasK0, double k0, bool hasK1, double k1);
	std::vector<Vec2> render4Cubic(double th0, double th1, bool hasK0, double k0, bool hasK1, double k1);
	std::vector<Vec2> render4(double th0, double th1, bool hasK0, double k0, bool hasK1, double k1);
};


//------------------------------ ControlPoint ------------------------------------

class ControlPoint
{
  public:
	Vec2 pt;
	char ty;
	double lth;
	double rth;
	bool hasLth;
	bool hasRth;

	bool hasKBlend;
	double kBlend;

	double rAk;
	double lAk;

	//ControlPoint(Vec2 pt);
	ControlPoint(Vec2 pt, double ty, bool hasLth, double lth, bool hasRth, double rth);
};


//------------------------------ TwoParamSpline ------------------------------------

class TwoParamSpline
{
  public:
	std::shared_ptr<TwoParamCurve> curve;
	std::vector<Vec2> ctrlPts;
	std::vector<double> ths;

	bool hasStartTh;
	bool hasEndTh;
	double startTh;
	double endTh;

	TwoParamSpline(std::shared_ptr<TwoParamCurve> curve, std::vector<Vec2> &ctrlPts);

	void initialThs();
	void getThs(int i, double *th0_ret, double *th1_ret, double *chord_ret);
	double computeErr(double th0,double th1,double chord, double *ak0, double th0_1, double th1_1, double chord_1, double *ak1);
	double iterDumb(int iter);
	//iterate();
	std::string renderSvg();
};

//------------------------------ Spline ------------------------------------

class Spline
{
  public:
	std::vector<ControlPoint> ctrlPts;
	std::shared_ptr<TwoParamCurve> curve;

	int StartIndex();
	double chordLen(int i);

	bool isClosed;

	Spline();
	Spline(std::vector<ControlPoint> &ctrlPts, bool isClosed);

	void solve();
	void computeCurvatureBlending();
	std::shared_ptr<BezPath> render(); //convert to bez path
	void renderWithFunctions(
							std::function<void(double x,double y)> moveto,
							std::function<void(double x,double y)> lineto,
							std::function<void(double x1,double y1,double x2,double y2,double x3,double y3)> curveto,
							std::function<void()> closepath
							);
	std::string renderSvg(); //convert to svg d string
	ControlPoint &pt(int i, int start);

	void AddControlPoint(double x, double y, char ty, bool hasLth, double lth, bool hasRth, double rth)
	{
		ctrlPts.push_back(ControlPoint(Vec2(x,y), ty, hasLth, lth, hasRth, rth));
	}

	void Clear();
};



} //namespace NewSpiro


