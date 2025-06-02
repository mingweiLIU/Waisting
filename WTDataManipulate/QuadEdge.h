#pragma once
#include "EntityPool.h"

//see http://www.cs.cmu.edu/afs/andrew/scs/cs/15-463/2001/pub/src/a2/quadedge.html
/*

      \_                 _/
        \_             _/
          \_    |    _/
            \_  |  _/
              \_*_/
                |e->Dest
                |
                ^
       *        |e          *
    e->Left     |         e->Right
                |e->Org
               _*_
             _/ | \_
           _/   |   \_
         _/     |     \_
       _/               \_
      /                   \




            next=ccw
      \_                 _/
        \_             _/
          \_    |    _/
    <Lnext  \_  |  _/   <Dnext
              \_|_/
                | Dest
                |        +--    <---+
                ^        | next=ccw |
                |e       +----------+
                |
                |
               _|_Org
             _/ | \_
    <Onext _/   |   \_ <Rnext
         _/     |     \_
       _/               \_
      /                   \





            prev=cw
      \_                 _/
        \_             _/
          \_    |    _/
    >Dprev  \_  |  _/  >Rprev
              \_|_/
                | Dest
                |
                ^        +--->   --+
                |e       | prev=cw |
                |        +---------+
                |
               _|_Org
             _/ | \_
    >Lprev _/   |   \_ >Oprev
         _/     |     \_
       _/               \_
      /                   \



                *
                |
                v e->Sym
    e->InvRot   |
    *----->-----|-------<-----*
                |       e->Rot
              e ^
                |
                *


 */

namespace WT {
	class QuadEdge;
	class DelaunayTriangle;
	using pool_ptr < QuadEdge >= qe_ptr;
	using pool_ptr < DelaunayTriangle >= dtri_ptr;

	class QuadEdge
	{
	public:
		QuadEdge()=default;
        void init(qe_ptr self_ptr);
        void init(qe_ptr self_ptr, qe_ptr qprev);

	private:

	};
};