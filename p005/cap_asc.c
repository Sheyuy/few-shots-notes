#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M7 2187
#define M8 6561
#define N0 512
#define MAXN 1600

/* All point codes are LITTLE-ENDIAN: code = sum_k digit_k * 3^k. */
static short third7[M7][M7];   /* third7[a][b] = code of -(a+b) mod 3 in F_3^7 */
static int pow3[10];

static unsigned long st;
static unsigned int xr(void){st^=st<<13;st^=st>>7;st^=st<<17;return (unsigned int)(st>>11);}

static int digit(int code8,int c){ return (code8/pow3[c])%3; }

/* projection dropping coordinate c -> 7-dim little-endian code */
static int proj8(int code8,int c){
    int z=0,m=0;
    for(int k=0;k<8;k++){ if(k==c) continue; int d=(code8/pow3[k])%3; z+=d*pow3[m]; m++; }
    return z;
}
/* insert value val at coordinate c of a 7-dim little-endian code */
static int lift7(int z,int c,int val){
    int code8=0,m=0;
    for(int k=0;k<8;k++){
        int d;
        if(k==c) d=val; else { d=(z/pow3[m])%3; m++; }
        code8+=d*pow3[k];
    }
    return code8;
}

static int maxcap7(const int*R,int Rn,int restarts,int*bestC,int*ord,char*mem,int*cur){
    int bestn=-1;
    for(int r=0;r<restarts;r++){
        memcpy(ord,R,Rn*sizeof(int));
        for(int i=Rn-1;i>0;i--){int j=xr()%(i+1);int t=ord[i];ord[i]=ord[j];ord[j]=t;}
        memset(mem,0,M7);
        int ncur=0;
        for(int i=0;i<Rn;i++){
            int p=ord[i],ok=1;
            for(int j=0;j<ncur;j++){ if(mem[third7[p][cur[j]]]){ok=0;break;} }
            if(ok){mem[p]=1;cur[ncur++]=p;}
        }
        if(ncur>bestn){bestn=ncur;if(bestC)memcpy(bestC,cur,ncur*sizeof(int));}
    }
    return bestn;
}

int main(int argc,char**argv){
    int restarts=(argc>1)?atoi(argv[1]):40;
    int passes=(argc>2)?atoi(argv[2]):8;
    st=(argc>3)?(unsigned long)atoll(argv[3]):88172645463325252UL; if(!st)st=1;
    pow3[0]=1; for(int k=1;k<10;k++) pow3[k]=pow3[k-1]*3;
    for(int a=0;a<M7;a++)for(int b=0;b<M7;b++){
        int x=a,y=b,z=0,pw=1;
        for(int k=0;k<7;k++){int s=((x%3)+(y%3))%3;int cc=(3-s)%3;z+=cc*pw;pw*=3;x/=3;y/=3;}
        third7[a][b]=(short)z;
    }
    FILE*f=fopen("baseline_pts.txt","r"); if(!f){fprintf(stderr,"no baseline\n");return 1;}
    int pts[MAXN], n=0;
    for(int t=0;t<N0;t++){
        int v[8]; for(int k=0;k<8;k++) if(fscanf(f,"%d",&v[k])!=1){fprintf(stderr,"parse\n");return 1;}
        int c=0; for(int k=0;k<8;k++) c+=v[k]*pow3[k];
        pts[n++]=c;
    }
    fclose(f);

    int *R=malloc(M7*sizeof(int));
    int *bestC=malloc(M7*sizeof(int));
    int *ord=malloc(M7*sizeof(int));
    int *cur=malloc(M7*sizeof(int));
    char *mem=malloc(M7);
    static char in8[M8];

    #define VERIFY() do{ \
        memset(in8,0,M8); for(int i=0;i<n;i++) in8[pts[i]]=1; \
        int bad=0; \
        for(int i=0;i<n&&!bad;i++){ int a=pts[i]; \
          for(int j=i+1;j<n&&!bad;j++){ int b=pts[j]; int cc=0; \
            for(int k=0;k<8;k++){int s=((a/pow3[k])%3+(b/pow3[k])%3)%3; cc+=((3-s)%3)*pow3[k];} \
            if(in8[cc]) bad=1; } } \
        printf("  verify n=%d %s\n",n,bad?"INVALID":"VALID"); \
    }while(0)

    printf("start n=%d\n",n); VERIFY();

    for(int pass=0; pass<passes; pass++){
        int improved=0;
        for(int c=0;c<8;c++){
            for(int i=0;i<3;i++){
                int Li[M7], Ln=0; static char inLi[M7]; memset(inLi,0,M7);
                for(int q=0;q<n;q++){ if(digit(pts[q],c)==i){ int z=proj8(pts[q],c); if(!inLi[z]){inLi[z]=1;Li[Ln++]=z;} } }
                int j=(i+1)%3, k=(i+2)%3;
                int Lj[M7], Ljn=0, Lk[M7], Lkn=0;
                static char inj[M7], ink[M7]; memset(inj,0,M7); memset(ink,0,M7);
                for(int q=0;q<n;q++){ int d=digit(pts[q],c); int z=proj8(pts[q],c);
                    if(d==j && !inj[z]){inj[z]=1;Lj[Ljn++]=z;}
                    if(d==k && !ink[z]){ink[z]=1;Lk[Lkn++]=z;} }
                static char forb[M7]; memset(forb,0,M7);
                for(int u=0;u<Ljn;u++) for(int v2=0;v2<Lkn;v2++) forb[third7[Lj[u]][Lk[v2]]]=1;
                int Rn=0; for(int z=0;z<M7;z++) if(!forb[z]) R[Rn++]=z;
                int m=maxcap7(R,Rn,restarts,bestC,ord,mem,cur);
                if(m>Ln){
                    printf("pass %d coord %d layer %d: %d -> %d (|R|=%d) total %d -> %d\n",
                           pass,c,i,Ln,m,Rn,n,n-Ln+m); fflush(stdout);
                    int nn=0;
                    for(int q=0;q<n;q++) if(digit(pts[q],c)!=i) pts[nn++]=pts[q];
                    for(int u=0;u<m;u++) pts[nn++]=lift7(bestC[u],c,i);
                    n=nn; improved=1;
                    VERIFY();
                }
            }
        }
        if(!improved){ printf("pass %d: no improvement, stop\n",pass); break; }
    }
    printf("FINAL n=%d\n",n); VERIFY();
    if(n>512){
        printf("*** IMPROVED to %d ***\n",n);
        FILE*o=fopen("candidate_pts.txt","w");
        for(int i=0;i<n;i++){ int x=pts[i]; int v[8]; for(int k=0;k<8;k++){v[k]=x%3;x/=3;}
            fprintf(o,"%d %d %d %d %d %d %d %d\n",v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7]); }
        fclose(o);
        printf("wrote candidate_pts.txt\n");
    }
    return 0;
}
