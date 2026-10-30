#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace std;
static const int N=6561, PN=8192, TRUE_W=INT_MAX, FALSE_W=0;
static uint8_t hi[N],lo[N],third4[81][81],fixedv[N];
static inline int third(int a,int b){return third4[hi[a]][hi[b]]*81+third4[lo[a]][lo[b]];}
static FILE *f;static int nextvar=N+1;static long long clauses=0,capclauses=0,comparators=0;
static void emit(initializer_list<int> ls){for(int x:ls)fprintf(f,"%d ",x);fputs("0\n",f);clauses++;}
static void cap_clause(int a,int b,int c){
 int ws[3]={a,b,c},lits[3],n=0;
 for(int w:ws){if(w==FALSE_W)return;if(w==TRUE_W)continue;lits[n++]=-w;}
 if(n==0){fprintf(stderr,"empty cap clause\n");exit(4);}
 for(int i=0;i<n;i++)fprintf(f,"%d ",lits[i]);fputs("0\n",f);clauses++;capclauses++;
}
static void comparator(int a,int b,int &hiout,int &loout){
 comparators++;
 if(a==TRUE_W||b==TRUE_W){hiout=TRUE_W;loout=(a==TRUE_W?b:a);return;}
 if(a==FALSE_W){hiout=b;loout=FALSE_W;return;}if(b==FALSE_W){hiout=a;loout=FALSE_W;return;}
 if(a==b){hiout=loout=a;return;}
 int mx=nextvar++,mn=nextvar++;hiout=mx;loout=mn;
 emit({-a,mx});emit({-b,mx});emit({a,b,-mx});
 emit({-mn,a});emit({-mn,b});emit({mn,-a,-b});
}
int main(int argc,char**argv){
 if(argc<2){fprintf(stderr,"usage: gen_global_sat out.cnf [target] [extra_fixed_point]\n");return 2;}int target=argc>2?atoi(argv[2]):513;
 for(int x=0;x<N;x++){hi[x]=x/81;lo[x]=x%81;}for(int a=0;a<81;a++)for(int b=0;b<81;b++){int aa=a,bb=b,w=0,p=1;for(int i=0;i<4;i++){w+=((3-(aa%3+bb%3)%3)%3)*p;aa/=3;bb/=3;p*=3;}third4[a][b]=w;}
 int basis[9]={0,2187,729,243,81,27,9,3,1};for(int x:basis)fixedv[x]=1;
 for(int i=0;i<9;i++)for(int j=i+1;j<9;j++){int z=third(basis[i],basis[j]);if(!fixedv[z])fixedv[z]=2;else if(fixedv[z]!=2){fprintf(stderr,"basis conflict\n");return 3;}}
 if(argc>3){int x=atoi(argv[3]);if(x<0||x>=N||fixedv[x]){fprintf(stderr,"invalid extra fixed point %d state %d\n",x,x>=0&&x<N?fixedv[x]:-1);return 3;}fixedv[x]=1;for(int i=0;i<9;i++){int z=third(x,basis[i]);if(fixedv[z]==1){fprintf(stderr,"extra point conflict %d %d %d\n",x,basis[i],z);return 3;}fixedv[z]=2;}fprintf(stderr,"EXTRA_FIXED %d\n",x);}
 vector<int>wire(N);for(int x=0;x<N;x++)wire[x]=fixedv[x]==1?TRUE_W:(fixedv[x]==2?FALSE_W:x+1);
 f=fopen(argv[1],"w");if(!f){perror(argv[1]);return 2;}
 // Fixed-width header, rewritten after generation.
 fprintf(f,"p cnf %010d %011lld\n",0,0LL);
 long long lines=0;for(int a=0;a<N;a++)for(int b=a+1;b<N;b++){int c=third(a,b);if(c>b){lines++;cap_clause(wire[a],wire[b],wire[c]);}}
 fprintf(stderr,"CAP lines %lld emitted %lld\n",lines,capclauses);
 vector<int>sortw(PN,FALSE_W);for(int x=0;x<N;x++)sortw[x]=wire[x];
 for(int p=1;p<PN;p<<=1)for(int k=p;k>=1;k>>=1)for(int j=k%p;j+k<PN;j+=2*k)for(int i=0;i<k;i++)if((i+j)/(2*p)==(i+j+k)/(2*p))comparator(sortw[i+j],sortw[i+j+k],sortw[i+j],sortw[i+j+k]);
 int need=sortw[target-1];if(need==FALSE_W){fprintf(stderr,"cardinality impossible\n");return 5;}if(need!=TRUE_W)emit({need});
 long long finalclauses=clauses;int maxvar=nextvar-1;
 fseek(f,0,SEEK_SET);fprintf(f,"p cnf %010d %011lld\n",maxvar,finalclauses);fclose(f);
 fprintf(stderr,"DONE vars %d clauses %lld comparators %lld target %d\n",maxvar,finalclauses,comparators,target);return 0;
}
