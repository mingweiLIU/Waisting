#pragma once
#include <random>
#include <queue>
#include <vector>
#include <array>
#include<glm/glm.hpp>
#include"QuadEdge.h"
#include "TriangleUlit.h"
#include "EntityPool.h"
#include "IOAdapter.h"

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
		DelaunayTrianglePtr mFirstFace;
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
		virtual void scanTriagnle(DelaunayTrianglePtr t) {};

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

	struct Candidate
	{
		int x = 0, y = 0;
		double z = 0.0;
		double importance = -DBL_MAX;
		int token = 0;
		DelaunayTrianglePtr triangle;
		void consider(int sx, int sy, double sz, double currentImportance) noexcept {
			if (currentImportance>importance)
			{
				x = sx;
				y = sy;
				z = sz;
				importance = currentImportance;
			}
		}

		bool operator > (const Candidate& c) const noexcept { return importance > c.importance; }
		bool operator < (const Candidate& c) const noexcept { return importance < c.importance; }
	};

	class CandidateList {
	private:
		std::priority_queue<Candidate> mCandidate;
	public:
		void push_back(const Candidate& candidate) { mCandidate.push(candidate); }
		size_t size()const noexcept { return mCandidate.size(); }
		bool empty()const noexcept { return mCandidate.empty(); }

		Candidate getGreatest() {
			if (mCandidate.empty()) return Candidate();

			Candidate topCandidate = mCandidate.top();
			mCandidate.pop();
			return topCandidate;
		}
	};

	//深化Mesh 采用Terra算法
	class TerraMesh final :protected DelaunayMesh {
	private:
		IOFileInfo* mFileInfo;
		int mWidth, mHeigth;
		double mMaxError;
		int mCounter;
		std::vector<int> mToken;
		std::vector<char> mUsed;
		CandidateList mCandidates;

		void scanTriangleLine(const Plane& plane, int y, double x1, double x2, Candidate& candidate);

		void exportToObj(const std::vector<glm::dvec3>& positions,
			const std::vector<int>& indices,
			const std::string& filename);

		int getPosOfRaster(int height, int width) {
			return height * mWidth + width;
		}

		//计算平面
		inline void computePlane(Plane& plane, DelaunayTrianglePtr tri, IOFileInfo* file) {
			const glm::dvec2& p1 = tri->point1();
			const glm::dvec2& p2 = tri->point2();
			const glm::dvec2& p3 = tri->point3();

			//TODO  这里的FileInfo是8位 要扩展一个16位的
			const glm::dvec3 v1(p1,file->data[getPosOfRaster(p1.y, p1.x)]);
			const glm::dvec3 v2(p2,  file->data[getPosOfRaster(p2.y, p2.x)]);
			const glm::dvec3 v3(p3,  file->data[getPosOfRaster(p3.y, p3.x)]);
			plane.init(v1, v2, v3);
		}

		//三个点排序 按照y从低到高
		inline void orderTrianglePoints(std::array<glm::dvec2, 3>& p)noexcept {
			if (p[0].y > p[1].y) std::swap(p[0], p[1]);
			if (p[1].y > p[2].y) std::swap(p[1], p[2]);
			if (p[0].y > p[1].y) std::swap(p[0], p[1]);
		}

	public:
		TerraMesh(int width,int height,IOFileInfo* fileInfo);
		void greedyInsert(double maxError);
		void scanTriagnle(DelaunayTrianglePtr t) override;
		void convertToOBJ(int N=-999);
	};
};