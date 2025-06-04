#pragma once
#include<glm/glm.hpp>
#include"QuadEdge.h"
#include "EntityPool.h"

namespace WT {
	class DelaunayTriangle;
	using DelaunayTrianglePtr=pool_ptr<DelaunayTriangle>;
	class DelaunayTriangle
	{
	private:
		QuadEdgePtr anchor;
		DelaunayTrianglePtr nextFace;
		DelaunayTrianglePtr selfPtr;
	public:
		DelaunayTriangle();

		void init(DelaunayTrianglePtr selfPtr, QuadEdgePtr eadge);
		DelaunayTrianglePtr linkTo(DelaunayTrianglePtr toTri) {
			nextFace = toTri;
			return selfPtr;
		}
		DelaunayTrianglePtr getLink() {
			return nextFace;
		}
		QuadEdgePtr getAnchor() { return anchor; }
		void dontAnchor(QuadEdgePtr edge);
		void reshape(QuadEdgePtr edge);

		glm::dvec3 point1()const { return anchor->Org(); }




	private:

	};
};