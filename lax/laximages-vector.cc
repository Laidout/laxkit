


namespace Laxkit {


/*! A vector path object much more streamlined for fast render than the
 * very complicated PathsData. These are meant to encapsulate basic svg path,
 * possibly to be used in glyphs of opentype svg fonts, for instance.
 *
 * When stroke or fill not given, assume use same from
 * previous part. When given, assume they will be used until another part
 * supplies others.
 */
class VectorPart
{
  public:
	enum VectorParts {
		None,
		SolidFill,
		GradientFill,
		MeshFill,
		MAX
	};

	int numpoints;
	flatpoint *points; //note: might contain multiple paths
	VectorStroke *stroke;
	VectorFill *fill;

	VectorPart();
	~VectorPart();
};

VectorPart::VectorPart()
{
	numpoints = 0;
	points = nullptr;
	stroke = nullptr;
	fill = nullptr;
}

VectorPart::~VectorPart()
{
	delete[] points;
	if (stroke) stroke->dec_count();
	if (fill)   fill  ->dec_count();
}


class VectorFill : public VectorPart
{
  public:
	int type;
	VectorFill(int type) { this->type = type; }
	virtual ~VectorFill() {}
};

class VectorPartStroke : public VectorPart
{
	int type;
	VectorStroke(int type) { this->type = type; }
	virtual ~VectorStroke() {}
};

class VectorPartStroke : public VectorStroke
{
  public:
	double width;
	int capstyle;
	int joinstyle;
	double miterlimit;
	int numdashes;
	double *dashes;
	double dash_offset;

	VectorPartLineStyle() { color=NULL; }
	virtual ~VectorPartLineStyle() { if (color) color->dec_count(); }
};

class VectorSolid : public VectorFill
{
  public:
	Color *color;
	VectorPartFillStyle() { color=NULL; }
	virtual ~VectorPartFillStyle() { if (color) color->dec_count(); }
};

class VectorGradient : public VectorFill
{
  public:
	GradientStrip *strip;
	VectorPartGradientFill() { strip=NULL; }
	virtual ~VectorPartGradientFill() { if (strip) strip->dec_count(); }
};

class VectorMeshFill : public VectorFill
{
  public:
	GradientMesh *mesh;
	VectorMeshFill() { mesh = nullptr; }
	virtual ~VectorMeshFill() { if (mesh) mesh->dec_count(); }
};

class VectorImageFill : public VectorFill
{
  public:
	LaxImage *img;
	VectorImageFill() { img = nullptr; }
	virtual ~VectorImageFill() { if (img) img->dec_count(); }
};


//------------------------------ VectorImage ----------------------------

/*! Special image form built from vector paths, with optional fill and stroke.
 */
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

