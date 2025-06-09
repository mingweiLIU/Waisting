#include "QuadEdge.h"
#include "TriangleUlit.h"
namespace WT {
	/*creates 3 more QuadEdges
	 this=e0
	 e1,e2,e3

	 they are linked together like this:

			  |        ^ e0
			  |  _---->|
	 e1       | /qprev |
	 <--------+--------+----------
		 ^    |        |
		  \___|        |
		qprev |        |___
			  |        |   \ qprev
			  |        |    V
	 ---------+--------+--------->
			  | qprev/ |         e3
			  |<-___/  |
		   e2 V        |
	 */
	void QuadEdge::init(QuadEdgePtr self_ptr)
	{
		this->selfPtr = self_ptr;
		
		QuadEdgePtr e0 = self_ptr;

		QuadEdgePtr e1 = self_ptr.pool()->spawn();
		QuadEdgePtr e2 = self_ptr.pool()->spawn();
		QuadEdgePtr e3 = self_ptr.pool()->spawn();

		e1->init(e1, e0);
		e2->init(e2, e1);
		e3->init(e3, e2);

		e0->qpreInvRot = e3;
		e3->qnextRot = e0;

		e0->mNext = e0;
		e1->mNext = e3;
		e2->mNext = e2;
		e3->mNext = e1;
	}

	void QuadEdge::init(QuadEdgePtr self_ptr, QuadEdgePtr qprev)
	{
		this->selfPtr = self_ptr;
		this->qpreInvRot = qprev;
		qprev->qnextRot = self_ptr;
	}

	void QuadEdge::recycleNext() {
		if (qnextRot)
		{
			QuadEdgePtr e1 = qnextRot;
			QuadEdgePtr e2 = qnextRot->qnextRot;
			QuadEdgePtr e3 = qpreInvRot;

			e1->qnextRot.clear();
			e2->qnextRot.clear();
			e3->qnextRot.clear();

			e1->recycleNext();
			e1.recycle(); 
			e2->recycleNext();
			e2.recycle();
			e3->recycleNext();
			e3.recycle();
		}
	}

	bool QuadEdge::righOf(const glm::dvec2& x)
	{
		return TriangleUlit::rightOf(x, this->Org(), this->Dest());
	}

	bool QuadEdge::leftOf(const glm::dvec2& x)
	{
		return TriangleUlit::leftOf(x, this->Org(), this->Dest());
	}

	void splice(QuadEdgePtr a, QuadEdgePtr b) {
		QuadEdgePtr alpha = a->Onext()->Rot();
		QuadEdgePtr beta = b->Onext()->Rot();

		QuadEdgePtr t1 = b->Onext();
		QuadEdgePtr t2 = a->Onext();
		QuadEdgePtr t3 = beta->Onext();
		QuadEdgePtr t4 = alpha->Onext();

		a->mNext = t1;
		b->mNext = t2;
		alpha->mNext = t3;
		beta->mNext = t4;
	}


};

