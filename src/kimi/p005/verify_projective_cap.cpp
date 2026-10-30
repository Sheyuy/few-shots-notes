#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
int main(int argc,char **argv){
 if(argc<4){std::cerr<<"usage: verify_projective_cap matrix.txt D P\n";return 2;}
 int D=std::stoi(argv[2]),P=std::stoi(argv[3]),Q=1;for(int r=0;r<D;r++)Q*=3;
 std::vector<std::uint8_t>M(D*P);std::ifstream in(argv[1]);
 for(int r=0;r<D;r++){std::string s;in>>s;if((int)s.size()!=P)return 2;for(int p=0;p<P;p++)M[r*P+p]=s[p]-'0';}
 std::vector<int> seen(Q,-1);int pow3[32],p=1;for(int r=0;r<D;r++){pow3[r]=p;p*=3;}
 for(int j=0;j<P;j++){
  int scale=1;for(int r=0;r<D;r++)if(M[r*P+j]){scale=M[r*P+j]==1?1:2;break;}
  int code=0;for(int r=0;r<D;r++)code+=((M[r*P+j]*scale)%3)*pow3[r];
  if(seen[code]>=0){std::cout<<"FAIL duplicate "<<seen[code]<<" "<<j<<"\n";return 1;}seen[code]=j;
 }
 for(int i=0;i<P;i++)for(int j=i+1;j<P;j++)for(int sign:{1,2}){
  int v[32],scale=1;for(int r=0;r<D;r++)v[r]=(M[r*P+i]+sign*M[r*P+j])%3;
  for(int r=0;r<D;r++)if(v[r]){scale=v[r]==1?1:2;break;}
  int code=0;for(int r=0;r<D;r++)code+=((v[r]*scale)%3)*pow3[r];
  if(seen[code]>=0){std::cout<<"FAIL triple "<<i<<" "<<j<<" "<<seen[code]<<" sign "<<sign<<"\n";return 1;}
 }
 std::cout<<"PASS projective cap D "<<D<<" P "<<P<<"\n";return 0;
}
