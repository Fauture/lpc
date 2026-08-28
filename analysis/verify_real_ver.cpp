#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "d3des.h"
static std::string fromHex(const std::string& h){std::string o;for(size_t i=0;i+1<h.size();i+=2){auto hv=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return 0;};o+=(char)((hv(h[i])<<4)|hv(h[i+1]));}return o;}
static std::string desDecrypt(const std::string& data,int action,const unsigned char key8[8]){size_t blocks=data.size()/8;std::string out(data.size(),'\0');deskey((unsigned char*)key8,action==0?0:1);for(size_t i=0;i<blocks;++i)des((unsigned char*)&data[i*8],(unsigned char*)&out[i*8]);if(action!=0&&!out.empty()){unsigned char last=(unsigned char)out[out.size()-1];if(last>=1&&last<=8&&out.size()>=last){bool ok=true;for(size_t i=out.size()-last;i<out.size();++i)if((unsigned char)out[i]!=last){ok=false;break;}if(ok)out.resize(out.size()-last);}}return out;}
static std::string toHex(const std::string& b){static const char* hx="0123456789ABCDEF";std::string o;for(unsigned char c:b){o+=hx[c>>4];o+=hx[c&0xF];}return o;}
int main(){
  std::string ver="20544DE38C2160D6EAC9BFE07486242E";
  std::string ct=fromHex(ver);
  printf("cipher(%zu bytes): %s\n", ct.size(), toHex(ct).c_str());
  // makekey-hashed key
  char kb[128]={0}; const char* kk="abcdef"; memcpy(kb,kk,strlen(kk)); unsigned char k1[8]={0}; makekey(kb,k1);
  std::string d1=desDecrypt(ct,1,k1);
  printf("makekey decrypt: [%s] (%zu)\n", d1.c_str(), d1.size());
  // raw key
  unsigned char k2[8]={0}; memcpy(k2,kk,strlen(kk));
  std::string d2=desDecrypt(ct,1,k2);
  printf("rawkey decrypt : [%s] (%zu)\n", d2.c_str(), d2.size());
  return 0;
}
