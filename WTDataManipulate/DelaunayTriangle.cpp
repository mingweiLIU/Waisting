#include "DelaunayTriangle.h"
namespace WT {

	void DelaunayTriangle::init(DelaunayTrianglePtr selfPtr, QuadEdgePtr edge)
	{
		this->selfPtr = selfPtr;
		reshape(edge);
	}

	void DelaunayTriangle::dontAnchor(QuadEdgePtr edge)
	{
		if (anchor==edge)
		{
			anchor = edge->Lnext();
		}
	}

	void DelaunayTriangle::reshape(QuadEdgePtr edge)
	{
		anchor = edge;
		edge->setLFace(selfPtr);
		edge->Lnext()->setLFace(selfPtr);
		edge->Lprev()->setLFace(selfPtr);
	}

};