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
//    Copyright (C) 2008, Tom Lechner
//


#include <lax/interfaces/svgcoord.h>
#include <lax/attributes.h>
#include <lax/strmanip.h>
#include <lax/bezutils.h>

#include <cstdlib>


#define DBG
#include <iostream>


using namespace Laxkit;


namespace LaxInterfaces {

//! Convert an SVG path data snippet to a Coordinate list.
/*!
 * The d attribute of Svg paths allows definition of lines composed of a mixture of
 * straight lines, cubic bezier lines, quartic bezier lines, circle arcs, and ellipses.
 *
 * Unless how&4==4, if d contains multiple paths, the paths are all parsed into a single coordinate list,
 * delimited by a Coordinate where Coordinate::flags contains either 
 * POINT_TERMINATOR, or POINT_LOOP_TERMINATOR. and
 * should not be considered an actual point. POINT_LOOP_TERMINATOR means that 
 * the preceding path is closed. POINT_TERMINATOR means an open path. 
 * This terminator stuff is necessary because of how the svg spec defines path
 * continuation after a 'z'/'Z' command, which is that sometimes it requires knowledge of
 * the previous subpath.
 * 
 * If everything in d is parsed, then endptr will be pointing to the terminating null
 * character of the string.
 *
 * Any path in d that corresponds to path with a single coordinate will be ignored.
 *
 * If how&3==0, then allow mixed poly lines and bezier segments.
 * If how&3==1, then the returned path must be only a poly line.
 * If how&3==2, then the returned path must be all bezier segments.
 * if how&4==4 then return a normal closed Coordinate upon parsing a 'z'. Otherwise use the clunky
 * LOOP_TERMINATOR points.
 *
 * Ellipsoidal arcs are converted into cubic bezier segments.
 *
 * Svg path data follows the following format. The letter is a command. If more than the
 * given numbers are present, then assume they are parameters for another of the same command,
 * except for moveto, which switches to lineto for subsequent x,y.
 * <pre>
 *   M x y    move to absolute coordinate (x,y)
 *   m x y    move to coordinate (x,y) relative to previous coordinate
 *       Any further x y are implied lineto commands
 *
 *   L x y    draw a line to absolute coordinate (x,y)
 *   l x y    draw a line to coordinate (x,y) relative to previous coordinate
 *
 *   H x      draw a horizontal line to absolute coordinate (x,current y)
 *   h x      draw a horizontal line to coordinate ((current x)+x,(current y))
 *
 *   V x      draw a vertical line to absolute coordinate ((current x), y)
 *   v x      draw a vertical line to coordinate ((current x),(current y)+y)
 *
 *   C x1 y1 x2 y2 x y   draw a cubic bezier segment. 1 and 2 are controls, (x,y) is vertex
 *   c x1 y1 x2 y2 x y   like C, but using relative coordinates
 *
 *   S x2 y2 x y   draw a cubic bezier segment.  2 is the second controls, (x,y) is vertex,
 *   s x2 y2 x y     the first control is reflection of control from previous bezier segment,
 *                   or the current point, if there was no previous segment. 's' is for
 *                   relative coordinates
 *
 *   Q x1 y1 x y   draw a quartic bezier segment. 1 is control point, (x,y) is vertex
 *   q x1 y1 x y   like q, but using relative coordinates
 *
 *   T x y         draw a quartic bezier segment. (x,y) is vertex,
 *   t x y           the control point is reflection of control from previous bezier segment,
 *                   or the current point, if there was no previous segment. 't' is for
 *                   relative coordinates
 *
 *   A/a rx ry x-axis-rotation large-arc-flag sweep-flag x y
 *               draw an arc from the current point to the point (x,y)
 *               rx and ry are the x and y radii.
 *               the ellipse is rotated by x-axis-rotation (in degrees).
 *               if large-arc-flag==1, then the swept out arc is large. 0 means small.
 *               if sweep-flag==1, then the arc is traced in a positive angle direction,
 *               otherwise 0 means in a negative direction (hard to describe without pictures).
 *   
 *
 *   Z or z   close the current path by with a straight line. If a move does not follow it,
 *            then use the initial point of the path just closed as the starting point for
 *            the new path
 * </pre>
 *
 * \todo *** right now how&3 bits are ignored..
 * \todo *** if d doesn't start with a move, what is the spec'd default starting point??
 *    right now, failure results if not starting with a move
 */
Coordinate *SvgToCoordinate(const char *d, int how, char **endptr, int *totalpoints)
{
	if (!d) return NULL;

	const char *p=d;
	char *ee;
	char command=0;
	char hascurpoint=0;
	char lastwasmove=0;//when previous point resulted from a move
	flatpoint curpoint,
			  lastcontrol, //Vector with length and direction of last control 
			  			   // handle pointing at the associated vertex
						   // ***TODO this needs to be implemented fully!!
			  fp; //temp vector
	Coordinate *points=NULL,*ptr=NULL;
	int numpoints=0; //number of points in current contiguous path
	//int error=0;

	try {
	  while (*p) {
		while (isspace (*p)) p++;
		if (!*p) break;

		if (isdigit(*p) || *p=='.' || *p=='-') {
			//is a number, so assume we are continuing last command
		} else {
			command=*p;
			p++;
		}

		if (command=='M' || command=='m') { 
			//if (command=='m' && !hascurpoint) command='M'; //initial 'm' is treated as 'M'

			if (numpoints>0 && !(ptr->flags&(POINT_TERMINATOR|POINT_LOOP_TERMINATOR))) {
				 //move, if not first command is supposed to start a new subpath
				 //so terminate current path, by adding a point with a terminator flag
				ptr->next=new Coordinate(ptr->fp,POINT_TERMINATOR,0);
				ptr->next->prev=ptr;
				ptr=ptr->next;
			}
			 // move curpoint
			fp.x=strtod(p,&ee);
			if (ee==p) throw (2);

			p=ee;
			while (isspace (*p) || *p==',') p++;
			fp.y=strtod(p,&ee);
			if (ee==p) throw (3);

			if (command=='m') {
				if (hascurpoint) fp=fp+curpoint; //lowercase command is relative coords
				command='l'; //subsequent numbers are implied lineto commands
			} else command='L';

			curpoint=fp;
			hascurpoint=1;
			numpoints=0;
			lastwasmove=1;
			lastcontrol.set(0,0);
			
			p=ee;
			continue;

		} else if (command=='L' || command=='l') {
			 // line to: l (x y)+
			 // can have one or more x,y pairs
			if (!hascurpoint) throw (4);
			fp.x=strtod(p,&ee);
			if (ee==p) throw (5);

			p=ee;
			while (isspace (*p) || *p==',') p++;
			fp.y=strtod(p,&ee);
			if (ee==p) throw (6);

			p=ee;
			while (isspace (*p) || *p==',') p++;

			if (lastwasmove) {
				 //needed to add initial point of line
				if (!points) ptr=points=new Coordinate(curpoint);
				else {
					ptr->next=new Coordinate(curpoint);
					ptr=ptr->next;
				}
				numpoints++;
			}

			if (command=='l') fp+=curpoint;

			ptr->next=new Coordinate(fp);
			ptr->next->prev=ptr;
			ptr=ptr->next;
			numpoints++;

			lastcontrol.set(0,0);
			lastwasmove=0;
			curpoint=fp;
			continue;

		} else if (command=='H' || command=='h') {
			 //one or more x values, constant y
			if (!hascurpoint) throw (4);
			fp.y=curpoint.y;
			fp.x=strtod(p,&ee);
			if (ee==p) throw (5);
			p=ee;
			if (command=='h') fp.x+=curpoint.x;
			if (lastwasmove) {
				 //needed to add initial point of line
				if (!points) ptr=points=new Coordinate(curpoint);
				else {
					ptr->next=new Coordinate(curpoint);
					ptr=ptr->next;
				}
				numpoints++;
			}
			ptr->next=new Coordinate(fp);
			ptr->next->prev=ptr;
			ptr=ptr->next;
			curpoint=fp;
			numpoints++;
			lastcontrol.set(0,0);
			lastwasmove=0;
			continue;

		} else if (command=='V' || command=='v') {
			 //one or more y values, constant x
			if (!hascurpoint) throw (4);
			fp.x=curpoint.x;
			fp.y=strtod(p,&ee);
			if (ee==p) throw (6);
			p=ee;
			if (command=='v') fp.y+=curpoint.y;
			if (lastwasmove) {
				 //needed to add initial point of line
				if (!points) ptr=points=new Coordinate(curpoint);
				else {
					ptr->next=new Coordinate(curpoint);
					ptr=ptr->next;
				}
				numpoints++;
			}
			ptr->next=new Coordinate(fp);
			ptr->next->prev=ptr;
			ptr=ptr->next;
			curpoint=fp;
			lastcontrol.set(0,0);
			lastwasmove=0;
			continue;

		} else if (command=='C' || command=='c') {
			 //C (x1 y1 x2 y2 x y)+
			 //x1,y1 == first control point on curpoint
			 //x2,y2 == control point for x,y
			 // x,y  == final point
			 // 'c' has each of the 3 points relative TO CURRENT POINT !?!!

			if (!hascurpoint) throw (7);
			double dd[6];
			int n=DoubleListAttribute(p,dd,6,&ee);
			if (ee==p || n!=6) throw (8);
			p=ee;
			if (lastwasmove) {
				 //needed to add initial point of line
				if (!points) ptr=points=new Coordinate(curpoint);
				else {
					ptr->next=new Coordinate(curpoint);
					ptr=ptr->next;
				}
				numpoints++;
			}
			ptr->next=new Coordinate(dd[0],dd[1],POINT_TOPREV,0);
			ptr->next->prev=ptr;
			ptr=ptr->next;
			if (command=='c') ptr->fp+=curpoint;

			ptr->next=new Coordinate(dd[2],dd[3],POINT_TONEXT,0);
			ptr->next->prev=ptr;
			ptr=ptr->next;
			if (command=='c') ptr->fp+=curpoint;

			ptr->next=new Coordinate(dd[4],dd[5]);
			ptr->next->prev=ptr;
			ptr=ptr->next;
			if (command=='c') ptr->fp+=curpoint;
			curpoint=ptr->fp;

			numpoints+=3;

			lastcontrol=ptr->p() - ptr->prev->p();
			lastwasmove=0;
			continue;

		} else if (command=='S' || command=='s') {
			 //smooth cubic curve:
			 //S (x2 y2 x y)+
			 //x2,y2 == control point for x,y
			 // x,y  == final point
			 //control point for curpoint is reflection about curpoint of the control
			 //point of previous segment. If the previous command was not s, S, c, or C,
			 //then assume the control point is the same as curpoint.

			if (!hascurpoint) throw (7);
			double dd[4];
			int n=DoubleListAttribute(p,dd,4,&ee);
			if (ee==p || n!=4) throw (8);
			p=ee;
			if (command=='s') {
				dd[0]+=curpoint.x;
				dd[1]+=curpoint.y;
				dd[2]+=curpoint.x;
				dd[3]+=curpoint.y;
			}

			if (lastwasmove) {
				 //needed to add initial point of line
				if (!points) ptr=points=new Coordinate(curpoint);
				else {
					ptr->next=new Coordinate(curpoint);
					ptr=ptr->next;
				}
				numpoints++;
			}
			ptr->next=new Coordinate(curpoint+lastcontrol, POINT_TOPREV,0);
			ptr->next->prev=ptr;
			ptr=ptr->next;

			ptr->next=new Coordinate(dd[2],dd[3],POINT_TONEXT,0);
			ptr->next->prev=ptr;
			ptr=ptr->next;

			ptr->next=new Coordinate(dd[4],dd[5]);
			ptr->next->prev=ptr;
			ptr=ptr->next;

			curpoint=ptr->fp; 
			numpoints+=3;

			lastcontrol=ptr->p() - ptr->prev->p();
			lastwasmove=0;
			continue;

		} else if (command=='Q' || command=='q') {
			 //quadratic bezier curve: x1 y1 x y
			 //convert to cubic, each control is exactly 2/3 distance to x1,y1

			if (!hascurpoint) throw (7);
			double dd[4];
			int n=DoubleListAttribute(p,dd,4,&ee);
			if (ee==p || n!=4) throw (8);
			p=ee;
			flatpoint cc=flatpoint(dd[0],dd[1]);
			flatpoint p2=flatpoint(dd[2],dd[3]);
			if (command=='q') { cc+=curpoint; p2+=curpoint; }

			if (lastwasmove) {
				 //needed to add initial point of line
				if (!points) ptr=points=new Coordinate(curpoint);
				else {
					ptr->next=new Coordinate(curpoint);
					ptr=ptr->next;
				}
				numpoints++;
			}
			ptr->next=new Coordinate(curpoint+2./3*(cc-curpoint), POINT_TOPREV,0);
			ptr->next->prev=ptr;
			ptr=ptr->next;

			ptr->next=new Coordinate(p2+2./3*(cc-p2), POINT_TONEXT,0);
			ptr->next->prev=ptr;
			ptr=ptr->next;

			ptr->next=new Coordinate(p2);
			ptr->next->prev=ptr;
			ptr=ptr->next;
			curpoint=ptr->fp;

			numpoints+=3;

			lastcontrol=ptr->p() - ptr->prev->p();
			lastwasmove=0;
			continue;

		} else if (command=='T' || command=='t') {
			 //smooth quadratic bezier curve continuation: (x y)+

			if (!hascurpoint) throw (7);
			double dd[2];
			int n=DoubleListAttribute(p,dd,2,&ee);
			if (ee==p || n!=2) throw (8);
			p=ee;

			flatpoint cc=curpoint+lastcontrol*3/2;
			flatpoint p2=flatpoint(dd[0],dd[1]);
			if (command=='t') { p2+=curpoint; }

			if (lastwasmove) {
				 //needed to add initial point of line
				if (!points) ptr=points=new Coordinate(curpoint);
				else {
					ptr->next=new Coordinate(curpoint);
					ptr=ptr->next;
				}
				numpoints++;
			}
			ptr->next=new Coordinate(curpoint+2./3*(cc-curpoint), POINT_TOPREV,0);
			ptr->next->prev=ptr;
			ptr=ptr->next;

			ptr->next=new Coordinate(p2+2./3*(cc-p2), POINT_TONEXT,0);
			ptr->next->prev=ptr;
			ptr=ptr->next;

			ptr->next=new Coordinate(p2);
			ptr->next->prev=ptr;
			ptr=ptr->next;
			curpoint=ptr->fp;

			numpoints+=3;

			lastcontrol=ptr->p() - ptr->prev->p();
			lastwasmove=0;
			continue;


		} else if (command=='A' || command=='a') {
			 //create an arc
			//   A/a rx ry            #x and y radii
			//       x-axis-rotation  #tilt in degrees
			//       large-arc-flag
			//       sweep-flag
			//       x y              #ending point
			//
			//	    An ellipse can touch both points in 2 ways (or none), and traced from p1 to p2 in two ways.
			//      Which of the 4 possible segments used in determined by various combinations of the flags.
			//      if large-arc-flag==1, then the swept out arc is large. 0 means small.
			//      if sweep-flag==1, then the arc is traced in a positive angle direction,
			//      otherwise 0 means in a negative direction (hard to describe without pictures).
			//			
			//		if either rx or ry is zero, then insert a straight line segment.
			//		if endpoint is same as current point, then skip.
			//		ignore negatives on rx or ry
			//		if there is no solution, scale up until there is
			//		for the flags, any nonzero value is taken to be 1
			//
			//		Implementation below is guided by svg 1.1 spec implementation notes F.6

			if (!hascurpoint) throw (9);

			double dd[7];
			int n=DoubleListAttribute(p,dd,7,&ee);
			if (ee==p || n!=7) throw (10);
			p=ee;

			double x1  = curpoint.x,
				   y1  = curpoint.y,
				   rx  = fabs(dd[0]),
				   ry  = fabs(dd[1]),
				   xrot= dd[2]*M_PI/180,
				   la  = (dd[3]==0 ? 0 : 1),
				   sw  = (dd[4]==0 ? 0 : 1),
				   x2  = dd[5] + (command=='a' ? curpoint.x : 0),
				   y2  = dd[6] + (command=='a' ? curpoint.y : 0);

			if (curpoint.x==x2 && curpoint.y==y2) continue; //skip when same as curpoint

			if (rx==0 || ry==0) {
				 //a radius had a bad value, treat whole thing as a line segment
				if (lastwasmove) {
					 //needed to add initial point of line
					if (!points) ptr=points=new Coordinate(curpoint);
					else {
						ptr->next=new Coordinate(curpoint);
						ptr=ptr->next;
					}
					numpoints++;
				}

				ptr->next=new Coordinate(x2,y2);
				ptr->next->prev=ptr;
				ptr=ptr->next;
				numpoints++;

				lastcontrol.set(0,0);
				lastwasmove=0;
				curpoint=fp;
				continue;
			}

			flatpoint mid=(curpoint-flatpoint(x2,y2))/2;
			double x1p= cos(xrot)*mid.x + sin(xrot)*mid.y;
			double y1p=-sin(xrot)*mid.x + cos(xrot)*mid.y;
			double lambda=x1p*x1p/(rx*rx) + y1p*y1p/(ry*ry);
			if (lambda>1) { //correction for scaling up when no solution otherwise
				rx*=sqrt(lambda);
				ry*=sqrt(lambda);
			}

			double sq=sqrt((rx*rx*ry*ry - rx*rx*y1p*y1p - ry*ry*x1p*x1p) / (rx*rx*y1p*y1p + ry*ry*x1p*x1p));
			if (la==sw) sq=-sq;

			double cxp=sq*rx*y1p/ry;
			double cyp=-sq*ry*x1p/rx;

			double cx=cos(xrot)*cxp-sin(xrot)*cyp + (x1+x2)/2; //F.6.5.3 in svg 1.1 spec
			double cy=sin(xrot)*cxp+cos(xrot)*cyp + (y1+y2)/2;

			flatvector u,v;
			u.x=1; u.y=0;
			v.x=(x1p-cxp)/rx;  v.y=(y1p-cyp)/ry;
			double uv = u.x*v.y-u.y*v.x;
			double theta1 = (uv<0 ? -1 : (uv>0 ? 1 : 0)) * acos(u*v/norm(u)/norm(v));

			u.x=( x1p-cxp)/rx;  u.y=( y1p-cyp)/ry;
			v.y=(-x1p-cxp)/rx;  v.y=(-y1p-cyp)/ry;
			uv = u.x*v.y-u.y*v.x;
			double dtheta = (uv<0 ? -1 : (uv>0 ? 1 : 0)) * acos(u*v/norm(u)/norm(v));

			if (sw==0 && dtheta>0) dtheta -= 2*M_PI;
			else if (sw==1 && dtheta<0) theta1 += 2*M_PI;

			dtheta = fmod(dtheta, 2*M_PI);
			theta1 = fmod(theta1, 2*M_PI);

			 //now ellipse is centered at cx,cy, radii rx,ry, rotated xrot, from theta1 to (theta1 + dtheta)
			 //first insert initial point if necessary
			if (lastwasmove) {
				 //needed to add initial point of line
				if (!points) ptr=points=new Coordinate(curpoint);
				else {
					ptr->next=new Coordinate(curpoint);
					ptr=ptr->next;
				}
				numpoints++;
			}

			 //now insert 4 bezier segments forming hopefully at most a full circle
			flatpoint xx(rx*cos(xrot),  rx*sin(xrot));
			flatpoint yy(-ry*sin(xrot), ry*cos(xrot));
			flatpoint pts[12];
			Laxkit::bez_ellipse(pts, 4, cx,cy, rx,ry, xx,yy, theta1,theta1+dtheta);

			int flag=0;
			for (int c=2; c<13; c++) {
				if (c%3==0) flag=POINT_TONEXT;
				else if (c%3==1) flag=POINT_VERTEX;
				else flag=POINT_TOPREV;

				ptr->next=new Coordinate(c<12 ? pts[c] : pts[0], flag, NULL);
				ptr->next->prev=ptr;
				ptr=ptr->next;
				numpoints++;
			}

			lastwasmove=0;
			continue;

		} else if (command=='Z' || command=='z') {
			 //close path. If command other than a moveto follows a z, then the next subpath
			 //begins at the same INITIAL point as the current subpath!!
			if (!hascurpoint || numpoints<=1) throw(10);
			
			if (ptr) {
				if (how&4) {
					 //makes a single path, by
				 	 //connecting with beginning of curve and returning
					ptr->next=points;
					points->prev=ptr;
					while (isspace(*p)) p++;
					if (endptr) *endptr=const_cast<char*>(p);
					break;
				}
				ptr->next=new Coordinate(ptr->fp,POINT_LOOP_TERMINATOR,0);
				ptr->next->prev=ptr;
				ptr=ptr->next;
				Coordinate *pp=ptr;
				 //scan for beginning of current subpath, make curpoint that
				while (pp->prev && !(pp->prev->flags&(POINT_LOOP_TERMINATOR|POINT_TERMINATOR)))
					pp=pp->prev;
				curpoint=pp->fp;
				numpoints=0;

				//while (isspace(*p)) p++;
				//if (!*p) {
					////we have reached the end of input... if there's only
					//one path, we should return a normal linked Coordinate, not
					//this terminator stuff??
					//
				//}
			}
			lastwasmove=0;
			continue;

		} else if (isalpha(*p)) {
			cerr << " *** must implement '"<<*p<<"' in SvgToCoordinate()!!"<<endl;
			if (endptr) *endptr=const_cast<char *>(p);
			break;

		} else {
			 //hit a non-letter with unclaimed command. this is error, and halts parsing
			if (endptr) *endptr=const_cast<char *>(p);
			break;
		}
	  } 
	} catch (int e) {
		 //was error
		DBG cerr <<"svgtocoord() had error in parsing: "<<e<<endl;
		if (points) { delete points; points=NULL; }
		if (endptr) *endptr=const_cast<char *>(p);
	}

	DBG char *str=CoordinateToSvg(points);
	DBG cerr <<"SvgToCoord \""<<d<<"\" --> \""<<(str?str:"???")<<"\""<<endl;
	DBG delete[] str;

	if (totalpoints) *totalpoints=numpoints;
	return points;
}

//Coordinate *SvgToFlatpoints(const char *d, int how, char **endptr,int *numpoints)
//{
//	Coordinate *coords=SvgToCoordinate(d,4,endptr,numpoints);
//	flatpoint *points=new flatpoint[numpoints];
//	for (int c=0; c<numpoints; c++) {
//		***
//	}
//}

//! Convert a coordinate list to an svg styled string.
/*! This only handles coordinates that are marked POINT_VERTEX,
 * POINT_TONEXT, and POINT_TOPREV. All others are assumed to be vertex points.
 *
 * \todo if the list is malformed, such as by having other than 2 control
 *    points between vertices, then this currently ignores extras...
 */
char *CoordinateToSvg(Coordinate *points)
{
	if (!points) return NULL;
	Coordinate *p=points;
	char *d=NULL;

	 //skip initial control points
	while (p->flags&POINT_TONEXT || p->flags&POINT_TOPREV) {
		p=p->next;
		if (p==points) return NULL;
	}

	 //add points
	flatpoint c1,c2;
	int lasttype=0;
	char str[200];
	do {
		if (p->flags&POINT_VERTEX) {
			if (!d) { 
				sprintf(str,"M %.15g %.15g ",p->fp.x,p->fp.y);
				d=newstr(str);
			} else {
				if (lasttype==0) {
					sprintf(str,"L %.15g %.15g ",p->fp.x,p->fp.y);
					appendstr(d,str);
				} else {
					if (lasttype==1) c2=c1;
					sprintf(str,"C %.15g %.15g %.15g %.15g %.15g %.15g ",
							c1.x,c1.y, c2.x,c2.y, p->fp.x,p->fp.y);

					lasttype=0;
				}
			}
		} else if (p->flags&POINT_TOPREV) {
			c1=p->fp;
			lasttype=1;
		} else if (p->flags&POINT_TONEXT) {
			if (lasttype==0) c1=c2=p->fp;
			else c2=p->fp;
			lasttype=2;
		} else {
			DBG cerr <<"*** unknown point flag in CoordinateToSvg(Coordinate*): "<<p->flags<<endl;
		}
		p=p->next;
	} while (p && p!=points);
	if (p==points) {
		 //correct for dangling controls
		if (lasttype!=0 || !(points->flags&POINT_VERTEX)) {
			flatpoint v=points->fp;
			if (lasttype==1) {
				if (points->flags&POINT_TONEXT) { 
					c2=points->fp;
					v=points->next->fp;
				} else c2=c1;
			} else if (lasttype==0) {
				 //flags must not have been vertex
				if (points->flags&POINT_TOPREV) { 
					c1=points->fp;
					c2=points->next->fp; //***this is an assumption!!
					v=points->next->next->fp;
				} else {
					c1=c2=points->fp;
					v=points->next->fp;
					sprintf(str,"C %.15g %.15g %.15g %.15g %.15g %.15g ",
							c1.x,c1.y, c2.x,c2.y, v.x,v.y);
					appendstr(d,str);
				}
				
			}
		}
		appendstr(d," z");
	}
	return d;
}

} //namespace LaxInterfaces

