#include "DelaunayTriangle.h"
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
		QuadEdgePtr ea = mEdges->spawn();
		ea->init(ea);
		ea->setEndPoints(a, b);

		QuadEdgePtr eb = mEdges->spawn();
		eb->init(eb);
		splice(ea->Sym(), eb);
		eb->setEndPoints(b, c);

		QuadEdgePtr ec = mEdges->spawn();
		ec->init(ec);
		splice(eb->Sym(), ec);
		ec->setEndPoints(c, d);

		QuadEdgePtr ed = mEdges->spawn();
		ed->init(ed);
		splice(ec->Sym(), ed);
		ed->setEndPoints(d, a);
		splice(ed->Sym(), ea);

		QuadEdgePtr diag = mEdges->spawn();
		diag->init(diag);
		splice(ed->Sym(), diag);
		splice(eb->Sym(), diag->Sym());
		diag->setEndPoints(a, c);

		mStartingEdge = ea;

		mFirstFace.clear();

		makeFace(ea->Sym());
		makeFace(ec->Sym());
	}

	bool DelaunayMesh::shouldSwap(const glm::dvec2 p, QuadEdgePtr e)
	{
		QuadEdgePtr t = e->Oprev();
		return inCircle(e->Org(), t->Dest(), e->Dest(), x);
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

		if (boundary_edge) delete_edge(boundary_edge);

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

	void DelaunayMesh::optimize(const glm::dvec2 x, QuadEdgePtr e)
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
			dt_ptr t = e->Lface();

			if (t) this->scanTriagnle(t);

			spoke = spoke->Onext();
		} while (spoke != start_spoke);
	}

	QuadEdgePtr DelaunayMesh::locate(const glm::dvec2 x, QuadEdgePtr hint)
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
					if (t == 0 && !leftOf(eo->Dest(), e))
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
		e->set_end_points(a->Dest(), b->Org());
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
		e->set_end_points(a->Dest(), b->Dest());

		f1->reshape(e);
		f2->reshape(e->Sym());
	}

	bool DelaunayMesh::ccwBoundary(QuadEdgePtr e)
	{
		return !rightOf(e->Oprev()->Dest(), e);
	}

	bool DelaunayMesh::onEdge(const glm::dvec2 p, QuadEdgePtr e)
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



};