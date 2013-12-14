


namespace Laxkit {




class VectorPart
{
  public:
	VectorPart();
	virtual ~VectorPart();
};

class VectorPartPath : public VectorPart
class VectorPartFill : public VectorPart
class VectorPartStroke : public VectorPart
class VectorPartLineStyle : public VectorPart
class VectorPartFillStyle : public VectorPart
class VectorPartGradientFill : public VectorPart
class VectorPartGradientStrokeFill : public VectorPart


class VectorImage : public LaxImage
{
  public:
	PtrStack<VectorPart> parts;
};






} // namespace Laxkit

