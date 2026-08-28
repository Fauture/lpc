#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "d3des.h"
static std::string toHex(const std::string& b){static const char* hx="0123456789ABCDEF";std::string o;for(unsigned char c:b){o+=hx[c>>4];o+=hx[c&0xF];}return o;}
static std::string desCipher(const std::string& data,int action,const unsigned char key8[8]){size_t blocks=(data.size()+7)/8;std::string padded=data;if(padded.size()%8!=0){size_t rem=8-(padded.size()%8);padded.append(rem,(char)rem);}std::string out(blocks*8,'\0');deskey((unsigned char*)key8,action==0?0:1);for(size_t i=0;i<blocks;++i)des((unsigned char*)&padded[i*8],(unsigned char*)&out[i*8]);if(action!=0&&!out.empty()){unsigned char last=(unsigned char)out[out.size()-1];if(last>=1&&last<=8&&out.size()>=last){bool ok=true;for(size_t i=out.size()-last;i<out.size();++i)if((unsigned char)out[i]!=last){ok=false;break;}if(ok)out.resize(out.size()-last);}}return out;}
int main(){
  std::string md5v="6A40E1A1109E5D93636BF046E73861DE";
  std::string expected="21A76DE63D74BD47EB1211F382C08F9CF395854EF1FA2765A3E28479A393FDBA";
  // raw key: first 8 bytes zero-padded
  unsigned char k1[8]={0}; const char* kk="uWvgwf"; memcpy(k1,kk,strlen(kk));
  std::string enc1=desCipher(md5v,0,k1);
  printf("raw-key   : %s match=%s\n", toHex(enc1).c_str(), toHex(enc1)==expected?"YES":"NO");
  // makekey-hashed
  char kb[128]={0}; memcpy(kb,kk,strlen(kk)); unsigned char k2[8]={0}; makekey(kb,k2);
  std::string enc2=desCipher(md5v,0,k2);
  printf("makekey   : %s match=%s\n", toHex(enc2).c_str(), toHex(enc2)==expected?"YES":"NO");
  return 0;
}
