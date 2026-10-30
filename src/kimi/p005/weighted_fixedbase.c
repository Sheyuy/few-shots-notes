#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#define N 6561
#define MAXC 600
static uint8_t hi[N],lo[N],member[N],third4[81][81],allowed[N],protected_point[N];
static uint16_t negp[N];
static int conf[N],S[MAXC],bestS[MAXC],cur_dirs,bestdirs;
static int64_t wconf[N];
static uint32_t *lw;
static unsigned long long tabu[N];
static uint64_t rng_state;
static int dbias=1,freeremove=0,resetw=0,fixbase=0;
static inline uint64_t rng64(void){uint64_t z=(rng_state+=0x9e3779b97f4a7c15ULL);z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;z=(z^(z>>27))*0x94d049bb133111ebULL;return z^(z>>31);}
static inline int rnd(int n){return (int)(rng64()%(uint64_t)n);}
static inline double uni(void){return (rng64()>>11)*(1.0/9007199254740992.0);}
static inline int third(int a,int b){return third4[hi[a]][hi[b]]*81+third4[lo[a]][lo[b]];}
static inline uint32_t line_id(int a,int b,int c){int mn=a,mx=a;if(b<mn)mn=b;if(b>mx)mx=b;if(c<mn)mn=c;if(c>mx)mx=c;return (uint32_t)(mn*N+mx);}
static int count_bad(int n){int b=0;for(int i=0;i<n;i++)b+=conf[S[i]];return b/3;}
static int64_t count_badw(int n){int64_t b=0;for(int i=0;i<n;i++)b+=wconf[S[i]];return b/3;}
static inline int is_new_direction(int x){return !member[negp[x]];}
static inline int is_paired(int x){return negp[x]!=x&&member[negp[x]];}
static int count_dirs(int n){int d=0;for(int i=0;i<n;i++){int x=S[i];if(negp[x]==x||!member[negp[x]]||x<negp[x])d++;}return d;}
static void recompute(int n){
  memset(conf,0,sizeof(conf));memset(wconf,0,sizeof(wconf));
  for(int i=0;i<n;i++)for(int j=i+1;j<n;j++){int z=third(S[i],S[j]);uint32_t id=line_id(S[i],S[j],z);conf[z]++;wconf[z]+=lw[id];}
}
static void remove_at(int idx,int *np,int *bad,int64_t *badw){
  int n=*np,x=S[idx],old=conf[x];int64_t oldw=wconf[x];
  if(!is_paired(x))cur_dirs--;
  S[idx]=S[n-1];(*np)--;member[x]=0;
  for(int i=0;i<n-1;i++){int z=third(x,S[i]);uint32_t id=line_id(x,S[i],z);conf[z]--;wconf[z]-=lw[id];}
  *bad-=old;*badw-=oldw;
}
static void add_point(int x,int *np,int *bad,int64_t *badw){
  int n=*np,delta=conf[x];int64_t deltaw=wconf[x];
  if(is_new_direction(x))cur_dirs++;
  for(int i=0;i<n;i++){int z=third(x,S[i]);uint32_t id=line_id(x,S[i],z);conf[z]++;wconf[z]+=lw[id];}
  S[n]=x;member[x]=1;*np=n+1;*bad+=delta;*badw+=deltaw;
}
static int choose_add(int n,unsigned long long iter,int bad,int bestbad,double noise,int walk,int64_t alpha){
  int64_t minv=INT64_MAX;int choice=-1,ties=0;
  for(int x=0;x<N;x++)if(allowed[x]&&!member[x]){
    int nb=bad+conf[x];if(tabu[x]>iter&&nb>=bestbad)continue;
    int64_t score=(wconf[x]+alpha*conf[x])*8-dbias*is_new_direction(x);if(score<minv){minv=score;choice=x;ties=1;}else if(score==minv&&rnd(++ties)==0)choice=x;
  }
  if(choice<0){memset(tabu,0,sizeof(tabu));return choose_add(n,iter,bad,bestbad,noise,walk,alpha);}
  if(uni()<noise){int cnt=0;choice=-1;for(int x=0;x<N;x++)if(allowed[x]&&!member[x]&&(wconf[x]+alpha*conf[x])*8-dbias*is_new_direction(x)<=minv+(int64_t)walk*8&&(tabu[x]<=iter||bad+conf[x]<bestbad)){cnt++;if(rnd(cnt)==0)choice=x;}}
  return choice;
}
static void save_points(const char *path,const int *pts,int n,int bad,unsigned long long iter){FILE *f=fopen(path,"w");if(!f)return;fprintf(f,"# size %d bad %d iter %llu\n",n,bad,iter);for(int i=0;i<n;i++)fprintf(f,"%d\n",pts[i]);fclose(f);}
static void increase_weights(int n,int64_t *badw){
  for(int i=0;i<n;i++)for(int j=i+1;j<n;j++){
    int a=S[i],b=S[j],z=third(a,b);
    if(z>a&&z>b&&member[z]){uint32_t id=line_id(a,b,z);if(lw[id]<256){lw[id]++;wconf[a]++;wconf[b]++;wconf[z]++;(*badw)++;}}
  }
}
#ifdef DEBUG
static int check_state(int n,int bad,int64_t badw){
  int fc[N];int64_t fw[N];memset(fc,0,sizeof(fc));memset(fw,0,sizeof(fw));
  for(int i=0;i<n;i++)for(int j=i+1;j<n;j++){int z=third(S[i],S[j]);uint32_t id=line_id(S[i],S[j],z);fc[z]++;fw[z]+=lw[id];}
  for(int x=0;x<N;x++)if(fc[x]!=conf[x]||fw[x]!=wconf[x]){fprintf(stderr,"state mismatch at %d\n",x);return 0;}
  int b=0;int64_t bw=0;for(int i=0;i<n;i++){b+=fc[S[i]];bw+=fw[S[i]];}
  if(b/3!=bad||bw/3!=badw){fprintf(stderr,"cost mismatch %d %d %lld %lld\n",bad,b/3,(long long)badw,(long long)(bw/3));return 0;}return 1;
}
#endif
int main(int argc,char **argv){
  if(argc<11){fprintf(stderr,"usage: %s seed iterations target capfile prefix pid tenure noise walk weight_interval [stall] [alpha]\n",argv[0]);return 2;}
  uint64_t seed=strtoull(argv[1],0,0);unsigned long long iterations=strtoull(argv[2],0,0);int target=atoi(argv[3]);
  const char *capfile=argv[4],*prefix=argv[5];int pid=atoi(argv[6]),tenure=atoi(argv[7]);double noise=atof(argv[8]);int walk=atoi(argv[9]);unsigned long long wi=strtoull(argv[10],0,0);
  unsigned long long stall=argc>11?strtoull(argv[11],0,0):3000000;int64_t alpha=argc>12?atoll(argv[12]):0;rng_state=seed;if(getenv("DBIAS"))dbias=atoi(getenv("DBIAS"));if(getenv("FREEREMOVE"))freeremove=atoi(getenv("FREEREMOVE"));if(getenv("RESETW"))resetw=atoi(getenv("RESETW"));if(getenv("FIXBASE"))fixbase=atoi(getenv("FIXBASE"));char path[1024];
  lw=malloc((size_t)N*N*sizeof(uint32_t));if(!lw){fprintf(stderr,"weight allocation failed\n");return 2;}for(size_t i=0;i<(size_t)N*N;i++)lw[i]=1;
  for(int x=0;x<N;x++){hi[x]=x/81;lo[x]=x%81;int y=0,a=x,p=1;for(int i=0;i<8;i++){y+=(3-a%3)%3*p;a/=3;p*=3;}negp[x]=y;}
  for(int a=0;a<81;a++)for(int b=0;b<81;b++){int aa=a,bb=b,w=0,pl=1;for(int i=0;i<4;i++){w+=((3-(aa%3+bb%3)%3)%3)*pl;aa/=3;bb/=3;pl*=3;}third4[a][b]=w;}
  int base[MAXC],nb=0;FILE *f=fopen(capfile,"r");if(!f){perror("capfile");return 2;}char line[256];while(fgets(line,sizeof(line),f)){if(line[0]=='#'||line[0]=='\n')continue;base[nb++]=atoi(line);}fclose(f);memset(allowed,1,sizeof(allowed));
  if(argc>13){int nban=0;f=fopen(argv[13],"r");if(!f){perror(argv[13]);return 2;}while(fgets(line,sizeof(line),f)){if(line[0]=='#'||line[0]=='\n')continue;int x=atoi(line);if(x>=0&&x<N&&allowed[x]){allowed[x]=0;nban++;}}fclose(f);printf("pid %d banned %d file %s\n",pid,nban,argv[13]);fflush(stdout);}
  for(int i=0;i<nb;i++)if(!allowed[base[i]]){fprintf(stderr,"base contains banned point\n");return 2;}
  if(nb>target){fprintf(stderr,"base larger than target\n");return 2;}
  int n=0,bad=0,bestbad=1000000;int64_t badw=0;unsigned long long last_improve=0,restarts=0;clock_t t0=clock();
  for(unsigned long long iter=0;iter<iterations;iter++){
    if(iter==0||iter-last_improve>stall){
      memset(member,0,sizeof(member));memset(protected_point,0,sizeof(protected_point));memset(tabu,0,sizeof(tabu));if(resetw)for(size_t wj=0;wj<(size_t)N*N;wj++)lw[wj]=1;n=0;for(int i=0;i<nb;i++){S[n++]=base[i];member[base[i]]=1;if(fixbase)protected_point[base[i]]=1;}recompute(n);bad=count_bad(n);badw=count_badw(n);cur_dirs=count_dirs(n);
      while(n<target){int x=choose_add(n,iter,bad,bestbad,0.15,3,alpha);add_point(x,&n,&bad,&badw);}
      if(iter==0||bad<bestbad||(bad==bestbad&&cur_dirs>bestdirs)){bestbad=bad;bestdirs=cur_dirs;memcpy(bestS,S,n*sizeof(int));snprintf(path,sizeof(path),"%s-wclosest-%d.txt",prefix,pid);save_points(path,bestS,n,bad,iter);}
      last_improve=iter;restarts++;printf("pid %d restart %llu iter %llu bad %d bestbad %d\n",pid,restarts,iter,bad,bestbad);fflush(stdout);
      if(bad==0){snprintf(path,sizeof(path),"%s-wvalid-%d.txt",prefix,pid);save_points(path,S,n,0,iter);printf("pid %d BREAKTHROUGH size %d\n",pid,n);return 0;}
    }
    int idx=-1,cnt=0;
    if(freeremove&&rnd(1000)<freeremove){for(int tries=0;tries<1000;tries++){int q=rnd(n);if(!protected_point[S[q]]){idx=q;break;}}}
    else if(uni()<0.25){int64_t maxc=-1,ties=0;for(int i=0;i<n;i++)if(!protected_point[S[i]]&&wconf[S[i]]>0){int64_t score=wconf[S[i]]*8+dbias*is_paired(S[i]);if(score>maxc){maxc=score;idx=i;ties=1;}else if(score==maxc&&rnd(++ties)==0)idx=i;}}
    else{for(int i=0;i<n;i++)if(!protected_point[S[i]]&&wconf[S[i]]>0){cnt++;if(rnd(cnt)==0)idx=i;}}
    if(idx<0){snprintf(path,sizeof(path),"%s-wvalid-%d.txt",prefix,pid);save_points(path,S,n,0,iter);printf("pid %d VALID size %d iter %llu\n",pid,n,iter);return 0;}
    int removed=S[idx];remove_at(idx,&n,&bad,&badw);tabu[removed]=iter+tenure+rnd(tenure+1);
    int x=choose_add(n,iter,bad,bestbad,noise,walk,alpha);add_point(x,&n,&bad,&badw);
    if(bad<bestbad||(bad==bestbad&&cur_dirs>bestdirs)){bestbad=bad;bestdirs=cur_dirs;last_improve=iter;memcpy(bestS,S,n*sizeof(int));snprintf(path,sizeof(path),"%s-wclosest-%d.txt",prefix,pid);save_points(path,bestS,n,bad,iter);printf("pid %d iter %llu new_bestbad %d dirs %d restarts %llu elapsed %.3f\n",pid,iter,bestbad,cur_dirs,restarts,(double)(clock()-t0)/CLOCKS_PER_SEC);fflush(stdout);if(bad==0){snprintf(path,sizeof(path),"%s-wvalid-%d.txt",prefix,pid);save_points(path,S,n,0,iter);printf("pid %d BREAKTHROUGH size %d\n",pid,n);return 0;}}
    if(wi&&(iter+1)%wi==0)increase_weights(n,&badw);
#ifdef DEBUG
    if(iter%100==0&&!check_state(n,bad,badw))return 3;
#endif
    if((iter+1)%1000000==0){printf("pid %d progress %llu bestbad %d/%d currentbad %d/%d badw %lld restarts %llu elapsed %.3f\n",pid,iter+1,bestbad,bestdirs,bad,cur_dirs,(long long)badw,restarts,(double)(clock()-t0)/CLOCKS_PER_SEC);fflush(stdout);}
  }
  printf("pid %d DONE iterations %llu bestbad %d currentbad %d badw %lld restarts %llu\n",pid,iterations,bestbad,bad,(long long)badw,restarts);free(lw);return 1;
}
