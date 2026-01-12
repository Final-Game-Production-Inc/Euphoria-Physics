

#include "btGjkEpa2.h"

#if ENABLE_EPA_PENETRATION_SOLVER_CODE

#include "phbound/support.h"
#include "math/amath.h"

#if defined(DEBUG) || defined (_DEBUG)
#include <stdio.h> //for debug printf
#ifdef __SPU__
#include <spu_printf.h>
#define printf spu_printf
#endif //__SPU__
#endif

namespace gjkepa2_impl
{

// Config

	/* GJK	*/ 
#define GJK_MAX_ITERATIONS	128
#define GJK_ACCURARY		((float)0.0001)
#define GJK_MIN_DISTANCE	((float)0.0001)
#define GJK_DUPLICATED_EPS	((float)0.0001)
#define GJK_SIMPLEX2_EPS	((float)0.0)
#define GJK_SIMPLEX3_EPS	((float)0.0)
#define GJK_SIMPLEX4_EPS	((float)0.0)

	/* EPA	*/ 
#define EPA_MAX_VERTICES	64
#define EPA_MAX_FACES		(EPA_MAX_VERTICES*2)
#define EPA_MAX_ITERATIONS	255
#define EPA_ACCURACY		((float)0.0001)
#define EPA_FALLBACK		(10*EPA_ACCURACY)
#define EPA_PLANE_EPS		((float)0.00001)
#define EPA_INSIDE_EPS		((float)0.01)


// Shorthands
typedef unsigned int	U;
typedef unsigned char	U1;

// MinkowskiDiff
struct	MinkowskiDiff
	{
	const rage::phBound*	m_shapes[2];
	rage::Matrix34				m_toshape1;
	rage::Matrix34				m_toshape0;
	bool					m_marginEnable;
	void					EnableMargin(bool enable)
		{
			m_marginEnable = enable;
		}	
	inline rage::Vector3		Support0(const rage::Vector3& d) const
		{
			if (m_marginEnable)
				return VEC3V_TO_VECTOR3(m_shapes[0]->LocalGetSupportingVertex(d));
			else
				return VEC3V_TO_VECTOR3(m_shapes[0]->LocalGetSupportingVertexWithoutMarginNotInlined(d));
		}
	inline rage::Vector3		Support1(const rage::Vector3& d) const
		{
			rage::Vector3 Xd;
			rage::Vector3 support;

			// It should be fine for this to be just a transform too, as we zero out the d component of the matrix.
			m_toshape1.Transform3x3(d,Xd);
			if (m_marginEnable)
				support = VEC3V_TO_VECTOR3(m_shapes[1]->LocalGetSupportingVertex(Xd));
			else
				support = VEC3V_TO_VECTOR3(m_shapes[1]->LocalGetSupportingVertexWithoutMarginNotInlined(Xd));

			m_toshape0.Transform(support);
			return support;
		}
	inline rage::Vector3		Support(const rage::Vector3& d) const
		{
		return(Support0(d)-Support1(-d));
		}
	rage::Vector3				Support(const rage::Vector3& d,U index) const
		{
		if(index)
			return(Support1(d));
			else
			return(Support0(d));
		}
	};

typedef	MinkowskiDiff	tShape;

// GJK
struct	GJK
{
/* Types		*/ 
struct	sSV
	{
	rage::Vector3	d,w;
	};
struct	sSimplex
	{
	sSV*		c[4];
	float	p[4];
	U			rank;
	};
struct	eStatus	{ enum _ {
	Valid,
	Inside,
	Failed		};};
/* Fields		*/ 
tShape			m_shape;
rage::Vector3		m_ray;
float		m_distance;
sSimplex		m_simplices[2];
sSV				m_store[4];
sSV*			m_free[4];
U				m_nfree;
U				m_current;
sSimplex*		m_simplex;
eStatus::_		m_status;
/* Methods		*/ 
					GJK()
	{
	Initialize();
	}
void				Initialize()
	{
	m_ray		=	rage::Vector3(0,0,0);
	m_nfree		=	0;
	m_status	=	eStatus::Failed;
	m_current	=	0;
	m_distance	=	0;
	}
eStatus::_			Evaluate(const tShape& shapearg,const rage::Vector3& guess)
	{
	U			iterations=0;
	float	sqdist=0;
	float	alpha=0;
	rage::Vector3	lastw[4];
	U			clastw=0;
	/* Initialize solver		*/ 
	m_free[0]			=	&m_store[0];
	m_free[1]			=	&m_store[1];
	m_free[2]			=	&m_store[2];
	m_free[3]			=	&m_store[3];
	m_nfree				=	4;
	m_current			=	0;
	m_status			=	eStatus::Valid;
	m_shape				=	shapearg;
	m_distance			=	0;
	/* Initialize simplex		*/ 
	m_simplices[0].rank	=	0;
	m_ray				=	guess;
	const float	sqrl=	m_ray.Mag2();
	appendvertice(m_simplices[0],sqrl>0?-m_ray:rage::Vector3(1,0,0));
	m_simplices[0].p[0]	=	1;
	m_ray				=	m_simplices[0].c[0]->w;	
	sqdist				=	sqrl;
	lastw[0]			=
	lastw[1]			=
	lastw[2]			=
	lastw[3]			=	m_ray;
	/* Loop						*/ 
	do	{
		const U		next=1-m_current;
		sSimplex&	cs=m_simplices[m_current];
		sSimplex&	ns=m_simplices[next];
		/* Check zero							*/ 
		const float	rl=m_ray.Mag();
		if(rl<GJK_MIN_DISTANCE)
			{/* Touching or inside				*/ 
			m_status=eStatus::Inside;
			break;
			}
		/* Append new vertice in -'v' direction	*/ 
		appendvertice(cs,-m_ray);
		const rage::Vector3&	w=cs.c[cs.rank-1]->w;
		bool				found=false;
		for(U i=0;i<4;++i)
			{
			if((w-lastw[i]).Mag2()<GJK_DUPLICATED_EPS)
				{ found=true;break; }
			}
		if(found)
			{/* Return old simplex				*/ 
			removevertice(m_simplices[m_current]);
			break;
			}
			else
			{/* Update lastw					*/ 
			lastw[clastw=(clastw+1)&3]=w;
			}
		/* Check for termination				*/ 
		const float	omega= m_ray.Dot(w)/rl;
		alpha= omega > alpha ? omega : alpha;
		if(((rl-alpha)-(GJK_ACCURARY*rl))<=0)
			{/* Return old simplex				*/ 
			removevertice(m_simplices[m_current]);
			break;
			}		
		/* Reduce simplex						*/ 
		float	weights[4];
		U			mask=0;
		switch(cs.rank)
			{
			case	2:	sqdist=projectorigin(	cs.c[0]->w,
												cs.c[1]->w,
												weights,mask);break;
			case	3:	sqdist=projectorigin(	cs.c[0]->w,
												cs.c[1]->w,
												cs.c[2]->w,
												weights,mask);break;
			case	4:	sqdist=projectorigin(	cs.c[0]->w,
												cs.c[1]->w,
												cs.c[2]->w,
												cs.c[3]->w,
												weights,mask);break;
			}
		if(sqdist>=0)
			{/* Valid	*/ 
			ns.rank		=	0;
			m_ray		=	rage::Vector3(0,0,0);
			m_current	=	next;
			U flag = 1;
			for(U i=0,ni=cs.rank;i<ni;++i)
			{
				if(mask & flag)
				{
					ns.c[ns.rank]		=	cs.c[i];
					ns.p[ns.rank++]		=	weights[i];
					m_ray				+=	cs.c[i]->w*weights[i];
				}
				else
				{
					m_free[m_nfree++]	=	cs.c[i];
				}
				flag <<= 1;
			}
			if(mask==15) m_status=eStatus::Inside;
			}
			else
			{/* Return old simplex				*/ 
			removevertice(m_simplices[m_current]);
			break;
			}
		m_status=((++iterations)<GJK_MAX_ITERATIONS)?m_status:eStatus::Failed;
		} while(m_status==eStatus::Valid);
	m_simplex=&m_simplices[m_current];
	switch(m_status)
		{
		case	eStatus::Valid:		m_distance=m_ray.Mag();break;
		case	eStatus::Inside:	m_distance=0;break;
		case    eStatus::Failed: break;
		}	
	return(m_status);
	}
bool					EncloseOrigin()
	{
	switch(m_simplex->rank)
		{
		case	1:
			{
			for(U i=0;i<3;++i)
				{
				rage::Vector3		axis=rage::Vector3(0,0,0);
				axis[i]=1;
				appendvertice(*m_simplex, axis);
				if(EncloseOrigin())	return(true);
				removevertice(*m_simplex);
				appendvertice(*m_simplex,-axis);
				if(EncloseOrigin())	return(true);
				removevertice(*m_simplex);
				}
			}
		break;
		case	2:
			{
			const rage::Vector3	d=m_simplex->c[1]->w-m_simplex->c[0]->w;
			for(U i=0;i<3;++i)
				{
				rage::Vector3		axis=rage::Vector3(0,0,0);
				axis[i]=1;
				if(fabsf(axis.Dot(d))>0)
					{
					rage::Vector3	p= d;
					p.Cross(axis);
					appendvertice(*m_simplex, p);
					if(EncloseOrigin())	return(true);
					removevertice(*m_simplex);
					appendvertice(*m_simplex,-p);
					if(EncloseOrigin())	return(true);
					removevertice(*m_simplex);
					}
				}
			}
		break;
		case	3:
			{
			rage::Vector3	n= m_simplex->c[1]->w-m_simplex->c[0]->w;
			n.Cross(m_simplex->c[2]->w-m_simplex->c[0]->w);
			const float	l=n.Mag();
			if(l>0)
				{
				appendvertice(*m_simplex,n);
				if(EncloseOrigin())	return(true);
				removevertice(*m_simplex);
				appendvertice(*m_simplex,-n);
				if(EncloseOrigin())	return(true);
				removevertice(*m_simplex);
				}
			}
		break;
		case	4:
			{
			if(fabsf(det(	m_simplex->c[0]->w-m_simplex->c[3]->w,
							m_simplex->c[1]->w-m_simplex->c[3]->w,
							m_simplex->c[2]->w-m_simplex->c[3]->w))>0)
				return(true);
			}
		break;
		}
	return(false);
	}
/* Internals	*/ 
void				getsupport(const rage::Vector3& d,sSV& sv) const
	{
	sv.d	=	d/d.Mag();
	sv.w	=	m_shape.Support(sv.d);
	}
void				removevertice(sSimplex& simplex)
	{
	m_free[m_nfree++]=simplex.c[--simplex.rank];
	}
void				appendvertice(sSimplex& simplex,const rage::Vector3& v)
	{
	simplex.p[simplex.rank]=0;
	simplex.c[simplex.rank]=m_free[--m_nfree];
	getsupport(v,*simplex.c[simplex.rank++]);
	}
static float		det(const rage::Vector3& a,const rage::Vector3& b,const rage::Vector3& c)
	{
	return(	a.GetY()*b.GetZ()*c.GetX()+a.GetZ()*b.GetX()*c.GetY()-
			a.GetX()*b.GetZ()*c.GetY()-a.GetY()*b.GetX()*c.GetZ()+
			a.GetX()*b.GetY()*c.GetZ()-a.GetZ()*b.GetY()*c.GetX());
	}
static float		projectorigin(	const rage::Vector3& a,
									const rage::Vector3& b,
									float* w,U& m)
	{
	const rage::Vector3	d=b-a;
	const float	l=d.Mag2();
	if(l>GJK_SIMPLEX2_EPS)
		{
		const float	t(l>0?-a.Dot(d)/l:0);
		if(t>=1)		{ w[0]=0;w[1]=1;m=2;return(b.Mag2()); }
		else if(t<=0)	{ w[0]=1;w[1]=0;m=1;return(a.Mag2()); }
		else			{ w[0]=1-(w[1]=t);m=3;return((a+d*t).Mag2()); }
		}
	return(-1);
	}
static float		projectorigin(	const rage::Vector3& a,
									const rage::Vector3& b,
									const rage::Vector3& c,
									float* w,U& m)
	{
	static const U		imd3[]={1,2,0};
	const rage::Vector3*	vt[]={&a,&b,&c};
	const rage::Vector3		dl[]={a-b,b-c,c-a};
	rage::Vector3		n=dl[0];
	n.Cross(dl[1]);
	const float		l=n.Mag2();
	if(l>GJK_SIMPLEX3_EPS)
		{
		float	mindist=-1;
		float	subw[2] = {0.0f, 0.0f};
		U			subm = 0;
		U flag = 1;
		for(U i=0;i<3;++i)
		{
			rage::Vector3 t = dl[i];
			t.Cross(n);
			if(t.Dot(*vt[i])>0)
			{
				const U			j=imd3[i];
				const float	subd(projectorigin(*vt[i],*vt[j],subw,subm));
				if((mindist<0)||(subd<mindist))
				{
					mindist		=	subd;
					m			=	((subm&1)? flag :0)+((subm&2)?1<<j:0);
					w[i]		=	subw[0];
					w[j]		=	subw[1];
					w[imd3[j]]	=	0;				
				}
			}
			flag <<= 1;
		}
		if(mindist<0)
			{
			const float	d=a.Dot(n);	
			const float	s=sqrtf(l);
			const rage::Vector3	p=n*(d/l);
			mindist	=	p.Mag2();
			m		=	7;
			rage::Vector3 temp = dl[1];
			temp.Cross(b-p);
			w[0]	=	temp.Mag()/s;
			temp = dl[2];
			temp.Cross(c-p);
			w[1]	=	temp.Mag()/s;
			w[2]	=	1-(w[0]+w[1]);
			}
		return(mindist);
		}
	return(-1);
	}
static float		projectorigin(	const rage::Vector3& a,
									const rage::Vector3& b,
									const rage::Vector3& c,
									const rage::Vector3& d,
									float* w,U& m)
	{
	static const U		imd3[]={1,2,0};
	const rage::Vector3*	vt[]={&a,&b,&c,&d};
	const rage::Vector3		dl[]={a-d,b-d,c-d};
	const float		vl=det(dl[0],dl[1],dl[2]);
	rage::Vector3 temp = b-c;
	temp.Cross(a-b);
	const bool			ng=(vl*a.Dot(temp))<=0;
	if(ng&&(fabsf(vl)>GJK_SIMPLEX4_EPS))
		{
		float	mindist=-1;
		float	subw[3] = {0.0f, 0.0f, 0.0f};
		U			subm = 0;
		U flag = 1;
		for(U i=0;i<3;++i)
		{
			const U			j=imd3[i];
			temp = dl[i];
			temp.Cross(dl[j]);
			const float	s=vl*d.Dot(temp);
			if(s>0)
			{
				const float	subd=projectorigin(*vt[i],*vt[j],d,subw,subm);
				if((mindist<0)||(subd<mindist))
				{
					mindist		=	subd;
					m			=	(subm&1? flag :0)+
									(subm&2?1<<j:0)+
									(subm&4?8:0);
					w[i]		=	subw[0];
					w[j]		=	subw[1];
					w[imd3[j]]	=	0;
					w[3]		=	subw[2];
				}
			}
			flag <<= 1;
		}
		if(mindist<0)
			{
			mindist	=	0;
			m		=	15;
			w[0]	=	det(c,b,d)/vl;
			w[1]	=	det(a,c,d)/vl;
			w[2]	=	det(b,a,d)/vl;
			w[3]	=	1-(w[0]+w[1]+w[2]);
			}
		return(mindist);
		}
	return(-1);
	}
};

// EPA
struct	EPA
{
/* Types		*/ 
typedef	GJK::sSV	sSV;
struct	sFace
	{
	rage::Vector3	n;
	float	d;
	float	p;
	sSV*		c[3];
	sFace*		f[3];
	sFace*		l[2];
	U1			e[3];
	U1			pass;
	};
struct	sList
	{
	sFace*		root;
	U			count;
				sList() : root(0),count(0)	{}
	};
struct	sHorizon
	{
	sFace*		cf;
	sFace*		ff;
	U			nf;
				sHorizon() : cf(0),ff(0),nf(0)	{}
	};
struct	eStatus { enum _ {
	Valid,
	Touching,
	Degenerated,
	NonConvex,
	InvalidHull,		
	OutOfFaces,
	OutOfVertices,
	AccuraryReached,
	FallBack,
	Failed,		};};
/* Fields		*/ 
eStatus::_		m_status;
GJK::sSimplex	m_result;
rage::Vector3		m_normal;
float		m_depth;
sSV				m_sv_store[EPA_MAX_VERTICES];
sFace			m_fc_store[EPA_MAX_FACES];
U				m_nextsv;
sList			m_hull;
sList			m_stock;
/* Methods		*/ 
					EPA()
	{
	Initialize();	
	}
void				Initialize()
	{
	m_status	=	eStatus::Failed;
	m_normal	=	rage::Vector3(0,0,0);
	m_depth		=	0;
	m_nextsv	=	0;
	for(U i=0;i<EPA_MAX_FACES;++i)
		{
		append(m_stock,&m_fc_store[EPA_MAX_FACES-i-1]);
		}
	}
eStatus::_			Evaluate(GJK& gjk,const rage::Vector3& guess)
	{
	GJK::sSimplex&	simplex=*gjk.m_simplex;
	if((simplex.rank>1)&&gjk.EncloseOrigin())
		{
		/* Clean up				*/ 
		while(m_hull.root)
			{
			sFace*	f(m_hull.root);
			remove(m_hull,f);
			append(m_stock,f);
			}
		m_status	=	eStatus::Valid;
		m_nextsv	=	0;
		/* Orient simplex		*/ 
		if(gjk.det(	simplex.c[0]->w-simplex.c[3]->w,
					simplex.c[1]->w-simplex.c[3]->w,
					simplex.c[2]->w-simplex.c[3]->w)<0)
			{
				rage::SwapEm(simplex.c[0],simplex.c[1]);
				rage::SwapEm(simplex.p[0],simplex.p[1]);
			}
		/* Build initial hull	*/ 
		sFace*	tetra[]={newface(simplex.c[0],simplex.c[1],simplex.c[2],true),
						newface(simplex.c[1],simplex.c[0],simplex.c[3],true),
						newface(simplex.c[2],simplex.c[1],simplex.c[3],true),
						newface(simplex.c[0],simplex.c[2],simplex.c[3],true)};
		if(m_hull.count==4)
			{
			sFace*		best=findbest();
			sFace		outer=*best;
			U			pass=0;
			U			iterations=0;
			bind(tetra[0],0,tetra[1],0);
			bind(tetra[0],1,tetra[2],0);
			bind(tetra[0],2,tetra[3],0);
			bind(tetra[1],1,tetra[3],2);
			bind(tetra[1],2,tetra[2],1);
			bind(tetra[2],2,tetra[3],1);
			m_status=eStatus::Valid;
			for(;iterations<EPA_MAX_ITERATIONS;++iterations)
				{
				if(m_nextsv<EPA_MAX_VERTICES)
					{	
					sHorizon		horizon;
					sSV*			w=&m_sv_store[m_nextsv++];
					bool			valid=true;					
					best->pass	=	(U1)(++pass);
					gjk.getsupport(best->n,*w);
					const float	wdist=best->n.Dot(w->w)-best->d;
					if(wdist>EPA_ACCURACY)
						{
						for(U j=0;(j<3)&&valid;++j)
							{
							valid&=expand(	pass,w,
											best->f[j],best->e[j],
											horizon);
							}
						if(valid&&(horizon.nf>=3))
							{
							bind(horizon.cf,1,horizon.ff,2);
							remove(m_hull,best);
							append(m_stock,best);
							best=findbest();
							if(best->p>=outer.p) outer=*best;
							} else { m_status=eStatus::InvalidHull;break; }
						} else { m_status=eStatus::AccuraryReached;break; }
					} else { m_status=eStatus::OutOfVertices;break; }
				}
			const rage::Vector3	projection=outer.n*outer.d;
			m_normal	=	outer.n;
			m_depth		=	outer.d;
			m_result.rank	=	3;
			m_result.c[0]	=	outer.c[0];
			m_result.c[1]	=	outer.c[1];
			m_result.c[2]	=	outer.c[2];
			rage::Vector3 temp = outer.c[1]->w-projection;
			temp.Cross (outer.c[2]->w-projection);
			m_result.p[0]	=	temp.Mag();
			temp = outer.c[2]->w-projection;
			temp.Cross(outer.c[0]->w-projection);
			m_result.p[1]	=	temp.Mag();
			temp = outer.c[0]->w-projection;
			temp.Cross(outer.c[1]->w-projection);
			m_result.p[2]	=	temp.Mag();
			const float	sum=m_result.p[0]+m_result.p[1]+m_result.p[2];
			m_result.p[0]	/=	sum;
			m_result.p[1]	/=	sum;
			m_result.p[2]	/=	sum;
			return(m_status);
			}
		}
	/* Fallback		*/ 
	m_status	=	eStatus::FallBack;
	m_normal	=	-guess;
	const float	nl=m_normal.Mag();
	if(nl>0)
		m_normal	=	m_normal/nl;
		else
		m_normal	=	rage::Vector3(1,0,0);
	m_depth	=	0;
	m_result.rank=1;
	m_result.c[0]=simplex.c[0];
	m_result.p[0]=1;	
	return(m_status);
	}
sFace*				newface(sSV* a,sSV* b,sSV* c,bool forced)
	{
	if(m_stock.root)
		{
		sFace*	face=m_stock.root;
		remove(m_stock,face);
		append(m_hull,face);
		face->pass	=	0;
		face->c[0]	=	a;
		face->c[1]	=	b;
		face->c[2]	=	c;
		face->n		=	b->w-a->w;
		face->n.Cross(c->w-a->w);
		const float	l=face->n.Mag();
		const bool		v=l>EPA_ACCURACY;
		rage::Vector3 temp1 = face->n;
		temp1.Cross(a->w-b->w);
		rage::Vector3 temp2 = face->n;
		rage::Vector3 temp3 = face->n;
		temp2.Cross(b->w-c->w);
		temp3.Cross(c->w-a->w);

		face->p		=	rage::Min(rage::Min(
							a->w.Dot(temp1),
							b->w.Dot(temp2)),
							c->w.Dot(temp3))	/
							(v?l:1);
		face->p		=	face->p>=-EPA_INSIDE_EPS?0:face->p;
		if(v)
			{
			face->d		=	a->w.Dot(face->n)/l;
			face->n		/=	l;
			if(forced||(face->d>=-EPA_PLANE_EPS))
				{
				return(face);
				} else m_status=eStatus::NonConvex;
			} else m_status=eStatus::Degenerated;
		remove(m_hull,face);
		append(m_stock,face);
		return(0);
		}
	m_status=m_stock.root?eStatus::OutOfVertices:eStatus::OutOfFaces;
	return(0);
	}
sFace*				findbest()
	{
	sFace*		minf=m_hull.root;
	float	mind=minf->d*minf->d;
	float	maxp=minf->p;
	for(sFace* f=minf->l[1];f;f=f->l[1])
		{
		const float	sqd=f->d*f->d;
		if((f->p>=maxp)&&(sqd<mind))
			{
			minf=f;
			mind=sqd;
			maxp=f->p;
			}
		}
	return(minf);
	}
bool				expand(U pass,sSV* w,sFace* f,U e,sHorizon& horizon)
	{
	static const U	i1m3[]={1,2,0};
	static const U	i2m3[]={2,0,1};
	if(f->pass!=pass)
		{
		const U	e1=i1m3[e];
		if((f->n.Dot(w->w)-f->d)<-EPA_PLANE_EPS)
			{
			sFace*	nf=newface(f->c[e1],f->c[e],w,false);
			if(nf)
				{
				bind(nf,0,f,e);
				if(horizon.cf) bind(horizon.cf,1,nf,2); else horizon.ff=nf;
				horizon.cf=nf;
				++horizon.nf;
				return(true);
				}
			}
			else
			{
			const U	e2=i2m3[e];
			f->pass		=	(U1)pass;
			if(	expand(pass,w,f->f[e1],f->e[e1],horizon)&&
				expand(pass,w,f->f[e2],f->e[e2],horizon))
				{
				remove(m_hull,f);
				append(m_stock,f);
				return(true);
				}
			}
		}
	return(false);
	}
static inline void		bind(sFace* fa,U ea,sFace* fb,U eb)
	{
	fa->e[ea]=(U1)eb;fa->f[ea]=fb;
	fb->e[eb]=(U1)ea;fb->f[eb]=fa;
	}
static inline void		append(sList& list,sFace* face)
	{
	face->l[0]	=	0;
	face->l[1]	=	list.root;
	if(list.root) list.root->l[0]=face;
	list.root	=	face;
	++list.count;
	}
static inline void		remove(sList& list,sFace* face)
	{
	if(face->l[1]) face->l[1]->l[0]=face->l[0];
	if(face->l[0]) face->l[0]->l[1]=face->l[1];
	if(face==list.root) list.root=face->l[1];
	--list.count;
	}
};

//
static void	Initialize(	const rage::phBound* shape0,const rage::Matrix34& wtrs0,
						const rage::phBound* shape1,const rage::Matrix34& wtrs1,
						btGjkEpaSolver2::sResults& results,
						tShape& shape,
						bool withmargins)
{
/* Results		*/ 
results.witnesses[0]	=
results.witnesses[1]	=	rage::Vector3(0,0,0);
results.status			=	btGjkEpaSolver2::sResults::Separated;
/* Shape		*/ 
shape.m_shapes[0]		=	shape0;
shape.m_shapes[1]		=	shape1;

// wtrs1.R^T * wtrs0.R
//shape.m_toshape1		=	wtrs1.getBasis().transposeTimes(wtrs0.getBasis());
shape.m_toshape1.Set(wtrs0);
rage::Matrix34 temp;
temp.Transpose(wtrs1);
shape.m_toshape1.Dot(temp);
//shape.m_toshape1.Dot3x3Transpose(wtrs1,wtrs0);
shape.m_toshape1.d.Zero();

// wtrs0^I * wtrs1
//shape.m_toshape0		=	wtrs0.inverseTimes(wtrs1);
//shape.m_toshape0.FastInverse(wtrs0);
//shape.m_toshape0.Dot(wtrs1);
shape.m_toshape0.Set(wtrs1);
temp.FastInverse(wtrs0);
shape.m_toshape0.Dot(temp);

shape.EnableMargin(withmargins);
}

}

//
// Api
//

using namespace	gjkepa2_impl;

//
int			btGjkEpaSolver2::StackSizeRequirement()
{
return(sizeof(GJK)+sizeof(EPA));
}

//
#if 0
float	btGjkEpaSolver2::Distance(	const rage::phBound*	shape0,
										const rage::Matrix34&		wtrs0,
										const rage::phBound*	shape1,
										const rage::Matrix34&		wtrs1,
										sResults&				results)
{
tShape			shape;
Initialize(shape0,wtrs0,shape1,wtrs1,results,shape,false);
GJK				gjk;	
GJK::eStatus::_	gjk_status=gjk.Evaluate(shape,rage::Vector3(1,1,1));
if(gjk_status==GJK::eStatus::Valid)
	{
	rage::Vector3	w0=rage::Vector3(0,0,0);
	rage::Vector3	w1=rage::Vector3(0,0,0);
	for(U i=0;i<gjk.m_simplex->rank;++i)
		{
		const float	p=gjk.m_simplex->p[i];
		w0+=shape.Support( gjk.m_simplex->c[i]->d,0)*p;
		w1+=shape.Support(-gjk.m_simplex->c[i]->d,1)*p;
		}
	results.witnesses[0]	=	wtrs0*w0;
	results.witnesses[1]	=	wtrs0*w1;
	return((w0-w1).Mag());
	}
	else
	{
	results.status	=	gjk_status==GJK::eStatus::Inside?
							sResults::Penetrating	:
							sResults::GJK_Failed	;
	return(-1);
	}
}

//
float	btGjkEpaSolver2::SignedDistance(const rage::Vector3& position,
											float margin,
											const rage::phBound* shape0,
											const rage::Matrix34& wtrs0,
											sResults& results)
{
tShape			shape;
btSphereShape	shape1(margin);
rage::Matrix34		wtrs1(btQuaternion(0,0,0,1),position);
Initialize(shape0,wtrs0,&shape1,wtrs1,results,shape,false);
GJK				gjk;	
GJK::eStatus::_	gjk_status=gjk.Evaluate(shape,rage::Vector3(1,1,1));
if(gjk_status==GJK::eStatus::Valid)
	{
	rage::Vector3	w0=rage::Vector3(0,0,0);
	rage::Vector3	w1=rage::Vector3(0,0,0);
	for(U i=0;i<gjk.m_simplex->rank;++i)
		{
		const float	p=gjk.m_simplex->p[i];
		w0+=shape.Support( gjk.m_simplex->c[i]->d,0)*p;
		w1+=shape.Support(-gjk.m_simplex->c[i]->d,1)*p;
		}
	results.witnesses[0]	=	wtrs0*w0;
	results.witnesses[1]	=	wtrs0*w1;
	const rage::Vector3	delta=	results.witnesses[1]-
							results.witnesses[0];
	const float	margin=	shape0->getMargin()+
							shape1.getMargin();
	const float	Mag=	delta.Mag();	
	results.normal			=	delta/Mag;
	results.witnesses[0]	+=	results.normal*margin;
	return(Mag-margin);
	}
	else
	{
	if(gjk_status==GJK::eStatus::Inside)
		{
		if(Penetration(shape0,wtrs0,&shape1,wtrs1,gjk.m_ray,results))
			{
			const rage::Vector3	delta=	results.witnesses[0]-
									results.witnesses[1];
			const float	Mag=	delta.Mag();
			results.normal	=	delta/Mag;			
			return(-Mag);
			}
		}	
	}
return(SIMD_INFINITY);
}
#endif

//
bool	btGjkEpaSolver2::Penetration(	const rage::phBound*	shape0,
										const rage::Matrix34&		wtrs0,
										const rage::phBound*	shape1,
										const rage::Matrix34&		wtrs1,
										const rage::Vector3&		guess,
										sResults&				results)
{
tShape			shape;
Initialize(shape0,wtrs0,shape1,wtrs1,results,shape,true);
GJK				gjk;	
GJK::eStatus::_	gjk_status=gjk.Evaluate(shape,-guess);
switch(gjk_status)
	{
	case	GJK::eStatus::Inside:
		{
		EPA				epa;
		EPA::eStatus::_	epa_status=epa.Evaluate(gjk,-guess);
		if(epa_status!=EPA::eStatus::Failed)
			{
			rage::Vector3	w0=rage::Vector3(0,0,0);
			for(U i=0;i<epa.m_result.rank;++i)
				{
				w0+=shape.Support(epa.m_result.c[i]->d,0)*epa.m_result.p[i];
				}
			results.status			=	sResults::Penetrating;
			results.witnesses[0]	=	w0;
			wtrs0.Transform(results.witnesses[0]);
			results.witnesses[1]	=	w0-epa.m_normal*epa.m_depth;
			wtrs0.Transform(results.witnesses[1]);
			return(true);
			} else results.status=sResults::EPA_Failed;
		}
	break;
	case	GJK::eStatus::Failed:
	results.status=sResults::GJK_Failed;
	break;
	case GJK::eStatus::Valid:
		// no case given
	break;
	}
return(false);
}

/* Symbols cleanup		*/ 

#undef GJK_MAX_ITERATIONS
#undef GJK_ACCURARY
#undef GJK_MIN_DISTANCE
#undef GJK_DUPLICATED_EPS
#undef GJK_SIMPLEX2_EPS
#undef GJK_SIMPLEX3_EPS
#undef GJK_SIMPLEX4_EPS

#undef EPA_MAX_VERTICES
#undef EPA_MAX_FACES
#undef EPA_MAX_ITERATIONS
#undef EPA_ACCURACY
#undef EPA_FALLBACK
#undef EPA_PLANE_EPS
#undef EPA_INSIDE_EPS

#endif // ENABLE_EPA_PENETRATION_SOLVER_CODE
