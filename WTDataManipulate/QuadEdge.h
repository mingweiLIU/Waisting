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
//�᳭��https://github.com/heremaps/tin-terrain
//�㷨see http://www.cs.cmu.edu/afs/andrew/scs/cs/15-463/2001/pub/src/a2/quadedge.html
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

        //��������
        //���Ա������Ϊ�������ڱ�
        QuadEdgePtr Onext()const noexcept { return mNext; }

        //�ߵķ����
        QuadEdgePtr Sym()const { return qnextRot->qnextRot; }

        //ָ����ߵĶ�ż��
        QuadEdgePtr Rot()const noexcept { return qnextRot; }

        //ָ���ұߵĶ�ż��
        QuadEdgePtr invRot()const noexcept { return qpreInvRot; }

        //�ϳɷ���
        QuadEdgePtr Oprev()const { return Rot()->Onext()->Rot(); }
        QuadEdgePtr Dnext()const { return Sym()->Onext()->Sym(); }
        QuadEdgePtr Dprev()const { return invRot()->Onext()->invRot(); }

        //�����ϵ�next��
        QuadEdgePtr Lnext()const { return invRot()->Onext()->Rot(); }
        //����ϵ�prev��
        QuadEdgePtr Lprev()const { return Onext()->Sym(); }

        //�����ϵ�next��
        QuadEdgePtr Rnext()const { return Rot()->Onext()->invRot(); }
        //�����ϵ�prev��
        QuadEdgePtr Lprev()const { return Sym()->Onext(); }

        //��ȡ���
        glm::dvec2 Org()const noexcept { return mOrigin; }
        //��ȡ�յ�
        glm::dvec2 Dest()const { return Sym()->mOrigin; }

        //��ȡ�����
        DelaunayTrianglePtr Lface()const noexcept { return mLeftFace; }
        void setLFace(DelaunayTrianglePtr face)noexcept { mLeftFace = face; }

        void setEndPoints(const glm::dvec2 org, const glm::dvec2 dest) {
            mOrigin = org;
            Sym()->mOrigin = dest;
        }

        //���˱༭
        friend void splice(QuadEdgePtr a, QuadEdgePtr b);

        //����ĸ�������
        //�жϵ��Ƿ��ڱߵ��Ҳ�
        bool righOf(const glm::dvec2& x);

        //�жϵ��Ƿ��ڱߵ����
        bool leftOf(const glm::dvec2& x);

    protected:
        glm::dvec2 mOrigin;
        void* data;//Ԥ������������
        QuadEdgePtr mNext;
        DelaunayTrianglePtr mLeftFace;

	private:
        QuadEdgePtr qnextRot;//Rot --qnext
        QuadEdgePtr qpreInvRot;//invRot--qprev
        QuadEdgePtr selfPtr;
        QuadEdge(QuadEdgePtr prev);
	};
};