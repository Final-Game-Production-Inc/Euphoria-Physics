// 
// data/bitfield.h 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_BITFIELD_H 
#define DATA_BITFIELD_H 

// Would be nice if these could assert that the sizes matched up.  No idea how to do that though
// since CompileTimeAssert won't work in the required context.
// Later fields are guaranteed at higher addresses.

// NOTE: If you use these macros, make sure to account for ALL bits in the int type. 
// E.g. if you're using a u32, make sure you specify all 32 bits.
// Otherwise different compilers will put the "unused" bits on different ends, making any bitfield data non-cross-platform.

#if __BE // Backward field order!
#define DECLARE_BITFIELD_2(a,as,b,bs)																			b:bs, a:as
#define DECLARE_BITFIELD_3(a,as,b,bs,c,cs)																		c:cs, b:bs, a:as
#define DECLARE_BITFIELD_4(a,as,b,bs,c,cs,d,ds)																	d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_5(a,as,b,bs,c,cs,d,ds,e,es)															e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_6(a,as,b,bs,c,cs,d,ds,e,es,f,fs)														f:fs, e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_7(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs)													g:gs, f:fs, e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_8(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs)												h:hs, g:gs, f:fs, e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_9(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is)										i:is, h:hs, g:gs, f:fs, e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_10(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js)									j:js, i:is, h:hs, g:gs, f:fs, e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_11(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks)								k:ks, j:js, i:is, h:hs, g:gs, f:fs, e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_12(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks,l,ls)						l:ls, k:ks, j:js, i:is, h:hs, g:gs, f:fs, e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_13(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks,l,ls,m,ms)					m:ms, l:ls, k:ks, j:js, i:is, h:hs, g:gs, f:fs, e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_14(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks,l,ls,m,ms,n,ns)				n:ns, m:ms, l:ls, k:ks, j:js, i:is, h:hs, g:gs, f:fs, e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_15(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks,l,ls,m,ms,n,ns,o,os)			o:os, n:ns, m:ms, l:ls, k:ks, j:js, i:is, h:hs, g:gs, f:fs, e:es, d:ds, c:cs, b:bs, a:as
#define DECLARE_BITFIELD_16(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks,l,ls,m,ms,n,ns,o,os,p,ps)	p:ps, o:os, n:ns, m:ms, l:ls, k:ks, j:js, i:is, h:hs, g:gs, f:fs, e:es, d:ds, c:cs, b:bs, a:as





#else
#define DECLARE_BITFIELD_2(a,as,b,bs)																			a:as, b:bs
#define DECLARE_BITFIELD_3(a,as,b,bs,c,cs)																		a:as, b:bs, c:cs
#define DECLARE_BITFIELD_4(a,as,b,bs,c,cs,d,ds)																	a:as, b:bs, c:cs, d:ds
#define DECLARE_BITFIELD_5(a,as,b,bs,c,cs,d,ds,e,es)															a:as, b:bs, c:cs, d:ds, e:es
#define DECLARE_BITFIELD_6(a,as,b,bs,c,cs,d,ds,e,es,f,fs)														a:as, b:bs, c:cs, d:ds, e:es, f:fs
#define DECLARE_BITFIELD_7(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs)													a:as, b:bs, c:cs, d:ds, e:es, f:fs, g:gs
#define DECLARE_BITFIELD_8(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs)												a:as, b:bs, c:cs, d:ds, e:es, f:fs, g:gs, h:hs
#define DECLARE_BITFIELD_9(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is)										a:as, b:bs, c:cs, d:ds, e:es, f:fs, g:gs, h:hs, i:is
#define DECLARE_BITFIELD_10(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js)									a:as, b:bs, c:cs, d:ds, e:es, f:fs, g:gs, h:hs, i:is, j:js
#define DECLARE_BITFIELD_11(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks)								a:as, b:bs, c:cs, d:ds, e:es, f:fs, g:gs, h:hs, i:is, j:js, k:ks
#define DECLARE_BITFIELD_12(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks,l,ls)						a:as, b:bs, c:cs, d:ds, e:es, f:fs, g:gs, h:hs, i:is, j:js, k:ks, l:ls
#define DECLARE_BITFIELD_13(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks,l,ls,m,ms)					a:as, b:bs, c:cs, d:ds, e:es, f:fs, g:gs, h:hs, i:is, j:js, k:ks, l:ls, m:ms
#define DECLARE_BITFIELD_14(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks,l,ls,m,ms,n,ns)				a:as, b:bs, c:cs, d:ds, e:es, f:fs, g:gs, h:hs, i:is, j:js, k:ks, l:ls, m:ms, n:ns
#define DECLARE_BITFIELD_15(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks,l,ls,m,ms,n,ns,o,os)			a:as, b:bs, c:cs, d:ds, e:es, f:fs, g:gs, h:hs, i:is, j:js, k:ks, l:ls, m:ms, n:ns, o:os
#define DECLARE_BITFIELD_16(a,as,b,bs,c,cs,d,ds,e,es,f,fs,g,gs,h,hs,i,is,j,js,k,ks,l,ls,m,ms,n,ns,o,os,p,ps)	a:as, b:bs, c:cs, d:ds, e:es, f:fs, g:gs, h:hs, i:is, j:js, k:ks, l:ls, m:ms, n:ns, o:os, p:ps
#endif

#endif // DATA_BITFIELD_H 
