#pragma once
#include <random>
#include<glm/glm.hpp>
#include"QuadEdge.h"
#include "EntityPool.h"

namespace WT {
	class DelaunayTriangle;
	using DelaunayTrianglePtr=pool_ptr<DelaunayTriangle>;

	class DelaunayMesh {
	private:
		std::shared_ptr<EntityPool<QuadEdge>> mEdges;
		std::shared_ptr<EntityPool<DelaunayTriangle>> mTriangles;
		std::mt19937 mRandomGen;

	protected:
		QuadEdgePtr mStartingEdge;
		QuadEdgePtr mFirstFace;
		DelaunayTrianglePtr makeFace(QuadEdgePtr edge);
		void deleteEdge(QuadEdgePtr edge);
		QuadEdgePtr connect(QuadEdgePtr a, QuadEdgePtr b);
		void swap(QuadEdgePtr edge);

		bool ccwBoundary(QuadEdgePtr e);
		bool onEdge(const glm::dvec2 p, QuadEdgePtr e);

		unsigned int nextRandomNumber() {
			return mRandomGen() % std::numeric_limits<unsigned int>::max();
		}

	public:
		DelaunayMesh();
		void initMesh(const glm::dvec2 a, const glm::dvec2 b, const glm::dvec2 c, const glm::dvec2 d);
		//void initMesh(const BBox2D& bb);
		
		virtual bool shouldSwap(const glm::dvec2 p, QuadEdgePtr e);
		virtual bool scanTriagnle(DelaunayTrianglePtr t);

		bool isInterior(QuadEdgePtr e);

		//添加一个点到mesh中 
		QuadEdgePtr spoke(const glm::dvec2 x, QuadEdgePtr e);
		void optimize(const glm::dvec2 x, QuadEdgePtr e);

		QuadEdgePtr locate(const glm::dvec2 x) { return locate(x, mStartingEdge); }
		QuadEdgePtr locate(const glm::dvec2 x, QuadEdgePtr hint);
		void insert(const glm::dvec2 x, DelaunayTrianglePtr tri);
	};




	class DelaunayTriangle
	{
	private:
		QuadEdgePtr anchor;
		DelaunayTrianglePtr nextFace;
		DelaunayTrianglePtr selfPtr;
	public:
		DelaunayTriangle() {};

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

		glm::dvec2 point1()const { return anchor->Org(); }
		glm::dvec2 point2()const { return anchor->Dest(); }
		glm::dvec2 point3()const { return anchor->Lprev()->Org(); }
	};
};