#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "d3des.h"
static std::string fromHex(const std::string& h){std::string o;for(size_t i=0;i+1<h.size();i+=2){auto hv=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return 0;};o+=(char)((hv(h[i])<<4)|hv(h[i+1]));}return o;}
static std::string toHex(const std::string& b){static const char* hx="0123456789ABCDEF";std::string o;for(unsigned char c:b){o+=hx[c>>4];o+=hx[c&0xF];}return o;}
int main(){
  std::string ver="20544DE38C2160D6EAC9BFE07486242E";
  std::string ct=fromHex(ver);
  char kb[128]={0}; const char* kk="abcdef"; memcpy(kb,kk,strlen(kk)); unsigned char k1[8]={0}; makekey(kb,k1);
  std::string out(ct.size(),'\0');
  deskey(k1,1);
  for(size_t i=0;i<ct.size()/8;++i) des((unsigned char*)&ct[i*8],(unsigned char*)&out[i*8]);
  printf("decrypt hex: %s\n", toHex(out).c_str());
  printf("last byte: 0x%02X\n", (unsigned char)out[out.size()-1]);
  // try PKCS7 strip
  std::string stripped=out;
  unsigned char last=(unsigned char)stripped[stripped.size()-1];
  if(last>=1&&last<=8&&stripped.size()>=last){bool ok=true;for(size_t i=stripped.size()-last;i<stripped.size();++i)if((unsigned char)stripped[i]!=last){ok=false;break;}if(ok)stripped.resize(stripped.size()-last);}
  printf("stripped(%zu): [%s]\n", stripped.size(), stripped.c_str());
  printf("match 1.60.0905: %s\n", stripped=="1.60.0905"?"YES":"NO");
  return 0;
}
