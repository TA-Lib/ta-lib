#include <stdio.h>
#include <stdlib.h>
#include "ta_libc.h"
#define N 400
int main(void){
  double x[N]; int full[N], cut[N], fmin[N], cmin[N], beg,nb,b2,n2,i,s,p, nmax=0,nmin=0, shown=0;
  unsigned r=12345;
  TA_Initialize();
  for(i=0;i<N;i++){ r=r*1103515245u+12345u; x[i]=(double)((r>>16)%4); }   /* many ties */
  for(p=2;p<=40;p++){
    TA_MAXINDEX(0,N-1,x,p,&beg,&nb,full); TA_MININDEX(0,N-1,x,p,&beg,&nb,fmin);
    for(s=beg+1;s<N;s+=7){
      TA_MAXINDEX(s,N-1,x,p,&b2,&n2,cut); TA_MININDEX(s,N-1,x,p,&b2,&n2,cmin);
      for(i=0;i<n2;i++){
        if(cut[i]!=full[b2+i-beg]){ nmax++; if(shown<4){shown++; printf("MAXINDEX period %d bar %d: from startIdx 0 -> %d, from startIdx %d -> %d (values %g and %g)\n",p,b2+i,full[b2+i-beg],s,cut[i],x[full[b2+i-beg]],x[cut[i]]);} }
        if(cmin[i]!=fmin[b2+i-beg]) nmin++;
      }
    }
  }
  printf("mismatching bars: MAXINDEX %d, MININDEX %d\n",nmax,nmin);
  return 0; }
