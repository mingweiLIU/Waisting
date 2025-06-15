#include "DelaunayTriangle.h"
#include <fstream>
#include "TriangleUlit.h"
namespace WT {
	DelaunayMesh::DelaunayMesh() :
		mEdges(EntityPool<QuadEdge>::create()),
		mTriangles(EntityPool<DelaunayTriangle>::create()),
		mRandomGen(42) //fixed seed for deterministics sequence of random numbers
	{
		mEdges->reserve(4096);
		mTriangles->reserve(1024);
	}

	void DelaunayMesh::initMesh(const glm::dvec2 a, const glm::dvec2 b, const glm::dvec2 c, const glm::dvec2 d)
	{
		QuadEdgePtr edgeAB = mEdges->spawn();
		edgeAB->init(edgeAB);
		edgeAB->setEndPoints(a, b);

		QuadEdgePtr edgeBC = mEdges->spawn();
		edgeBC->init(edgeBC);
		splice(edgeAB->Sym(), edgeBC);
		edgeBC->setEndPoints(b, c);

		QuadEdgePtr edgeCD = mEdges->spawn();
		edgeCD->init(edgeCD);
		splice(edgeBC->Sym(), edgeCD);
		edgeCD->setEndPoints(c, d);

		QuadEdgePtr edgeDA = mEdges->spawn();
		edgeDA->init(edgeDA);
		splice(edgeCD->Sym(), edgeDA);
		edgeDA->setEndPoints(d, a);
		splice(edgeDA->Sym(), edgeAB);

		QuadEdgePtr diag = mEdges->spawn();
		diag->init(diag);
		splice(edgeDA->Sym(), diag);
		splice(edgeBC->Sym(), diag->Sym());
		diag->setEndPoints(a, c);

		mStartingEdge = edgeAB;

		mFirstFace.clear();

		makeFace(edgeAB->Sym());
		makeFace(edgeCD->Sym());
	}

	bool DelaunayMesh::shouldSwap(const glm::dvec2 p, QuadEdgePtr e)
	{
		QuadEdgePtr t = e->Oprev();
		return TriangleUlit::inCircle(e->Org(), t->Dest(), e->Dest(), p);
	}


	bool DelaunayMesh::isInterior(QuadEdgePtr e)
	{
		return (e->Lnext()->Lnext()->Lnext() == e && e->Rnext()->Rnext()->Rnext() == e);
	}

	QuadEdgePtr DelaunayMesh::spoke(const glm::dvec2 x, QuadEdgePtr e)
	{
		DelaunayTrianglePtr new_faces[4];
		int facedex = 0;

		QuadEdgePtr boundary_edge;

		DelaunayTrianglePtr lface = e->Lface();
		lface->dontAnchor(e);
		new_faces[facedex++] = lface;

		if (onEdge(x, e))
		{
			if (ccwBoundary(e))
			{
				// e lies on the boundary
				// Defer deletion until after new edges are added.
				boundary_edge = e;
			}
			else
			{
				DelaunayTrianglePtr sym_lface = e->Sym()->Lface();
				new_faces[facedex++] = sym_lface;
				sym_lface->dontAnchor(e->Sym());

				e = e->Oprev();
				deleteEdge(e->Onext());
			}
		}
		else
		{
			// x lies within the Lface of e
		}

		QuadEdgePtr base = mEdges->spawn();
		base->init(base);

		base->setEndPoints(e->Org(), x);

		splice(base, e);

		mStartingEdge = base;
		do
		{
			base = connect(e, base->Sym());
			e = base->Oprev();
		} while (e->Lnext() != mStartingEdge);

		if (boundary_edge) deleteEdge(boundary_edge);

		// Update all the faces in our new spoked polygon.
		// If point x on perimeter, then don't add an exterior face.

		base = boundary_edge ? mStartingEdge->Rprev() : mStartingEdge->Sym();

		do
		{
			if (facedex)
			{
				new_faces[--facedex]->reshape(base);
			}
			else
			{
				makeFace(base);
			}

			base = base->Onext();
		} while (base != mStartingEdge->Sym());

		return mStartingEdge;
	}

	void DelaunayMesh::optimize(const glm::dvec2 x, QuadEdgePtr s)
	{
		QuadEdgePtr start_spoke = s;
		QuadEdgePtr spoke = s;

		do
		{
			QuadEdgePtr e = spoke->Lnext();
			if (isInterior(e) && shouldSwap(x, e))
			{
				swap(e);
			}
			else
			{
				spoke = spoke->Onext();
				if (spoke == start_spoke) break;
			}
		} while (true);

		// Now, update all the triangles
		spoke = start_spoke;

		do
		{
			QuadEdgePtr e = spoke->Lnext();
			DelaunayTrianglePtr t = e->Lface();

			if (t) this->scanTriagnle(t);

			spoke = spoke->Onext();
		} while (spoke != start_spoke);
	}

	QuadEdgePtr DelaunayMesh::locate(const glm::dvec2 x, QuadEdgePtr start)
	{
		QuadEdgePtr e = start;
		double t = TriangleUlit::triAreaDouble(x, e->Dest(), e->Org());

		if (t > 0)
		{ // x is to the right of edge e
			t = -t;
			e = e->Sym();
		}

		while (true)
		{
			QuadEdgePtr eo = e->Onext();
			QuadEdgePtr ed = e->Dprev();

			double to = TriangleUlit::triAreaDouble(x, eo->Dest(), eo->Org());
			double td = TriangleUlit::triAreaDouble(x, ed->Dest(), ed->Org());

			if (td > 0)
			{ // x is below ed
				if (to > 0 || (to == 0 && t == 0))
				{
					// x is interior, or origin endpoint
					mStartingEdge = e;
					return e;
				}
				else
				{
					// x is below ed, below eo
					t = to;
					e = eo;
				}
			}
			else
			{ // x is on or above ed
				if (to > 0)
				{
					// x is above eo
					if (td == 0 && t == 0)
					{
						// x is destination endpoint
						mStartingEdge = e;
						return e;
					}
					else
					{
						// x is on or above ed and above eo
						t = td;
						e = ed;
					}
				}
				else
				{
					// x is on or below eo
					if (t == 0 && !TriangleUlit::leftOf(eo->Dest(), e))
					{
						// x on e but DelaunayMesh is to right
						e = e->Sym();
					}
					else if ((nextRandomNumber() & 1) == 0)
					{
						// x is on or above ed and on or below eo; step randomly
						t = to;
						e = eo;
					}
					else
					{
						t = td;
						e = ed;
					}
				}
			}
		}
	}

	void DelaunayMesh::insert(const glm::dvec2 x, DelaunayTrianglePtr tri)
	{
		QuadEdgePtr e = tri ? locate(x, tri->getAnchor()) : locate(x);

		if ((x == e->Org()) || (x == e->Dest()))
		{
			// point is already in the mesh, so update the triangles x is in
			optimize(x, e);
		}
		else
		{
			QuadEdgePtr start_spoke = spoke(x, e);
			if (start_spoke)
			{
				optimize(x, start_spoke->Sym());
			}
		}
	}

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

	DelaunayTrianglePtr DelaunayMesh::makeFace(QuadEdgePtr edge)
	{
		DelaunayTrianglePtr t = mTriangles->spawn();
		t->init(t, edge);

		mFirstFace = t->linkTo(mFirstFace);
		return t;
	}

	void DelaunayMesh::deleteEdge(QuadEdgePtr edge)
	{
		splice(edge, edge->Oprev());
		splice(edge->Sym(), edge->Sym()->Oprev());
		edge->recycleNext();
		edge.recycle();
	}

	QuadEdgePtr DelaunayMesh::connect(QuadEdgePtr a, QuadEdgePtr b)
	{
		QuadEdgePtr e = mEdges->spawn();
		e->init(e);

		splice(e, a->Lnext());
		splice(e->Sym(), b);
		e->setEndPoints(a->Dest(), b->Org());
		return e;
	}

	void DelaunayMesh::swap(QuadEdgePtr e)
	{
		DelaunayTrianglePtr f1 = e->Lface();
		DelaunayTrianglePtr f2 = e->Sym()->Lface();

		QuadEdgePtr a = e->Oprev();
		QuadEdgePtr b = e->Sym()->Oprev();

		splice(e, a);
		splice(e->Sym(), b);
		splice(e, a->Lnext());
		splice(e->Sym(), b->Lnext());
		e->setEndPoints(a->Dest(), b->Dest());

		f1->reshape(e);
		f2->reshape(e->Sym());
	}

	bool DelaunayMesh::ccwBoundary(QuadEdgePtr e)
	{
		return !TriangleUlit::rightOf(e->Oprev()->Dest(), e);
	}

	bool DelaunayMesh::onEdge(const glm::dvec2 x, QuadEdgePtr e)
	{
		double EPS = 1e-7;
		double t1, t2, t3;

		t1 = (x - e->Org()).length();
		t2 = (x - e->Dest()).length();

		if (t1 < EPS || t2 < EPS) return true;

		t3 = (e->Org() - e->Dest()).length();

		if (t1 > t3 || t2 > t3) return false;

		Line line(e->Org(), e->Dest());
		return (fabs(line.eval(x)) < EPS);
	}

	TerraMesh::TerraMesh(int width, int height, IOFileInfo* fileInfo)
	{
		this->mWidth = width;
		this->mHeigth = height;
		this->mFileInfo = fileInfo;
	}

	void TerraMesh::greedyInsert(double maxError)
	{
		this->mMaxError = maxError;
		mCounter = 0;
		mToken.resize(mWidth*mHeigth);
		mUsed.resize(mWidth * mHeigth);

		this->initMesh(glm::dvec2(0, 0), glm::dvec2(0, mHeigth - 1), glm::dvec2(mWidth - 1, mHeigth - 1), glm::dvec2(mWidth - 1, 0));
		mUsed[getPosOfRaster(0, 0)] = 1;
		mUsed[getPosOfRaster(mHeigth - 1, 0)] = 1;
		mUsed[getPosOfRaster(mHeigth - 1, mWidth-1)] = 1;
		mUsed[getPosOfRaster(0, mWidth - 1)] = 1;

		int N = 1;
		convertToOBJ(N);

		DelaunayTrianglePtr t = mFirstFace;
		while (t)
		{
			scanTriagnle(t);
			t = t->getLink();
		}

		while (!mCandidates.empty())
		{
			Candidate oneCandidate = mCandidates.getGreatest();
			if (oneCandidate.importance<mMaxError) continue;

			if (mToken[getPosOfRaster(oneCandidate.y, oneCandidate.x)] != oneCandidate.token) continue;

			mUsed[getPosOfRaster(oneCandidate.y, oneCandidate.x)] = 1;
			this->insert(glm::dvec2(oneCandidate.x, oneCandidate.y), oneCandidate.triangle);

			convertToOBJ(++N);
		}
	}

	void TerraMesh::scanTriagnle(DelaunayTrianglePtr t)
	{
		Plane zPlane;
		computePlane(zPlane, t, mFileInfo);

		std::array<glm::dvec2, 3> byY = { {
				t->point1(),t->point2(),t->point3(),
		} };

		orderTrianglePoints(byY);
		const double v0_x = byY[0].x;
		const double v0_y = byY[0].y;
		const double v1_x = byY[1].x;
		const double v1_y = byY[1].y;
		const double v2_x = byY[2].x;
		const double v2_y = byY[2].y;

		Candidate oneCadidate = { 0,0,0.0,-DBL_MAX,mCounter++,t };

		const double dx2 = (v2_x - v0_x) / (v2_y - v0_y);
		if (v1_y!=v0_y)
		{
			const double dx1 = (v1_x - v0_x) / (v1_y - v0_y);
			double x1 = v0_x;
			double x2 = v0_x;
			const int startY = v0_y;
			const int endY = v1_y;

			for (int y=startY;y<endY;++y)
			{
				scanTriangleLine(zPlane, y, x1, x2, oneCadidate);
				x1 += dx1; x2 += dx2;
			}
		}

		if (v2_y!=v1_y)
		{
			const double dx1 = (v2_x - v1_x) / (v2_y - v1_y);
			double x1 = v1_x; double x2 = v0_x;
			const int startY = v1_y, endY = v2_y;
			for (size_t y = startY; y <= endY; y++)
			{
				scanTriangleLine(zPlane, y, x1, x2, oneCadidate);
				x1 += dx1; x2 += dx2;
			}
		}

		mToken[getPosOfRaster(oneCadidate.y, oneCadidate.x)] = oneCadidate.token;
		mCandidates.push_back(oneCadidate);
	}

	void TerraMesh::scanTriangleLine(const Plane& plane, int y, double x1, double x2, Candidate& candidate) {
		const int startX = static_cast<int>(ceil(fmin(x1, x2)));
		const int endX = static_cast<int>(floor(fmax(x1, x2)));

		if (startX > endX) return;

		double z0 = plane.eval(startX, y);
		double dz = plane.a;

		for (size_t x = startX; x <= endX; x++)
		{
			if (!mUsed[getPosOfRaster(y,x)])
			{
				const double z = mFileInfo->data[getPosOfRaster(y, x)];
				const double diff = fabs(z - z0);
				candidate.consider(x, y, z, diff);
			}
			z0 += dz;
		}
	}

	void TerraMesh::convertToOBJ(int N)
	{
		//这里将导出obj
		std::vector < glm::dvec3> pos;
		std::vector<int> indies;

		std::vector<int> vertexID;
		vertexID.resize(mWidth * mHeigth);
		
		int index = 0;
		for (int y=0;y<mHeigth;++y)
		{
			for (int x = 0; x < mWidth; x++)
			{
				if (mUsed[getPosOfRaster(y,x)] == 1)
				{
					const double z = mFileInfo->data[getPosOfRaster(y, x)];
					//TODO 这里是将影像位置空间化 为了检验代码可用性我先简单处理
					glm::dvec3 onePos(2 * x, 2 * y, z);
					pos.push_back(onePos);
					vertexID[getPosOfRaster(y, x)] = index;
					index++;
				}
			}
		}

		DelaunayTrianglePtr t = mFirstFace;
		while (t)
		{
			glm::dvec2 p1 = t->point1();
			glm::dvec2 p2 = t->point2();
			glm::dvec2 p3 = t->point3();

			if (TriangleUlit::triCCW(p1,p2,p3))
			{
				indies.push_back(vertexID[getPosOfRaster((int)p1.y, (int)p1.x)]);
				indies.push_back(vertexID[getPosOfRaster((int)p2.y, (int)p2.x)]);
				indies.push_back(vertexID[getPosOfRaster((int)p3.y, (int)p3.x)]);
			}
			else
			{
				indies.push_back(vertexID[getPosOfRaster((int)p3.y, (int)p3.x)]);
				indies.push_back(vertexID[getPosOfRaster((int)p2.y, (int)p2.x)]);
				indies.push_back(vertexID[getPosOfRaster((int)p1.y, (int)p1.x)]);
			}
			t = t->getLink();
		}
		//输出obj
		//简单输出
		if (N > 0)
		{
			exportToObj(pos, indies, "terraTestMesh_" + std::to_string(N) + ".obj");
		}
		else {
			exportToObj(pos, indies, "terraTestMesh.obj");
		}

	}

	std::array<std::vector<glm::dvec3>, 4> TerraMesh::getBoundaryPoints()
	{
		//1.左侧west
		std::vector<glm::dvec3> west=getPointsOnLineX(0);
		//2下册 south
		std::vector<glm::dvec3> south = getPointsOnLineY(mHeigth-1);
		//3 右侧 east
		std::vector<glm::dvec3> east = getPointsOnLineX(mWidth-1);
		//4 上侧 north
		std::vector<glm::dvec3> north = getPointsOnLineY(0);

		std::array<std::vector<glm::dvec3>, 4> boundaryPoints = { west,south,east,north };
		return std::move(boundaryPoints);
	}

	void TerraMesh::exportToObj(const std::vector<glm::dvec3>& positions,
		const std::vector<int>& indices,
		const std::string& filename) {
		std::ofstream outfile(filename);

		if (!outfile.is_open()) {
			throw std::runtime_error("无法打开文件: " + filename);
		}

		// 写入文件头
		outfile << "# 导出的3D模型\n";
		outfile << "# 顶点数: " << positions.size() << "\n";
		outfile << "# 面数: " << indices.size() / 3 << "\n\n";

		// 写入顶点数据 (v x y z)
		for (const auto& pos : positions) {
			outfile << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
		}

		outfile << "\n"; // 顶点和面之间空一行

		// 写入面数据 (f v1 v2 v3)
		// 假设索引是三角形列表形式，每3个索引一个面
		for (size_t i = 0; i < indices.size(); i += 3) {
			// OBJ索引从1开始，所以每个索引+1
			outfile << "f " << indices[i] + 1 << " "
				<< indices[i + 1] + 1 << " "
				<< indices[i + 2] + 1 << "\n";
		}

		outfile.close();
	}

	std::vector<glm::dvec3> TerraMesh::getPointsOnLineX(int x)
	{
		std::vector<glm::dvec3> points;
		for (int y=0;y<mHeigth;++y)
		{
			if (mUsed[getPosOfRaster(y,x)]==1)
			{
				double z=mFileInfo->data[getPosOfRaster(y, x)];
				glm::dvec3 onePos(2 * x, 2 * y, z);
				points.push_back(onePos);
			}
		}
		return std::move(points);
	}

	std::vector<glm::dvec3> TerraMesh::getPointsOnLineY(int y)
	{
		std::vector<glm::dvec3> points;
		for (int x = 0; x < mWidth; ++x)
		{
			if (mUsed[getPosOfRaster(y, x)] == 1)
			{
				double z = mFileInfo->data[getPosOfRaster(y, x)];
				glm::dvec3 onePos(2 * x, 2 * y, z);
				points.push_back(onePos);
			}
		}
		return std::move(points);
	}

};