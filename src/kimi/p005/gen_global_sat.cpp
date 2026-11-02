#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <vector>
using namespace std;
static const int N=6561, PN=8192, TRUE_W=INT_MAX, FALSE_W=0;
static uint8_t hi[N],lo[N],third4[81][81],fixedv[N];
static inline int third(int a,int b){return third4[hi[a]][hi[b]]*81+third4[lo[a]][lo[b]];}
static FILE *f;static int nextvar=N+1;static long long clauses=0,capclauses=0,planeclauses=0,flat3clauses=0,flat4clauses=0,comparators=0;
static void emit(initializer_list<int> ls){for(int x:ls)fprintf(f,"%d ",x);fputs("0\n",f);clauses++;}
static void cap_clause(int a,int b,int c){
 int ws[3]={a,b,c},lits[3],n=0;
 for(int w:ws){if(w==FALSE_W)return;if(w==TRUE_W)continue;lits[n++]=-w;}
 if(n==0){fprintf(stderr,"empty cap clause\n");exit(4);}
 for(int i=0;i<n;i++)fprintf(f,"%d ",lits[i]);fputs("0\n",f);clauses++;capclauses++;
}
static void at_most(const vector<int>&ws,int bound){
 vector<int>lits;int fixed_true=0;
 for(int w:ws){if(w==FALSE_W)continue;if(w==TRUE_W){fixed_true++;continue;}lits.push_back(w);}
 int b=bound-fixed_true;if(b<0){fprintf(stderr,"constant cardinality conflict\n");exit(4);}if((int)lits.size()<=b)return;
 int need=b+1;vector<int>pick;
 function<void(int)>rec=[&](int start){
  if((int)pick.size()==need){for(int i:pick)fprintf(f,"%d ",-lits[i]);fputs("0\n",f);clauses++;planeclauses++;return;}
  for(int i=start;i<=(int)lits.size()-(need-(int)pick.size());i++){pick.push_back(i);rec(i+1);pick.pop_back();}
 };
 rec(0);
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
static void at_most_network(const vector<int>&ws,int bound,long long &counter){
 long long before=clauses;vector<int>lits;int fixed_true=0;
 for(int w:ws){if(w==FALSE_W)continue;if(w==TRUE_W){fixed_true++;continue;}lits.push_back(w);}
 int b=bound-fixed_true;if(b<0){fprintf(stderr,"constant flat3 cardinality conflict\n");exit(4);}if((int)lits.size()<=b)return;
 int pn=1;while(pn<(int)lits.size())pn<<=1;vector<int>sortw(pn,FALSE_W);copy(lits.begin(),lits.end(),sortw.begin());
 for(int p=1;p<pn;p<<=1)for(int k=p;k>=1;k>>=1)for(int j=k%p;j+k<pn;j+=2*k)for(int i=0;i<k;i++)if((i+j)/(2*p)==(i+j+k)/(2*p))comparator(sortw[i+j],sortw[i+j+k],sortw[i+j],sortw[i+j+k]);
 if(sortw[b]==TRUE_W){fprintf(stderr,"constant flat3 network conflict\n");exit(4);}if(sortw[b]!=FALSE_W)emit({-sortw[b]});
 counter+=clauses-before;
}
int main(int argc,char**argv){
 if(argc<2){fprintf(stderr,"usage: gen_global_sat out.cnf [target] [extra_fixed_point|-1] [coordinate_planes] [forbid_support2] [forbid_support3] [fixed_unit_clauses] [coordinate_flats3_max_free] [coordinate_flats4_max_free]\n");return 2;}int target=argc>2?atoi(argv[2]):513;
 for(int x=0;x<N;x++){hi[x]=x/81;lo[x]=x%81;}for(int a=0;a<81;a++)for(int b=0;b<81;b++){int aa=a,bb=b,w=0,p=1;for(int i=0;i<4;i++){w+=((3-(aa%3+bb%3)%3)%3)*p;aa/=3;bb/=3;p*=3;}third4[a][b]=w;}
 int basis[9]={0,2187,729,243,81,27,9,3,1};for(int x:basis)fixedv[x]=1;
 for(int i=0;i<9;i++)for(int j=i+1;j<9;j++){int z=third(basis[i],basis[j]);if(!fixedv[z])fixedv[z]=2;else if(fixedv[z]!=2){fprintf(stderr,"basis conflict\n");return 3;}}
 if(argc>3&&atoi(argv[3])>=0){int x=atoi(argv[3]);if(x>=N||fixedv[x]){fprintf(stderr,"invalid extra fixed point %d state %d\n",x,x<N?fixedv[x]:-1);return 3;}fixedv[x]=1;for(int i=0;i<9;i++){int z=third(x,basis[i]);if(fixedv[z]==1){fprintf(stderr,"extra point conflict %d %d %d\n",x,basis[i],z);return 3;}fixedv[z]=2;}fprintf(stderr,"EXTRA_FIXED %d\n",x);}
 if(argc>5&&atoi(argv[5])){int total=0,newly=0;for(int x=0;x<N;x++){int y=x,w=0;for(int i=0;i<8;i++){w+=y%3!=0;y/=3;}if(w==2){total++;if(fixedv[x]==1){fprintf(stderr,"support2 fixed true conflict %d\n",x);return 3;}if(!fixedv[x])newly++;fixedv[x]=2;}}fprintf(stderr,"FORBID_SUPPORT2 total %d newly %d\n",total,newly);}
 if(argc>6&&atoi(argv[6])){int total=0,newly=0;for(int x=0;x<N;x++){int y=x,w=0;for(int i=0;i<8;i++){w+=y%3!=0;y/=3;}if(w==3){total++;if(fixedv[x]==1){fprintf(stderr,"support3 fixed true conflict %d\n",x);return 3;}if(!fixedv[x])newly++;fixedv[x]=2;}}fprintf(stderr,"FORBID_SUPPORT3 total %d newly %d\n",total,newly);}
 vector<int>wire(N);for(int x=0;x<N;x++)wire[x]=fixedv[x]==1?TRUE_W:(fixedv[x]==2?FALSE_W:x+1);
 f=fopen(argv[1],"w");if(!f){perror(argv[1]);return 2;}
 // Fixed-width header, rewritten after generation.
 fprintf(f,"p cnf %010d %011lld\n",0,0LL);
 long long lines=0;for(int a=0;a<N;a++)for(int b=a+1;b<N;b++){int c=third(a,b);if(c>b){lines++;cap_clause(wire[a],wire[b],wire[c]);}}
 fprintf(stderr,"CAP lines %lld emitted %lld\n",lines,capclauses);
 if(argc>4&&atoi(argv[4])){
  int pow3[8]={2187,729,243,81,27,9,3,1};long long forms=0;
  for(int i=0;i<8;i++)for(int j=i+1;j<8;j++)for(int mask=0;mask<729;mask++){
   int d[8]={0},t=mask;for(int k=0;k<8;k++)if(k!=i&&k!=j){d[k]=t%3;t/=3;}
   vector<int>ws;for(int u=0;u<3;u++)for(int v=0;v<3;v++){d[i]=u;d[j]=v;int x=0;for(int k=0;k<8;k++)x+=d[k]*pow3[k];ws.push_back(wire[x]);}
   at_most(ws,4);forms++;
  }
  fprintf(stderr,"PLANES forms %lld clauses %lld\n",forms,planeclauses);
 }
 if(argc>8&&atoi(argv[8])>=0){
  int max_free=atoi(argv[8]);int pow3[8]={2187,729,243,81,27,9,3,1};long long forms=0,selected=0;
  for(int i=0;i<8;i++)for(int j=i+1;j<8;j++)for(int k=j+1;k<8;k++){
   int others[5],no=0;for(int c=0;c<8;c++)if(c!=i&&c!=j&&c!=k)others[no++]=c;
   for(int mask=0;mask<243;mask++){
    int d[8]={0},t=mask;for(int c:others){d[c]=t%3;t/=3;}
    vector<int>ws;for(int u=0;u<3;u++)for(int v=0;v<3;v++)for(int w=0;w<3;w++){
     d[i]=u;d[j]=v;d[k]=w;int x=0;for(int c=0;c<8;c++)x+=d[c]*pow3[c];ws.push_back(wire[x]);
    }
    int free_points=count_if(ws.begin(),ws.end(),[](int w){return w!=TRUE_W&&w!=FALSE_W;});
    int fixed_true=count(ws.begin(),ws.end(),TRUE_W);
    if(free_points<=max_free&&free_points>9-fixed_true){at_most_network(ws,9,flat3clauses);selected++;}
    forms++;
   }
  }
  if(forms!=13608){fprintf(stderr,"bad global flat3 form count %lld\n",forms);return 5;}
  fprintf(stderr,"FLATS3 forms %lld selected %lld max_free %d clauses %lld\n",forms,selected,max_free,flat3clauses);
 }
 if(argc>9&&atoi(argv[9])>=0){
  int max_free=atoi(argv[9]);int pow3[8]={2187,729,243,81,27,9,3,1};long long forms=0,selected=0;
  for(int i=0;i<8;i++)for(int j=i+1;j<8;j++)for(int k=j+1;k<8;k++)for(int l=k+1;l<8;l++){
   int others[4],no=0;for(int c=0;c<8;c++)if(c!=i&&c!=j&&c!=k&&c!=l)others[no++]=c;
   for(int mask=0;mask<81;mask++){
    int d[8]={0},t=mask;for(int c:others){d[c]=t%3;t/=3;}
    vector<int>ws;for(int u=0;u<3;u++)for(int v=0;v<3;v++)for(int w=0;w<3;w++)for(int z=0;z<3;z++){
     d[i]=u;d[j]=v;d[k]=w;d[l]=z;int x=0;for(int c=0;c<8;c++)x+=d[c]*pow3[c];ws.push_back(wire[x]);
    }
    int free_points=count_if(ws.begin(),ws.end(),[](int w){return w!=TRUE_W&&w!=FALSE_W;});
    int fixed_true=count(ws.begin(),ws.end(),TRUE_W);
    if(free_points<=max_free&&free_points>20-fixed_true){at_most_network(ws,20,flat4clauses);selected++;}
    forms++;
   }
  }
  if(forms!=5670){fprintf(stderr,"bad global flat4 form count %lld\n",forms);return 5;}
  fprintf(stderr,"FLATS4 forms %lld selected %lld max_free %d clauses %lld\n",forms,selected,max_free,flat4clauses);
 }
 if(argc>7&&atoi(argv[7])){int fixed_true_units=0,fixed_false_units=0;for(int x=0;x<N;x++)if(fixedv[x]){emit({fixedv[x]==1?x+1:-(x+1)});if(fixedv[x]==1)fixed_true_units++;else fixed_false_units++;}fprintf(stderr,"FIXED_UNITS true %d false %d\n",fixed_true_units,fixed_false_units);}
 vector<int>sortw(PN,FALSE_W);for(int x=0;x<N;x++)sortw[x]=wire[x];
 for(int p=1;p<PN;p<<=1)for(int k=p;k>=1;k>>=1)for(int j=k%p;j+k<PN;j+=2*k)for(int i=0;i<k;i++)if((i+j)/(2*p)==(i+j+k)/(2*p))comparator(sortw[i+j],sortw[i+j+k],sortw[i+j],sortw[i+j+k]);
 int need=sortw[target-1];if(need==FALSE_W){fprintf(stderr,"cardinality impossible\n");return 5;}if(need!=TRUE_W)emit({need});
 long long finalclauses=clauses;int maxvar=nextvar-1;
 fseek(f,0,SEEK_SET);fprintf(f,"p cnf %010d %011lld\n",maxvar,finalclauses);fclose(f);
 fprintf(stderr,"DONE vars %d clauses %lld comparators %lld target %d\n",maxvar,finalclauses,comparators,target);return 0;
}
