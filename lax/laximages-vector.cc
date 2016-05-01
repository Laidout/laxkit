


namespace Laxkit {




class VectorPart
{
  public:
	VectorPart();
	virtual ~VectorPart();
};

class VectorPartPath : public VectorPart
{
  public:
	int numpoints;
	flatpoint *points;
};

class VectorPartFill : public VectorPart
{
  public:
};

class VectorPartStroke : public VectorPart
{
  public:
};

class VectorPartLineStyle : public VectorPart
{
  public:
	Color *color;
	double width;
	VectorPartLineStyle() { color=NULL; }
	~VectorPartLineStyle() { if (color) color->dec_count(); }
};

class VectorPartFillStyle : public VectorPart
{
  public:
	Color *color;
	VectorPartFillStyle() { color=NULL; }
	~VectorPartFillStyle() { if (color) color->dec_count(); }
};

class VectorPartGradientFill : public VectorPart
{
  public:
	GradientStrip *strip;
	VectorPartGradientFill() { strip=NULL; }
	~VectorPartGradientFill() { if (strip) strip->dec_count(); }
};

class VectorPartMeshFill : public VectorPart
{
  public:
	***
};


class VectorImage : public LaxImage
{
  public:
	PtrStack<VectorPart> parts;
	void Draw(Displayer *dp);
};

void VectorImage::Draw(Displayer *dp)
{
	dp->Push();

	for (int c=0; c<parts.n; c++) {
		if (dynamic_cast<VectorPartPath*>(parts.e[c])) {
			VectorPartPath *path=dynamic_cast<VectorPartPath*>(parts.e[c]);
			dp->drawFormattedPoints(path->points, path->numpoints, -1);

		} else if (dynamic_cast<VectorPartLineStyle*>(parts.e[c])) {
			VectorPartLineStyle *s=dynamic_cast<VectorPartLineStyle*>(parts.e[c]);
			if (s->width>=0) dp->LineWidth(s->width);
			if (s->color) dp->NewFG(s->Color);
		}
	}

	dp->Pop();
}






} // namespace Laxkit

