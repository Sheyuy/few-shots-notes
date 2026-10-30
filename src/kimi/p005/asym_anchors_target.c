#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define N 6561
#define MAXC 600
static uint8_t hi[N],lo[N],member[N],protected_point[N],forbidden[N],covermark[N],allowed[N];
static uint8_t third4[81][81];
static uint16_t negp[N];
static int conf[N],S[MAXC],bestS[MAXC],anchors[MAXC],anchor_pos[N];
static unsigned long long tabu[N];
static uint64_t rng_state;
static int dbias=1;

static inline uint64_t rng64(void){uint64_t z=(rng_state+=0x9e3779b97f4a7c15ULL);z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;z=(z^(z>>27))*0x94d049bb133111ebULL;return z^(z>>31);}
static inline int rnd(int n){return (int)(rng64()%(uint64_t)n);}
static inline double uni(void){return (rng64()>>11)*(1.0/9007199254740992.0);}
static inline int third(int a,int b){return third4[hi[a]][hi[b]]*81+third4[lo[a]][lo[b]];}
static inline int is_new_direction(int x){return !member[negp[x]];}
static inline int is_paired(int x){return negp[x]!=x&&member[negp[x]];}

static int count_bad(int n){int b=0;for(int i=0;i<n;i++)b+=conf[S[i]];return b/3;}
static int count_dirs(int n){int d=0;for(int i=0;i<n;i++){int x=S[i];if(negp[x]==x||!member[negp[x]]||x<negp[x])d++;}return d;}
static void recompute(int n){memset(conf,0,sizeof(conf));for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)conf[third(S[i],S[j])]++;}

static int remove_at(int idx,int *np,int bad){
  int n=*np,x=S[idx],old=conf[x];
  S[idx]=S[n-1];(*np)--;member[x]=0;
  for(int i=0;i<n-1;i++)conf[third(x,S[i])]--;
  return bad-old;
}
static int add_point(int x,int *np,int bad){
  int n=*np,delta=conf[x];
  for(int i=0;i<n;i++)conf[third(x,S[i])]++;
  S[n]=x;member[x]=1;*np=n+1;return bad+delta;
}

static int choose_add(unsigned long long iter,int bad,int bestbad,double noise,int walk){
  int64_t minv=INT64_MAX;int choice=-1,ties=0;
  for(int x=0;x<N;x++)if(allowed[x]&&!member[x]){
    int nb=bad+conf[x];if(tabu[x]>iter&&nb>=bestbad)continue;
    int64_t score=(int64_t)conf[x]*8-dbias*is_new_direction(x);
    if(score<minv){minv=score;choice=x;ties=1;}
    else if(score==minv&&rnd(++ties)==0)choice=x;
  }
  if(choice<0){memset(tabu,0,sizeof(tabu));return choose_add(iter,bad,bestbad,noise,walk);}
  if(uni()<noise){
    int cnt=0;choice=-1;int64_t lim=minv+(int64_t)walk*8;
    for(int x=0;x<N;x++)if(allowed[x]&&!member[x]&&(tabu[x]<=iter||bad+conf[x]<bestbad)){
      int64_t score=(int64_t)conf[x]*8-dbias*is_new_direction(x);
      if(score<=lim){cnt++;if(rnd(cnt)==0)choice=x;}
    }
  }
  return choice;
}

static int choose_anchor(int na,int walk){
  int minv=INT_MAX,cnt=0,choice=-1;
  memset(forbidden,0,sizeof(forbidden));
  for(int i=0;i<na;i++)for(int j=i+1;j<na;j++)forbidden[third(anchors[i],anchors[j])]=1;
  for(int x=0;x<N;x++)if(allowed[x]&&!member[x]&&!forbidden[x]&&is_new_direction(x)&&conf[x]<minv)minv=conf[x];
  if(minv==INT_MAX)return -1;
  int lim=minv+walk;
  for(int x=0;x<N;x++)if(allowed[x]&&!member[x]&&!forbidden[x]&&is_new_direction(x)&&conf[x]<=lim){cnt++;if(rnd(cnt)==0)choice=x;}
  return choice;
}

static int choose_conflict_point(int n){
  int idx=-1,cnt=0;
  if(uni()<0.30){
    int maxscore=INT_MIN,ties=0;
    for(int i=0;i<n;i++)if(!protected_point[S[i]]&&conf[S[i]]>0){
      int score=conf[S[i]]*4+dbias*is_paired(S[i]);
      if(score>maxscore){maxscore=score;idx=i;ties=1;}
      else if(score==maxscore&&rnd(++ties)==0)idx=i;
    }
  }else{
    for(int i=0;i<n;i++)if(!protected_point[S[i]]&&conf[S[i]]>0){cnt++;if(rnd(cnt)==0)idx=i;}
  }
  return idx;
}

static int remove_initial_excess(int *np,int bad,int target){
  int n=*np;
  while(n>target){
    int idx=-1,maxscore=INT_MIN,ties=0;
    for(int i=0;i<n;i++)if(!protected_point[S[i]]){
      int score=conf[S[i]]*4+dbias*is_paired(S[i]);
      if(score>maxscore){maxscore=score;idx=i;ties=1;}
      else if(score==maxscore&&rnd(++ties)==0)idx=i;
    }
    if(idx<0)return -1;
    bad=remove_at(idx,&n,bad);
  }
  *np=n;return bad;
}

static void save_points(const char *path,const int *pts,int n,int bad,int dirs,unsigned long long iter){
  FILE *f=fopen(path,"w");if(!f)return;
  fprintf(f,"# size %d bad %d dirs %d iter %llu\n",n,bad,dirs,iter);
  for(int i=0;i<n;i++)fprintf(f,"%d\n",pts[i]);
  fclose(f);
}

#define MAXL 4096
static int conflict_cover(int n){
  static int la[MAXL],lb[MAXL],lc[MAXL],deg[N];static uint8_t active[MAXL];
  int nl=0;memset(covermark,0,sizeof(covermark));
  for(int i=0;i<n;i++)for(int j=i+1;j<n;j++){
    int a=S[i],b=S[j],z=third(a,b);
    if(z>a&&z>b&&member[z]){
      if(nl>=MAXL)return MAXL;
      la[nl]=a;lb[nl]=b;lc[nl]=z;active[nl]=1;nl++;
    }
  }
  int left=nl,cover=0;
  while(left){
    memset(deg,0,sizeof(deg));
    for(int l=0;l<nl;l++)if(active[l]){deg[la[l]]++;deg[lb[l]]++;deg[lc[l]]++;}
    int x=-1,maxdeg=0;
    for(int i=0;i<n;i++){int y=S[i];if(deg[y]>maxdeg){maxdeg=deg[y];x=y;}}
    if(x<0)break;covermark[x]=1;cover++;
    for(int l=0;l<nl;l++)if(active[l]&&(la[l]==x||lb[l]==x||lc[l]==x)){active[l]=0;left--;}
  }
  return cover;
}

int main(int argc,char **argv){
  if(argc<12){fprintf(stderr,"usage: %s seed iterations capfile prefix pid anchors protect tenure noise walk stall [poolfile]\n",argv[0]);return 2;}
  uint64_t seed=strtoull(argv[1],0,0);unsigned long long iterations=strtoull(argv[2],0,0);
  const char *capfile=argv[3],*prefix=argv[4];int pid=atoi(argv[5]),nanchors=atoi(argv[6]);
  unsigned long long protect=strtoull(argv[7],0,0);int tenure=atoi(argv[8]);double noise=atof(argv[9]);
  int walk=atoi(argv[10]);unsigned long long stall=strtoull(argv[11],0,0);
  int target=513;if(getenv("TARGET"))target=atoi(getenv("TARGET"));rng_state=seed;if(getenv("DBIAS"))dbias=atoi(getenv("DBIAS"));char path[1024];
  for(int x=0;x<N;x++){
    hi[x]=x/81;lo[x]=x%81;anchor_pos[x]=-1;
    int y=0,a=x,p=1;for(int i=0;i<8;i++){y+=(3-a%3)%3*p;a/=3;p*=3;}negp[x]=y;
  }
  for(int a=0;a<81;a++)for(int b=0;b<81;b++){
    int aa=a,bb=b,w=0,p=1;for(int i=0;i<4;i++){w+=((3-(aa%3+bb%3)%3)%3)*p;aa/=3;bb/=3;p*=3;}third4[a][b]=w;
  }
  int base[MAXC],nb=0;FILE *f=fopen(capfile,"r");if(!f){perror(capfile);return 2;}char line[256];
  while(fgets(line,sizeof(line),f)){if(line[0]=='#'||line[0]=='\n')continue;base[nb++]=atoi(line);}fclose(f);
  memset(allowed,1,sizeof(allowed));
  if(argc>12){
    memset(allowed,0,sizeof(allowed));int np=0;f=fopen(argv[12],"r");if(!f){perror(argv[12]);return 2;}
    while(fgets(line,sizeof(line),f)){if(line[0]=='#'||line[0]=='\n')continue;int x=atoi(line);if(x>=0&&x<N&&!allowed[x]){allowed[x]=1;np++;}}fclose(f);
    printf("pid %d pool %d file %s\n",pid,np,argv[12]);fflush(stdout);
    for(int i=0;i<nb;i++)if(!allowed[base[i]]){fprintf(stderr,"base point outside pool\n");return 2;}
  }
  if(nb>target||nanchors<1||nb+nanchors>=MAXC){fprintf(stderr,"invalid base/anchor sizes %d %d\n",nb,nanchors);return 2;}

  int n=0,bad=0,bestbad=INT_MAX,bestdirs=0,bestcover=INT_MAX,coverdirs=0,na=0;
  int highdirs=0,highbad=INT_MAX,highS[MAXC];
  unsigned long long restarts=0,start_iter=0,accepted=0;clock_t t0=clock();
  int restart_best=INT_MAX,restart_dirs=0,restartS[MAXC];
  for(unsigned long long iter=0;iter<iterations;iter++){
    if(iter==0||iter-start_iter>=stall){
      if(restarts&&restart_best<INT_MAX&&restart_best<=12&&restart_dirs>=258){
        snprintf(path,sizeof(path),"%s-r%llu-b%d-d%d-p%d.txt",prefix,restarts-1,restart_best,restart_dirs,pid);
        save_points(path,restartS,target,restart_best,restart_dirs,iter-1);
      }
      memset(member,0,sizeof(member));memset(protected_point,0,sizeof(protected_point));memset(tabu,0,sizeof(tabu));
      for(int x=0;x<N;x++)anchor_pos[x]=-1;
      n=0;na=0;bad=0;start_iter=iter;restart_best=INT_MAX;restart_dirs=0;
      for(int i=0;i<nb;i++){S[n++]=base[i];member[base[i]]=1;}recompute(n);bad=count_bad(n);
      while(na<nanchors){
        int x=choose_anchor(na,walk+3);if(x<0){fprintf(stderr,"no anchor available\n");return 3;}
        bad=add_point(x,&n,bad);protected_point[x]=1;anchor_pos[x]=na;anchors[na++]=x;
      }
      while(n<target){int x=choose_add(iter,bad,bestbad,0.12,walk);bad=add_point(x,&n,bad);}
      bad=remove_initial_excess(&n,bad,target);if(bad<0){fprintf(stderr,"protected excess\n");return 3;}
      restarts++;printf("pid %d restart %llu iter %llu anchors %d initial_bad %d dirs %d best %d/%d elapsed %.2f\n",pid,restarts,iter,na,bad,count_dirs(n),bestbad,bestdirs,(double)(clock()-t0)/CLOCKS_PER_SEC);fflush(stdout);
    }
    if(protect){
      unsigned long long elapsed=iter-start_iter;
      int released=(int)((elapsed*(unsigned long long)na)/protect);
      if(released>na)released=na;
      for(int i=0;i<released;i++)protected_point[anchors[i]]=0;
    }else for(int i=0;i<na;i++)protected_point[anchors[i]]=0;

    int idx=choose_conflict_point(n);
    if(idx<0){
      if(bad==0){
        int dirs=count_dirs(n);snprintf(path,sizeof(path),"%s-valid-%d.txt",prefix,pid);save_points(path,S,n,0,dirs,iter);
        printf("pid %d VALID size %d dirs %d iter %llu\n",pid,n,dirs,iter);fflush(stdout);return 0;
      }
      for(int i=0;i<na;i++)protected_point[anchors[i]]=0;
      idx=choose_conflict_point(n);if(idx<0){start_iter=iter;continue;}
    }
    int removed=S[idx];bad=remove_at(idx,&n,bad);tabu[removed]=iter+tenure+rnd(tenure+1);
    int x=choose_add(iter,bad,bestbad,noise,walk);bad=add_point(x,&n,bad);accepted++;
    if(accepted%2048==0&&bad<=120){int hd=count_dirs(n);if(hd>highdirs){highdirs=hd;highbad=bad;memcpy(highS,S,n*sizeof(int));snprintf(path,sizeof(path),"%s-high-%d.txt",prefix,pid);save_points(path,highS,n,highbad,highdirs,iter);printf("pid %d iter %llu HIGH_DIRS bad %d dirs %d elapsed %.2f\n",pid,iter,highbad,highdirs,(double)(clock()-t0)/CLOCKS_PER_SEC);fflush(stdout);}}
    int dirs=-1;
    if(bad<=bestbad||bad<=restart_best){dirs=count_dirs(n);}
    if(bad<restart_best||(bad==restart_best&&dirs>restart_dirs)){restart_best=bad;restart_dirs=dirs;memcpy(restartS,S,n*sizeof(int));}
    int new_bad=0;
    if(bad<bestbad||(bad==bestbad&&dirs>bestdirs)){
      int old=bestbad;new_bad=bad<bestbad;bestbad=bad;bestdirs=dirs;memcpy(bestS,S,n*sizeof(int));
      snprintf(path,sizeof(path),"%s-closest-%d.txt",prefix,pid);save_points(path,bestS,n,bad,dirs,iter);
      printf("pid %d iter %llu NEW_BEST bad %d dirs %d previous %d restart %llu elapsed %.2f\n",pid,iter,bad,dirs,old,restarts,(double)(clock()-t0)/CLOCKS_PER_SEC);fflush(stdout);
      if(bad==0){snprintf(path,sizeof(path),"%s-valid-%d.txt",prefix,pid);save_points(path,S,n,0,dirs,iter);return 0;}
    }
    if(bad<=60&&(new_bad||accepted%8192==0)){
      int cd=count_dirs(n),cover=conflict_cover(n);
      if(cover<bestcover||(cover==bestcover&&cd>coverdirs)){
        static uint8_t sm[N];int seedS[MAXC],sn=0,sd=0;
        bestcover=cover;coverdirs=cd;
        snprintf(path,sizeof(path),"%s-cover-%d.txt",prefix,pid);save_points(path,S,n,bad,cd,iter);
        for(int i=0;i<n;i++)if(!covermark[S[i]])seedS[sn++]=S[i];
        memset(sm,0,sizeof(sm));for(int i=0;i<sn;i++)sm[seedS[i]]=1;
        for(int i=0;i<sn;i++){int x=seedS[i];if(negp[x]==x||!sm[negp[x]]||x<negp[x])sd++;}
        snprintf(path,sizeof(path),"%s-cover-seed-%d.txt",prefix,pid);save_points(path,seedS,sn,0,sd,iter);
        printf("pid %d iter %llu NEW_COVER cover %d bad %d dirs %d seed %d/%d elapsed %.2f\n",pid,iter,cover,bad,cd,sn,sd,(double)(clock()-t0)/CLOCKS_PER_SEC);fflush(stdout);
      }
    }
    if((iter+1)%1000000==0){printf("pid %d progress %llu best %d/%d cover %d/%d current %d/%d restart %llu accepted %llu elapsed %.2f\n",pid,iter+1,bestbad,bestdirs,bestcover,coverdirs,bad,count_dirs(n),restarts,accepted,(double)(clock()-t0)/CLOCKS_PER_SEC);fflush(stdout);}
  }
  printf("pid %d DONE best %d/%d high %d/%d restarts %llu accepted %llu\n",pid,bestbad,bestdirs,highbad,highdirs,restarts,accepted);return 1;
}
