#pragma once
#include<glm/glm.hpp>
#include "EntityPool.h"

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
//搬抄自https://github.com/heremaps/tin-terrain
//算法see http://www.cs.cmu.edu/afs/andrew/scs/cs/15-463/2001/pub/src/a2/quadedge.html
namespace WT {
	class QuadEdge;
	class DelaunayTriangle;
	using QuadEdgePtr=pool_ptr <QuadEdge> ;
	using DelaunayTrianglePtr=pool_ptr <DelaunayTriangle> ;

	class QuadEdge
	{
	public:
		QuadEdge()=default;
        void init(QuadEdgePtr self_ptr);
        void init(QuadEdgePtr self_ptr, QuadEdgePtr qprev);
        void recycleNext();

        //基础方法
        //在以本边起点为起点的相邻边
        QuadEdgePtr Onext()const noexcept { return mNext; }

        //边的反向边
        QuadEdgePtr Sym()const { return mRot->mRot; }

        //指向左边的对偶边
        QuadEdgePtr Rot()const noexcept { return mRot; }

        //指向右边的对偶边
        QuadEdgePtr invRot()const noexcept { return mInvRot; }

        //合成方法
        QuadEdgePtr Oprev()const { return Rot()->Onext()->Rot(); }
        QuadEdgePtr Dnext()const { return Sym()->Onext()->Sym(); }
        QuadEdgePtr Dprev()const { return invRot()->Onext()->invRot(); }

        //左面上的next边
        QuadEdgePtr Lnext()const { return invRot()->Onext()->Rot(); }
        //左边上的prev边
        QuadEdgePtr Lprev()const { return Onext()->Sym(); }

        //右面上的next边
        QuadEdgePtr Rnext()const { return Rot()->Onext()->invRot(); }
        //右面上的prev边
        QuadEdgePtr Lprev()const { return Sym()->Onext(); }

        //获取起点
        glm::dvec3 Org()const noexcept { return mOrigin; }
        //获取终点
        glm::dvec3 Dest()const { return Sym()->mOrigin; }

        //获取左侧面
        DelaunayTrianglePtr Lface()const noexcept { return mLeftFace; }
        void setLFace(DelaunayTrianglePtr face)noexcept { mLeftFace = face; }

        void setEndPoints(const glm::dvec3 org, const glm::dvec3 dest) {
            mOrigin = org;
            Sym()->mOrigin = dest;
        }

    protected:
        glm::dvec3 mOrigin;
        void* data;//预留的其他数据
        QuadEdgePtr mNext;
        DelaunayTrianglePtr mLeftFace;

	private:
        QuadEdgePtr mRot;//Rot --qnext
        QuadEdgePtr mInvRot;//invRot--qprev
        QuadEdgePtr selfPtr;
        QuadEdge(QuadEdgePtr prev);
	};
};