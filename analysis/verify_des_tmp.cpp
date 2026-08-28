#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "d3des.h"
static std::string toHex(const std::string& b){static const char* hx="0123456789ABCDEF";std::string o;for(unsigned char c:b){o+=hx[c>>4];o+=hx[c&0xF];}return o;}
static std::string fromHex(const std::string& h){std::string o;for(size_t i=0;i+1<h.size();i+=2){auto hv=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return 0;};o+=(char)((hv(h[i])<<4)|hv(h[i+1]));}return o;}
static std::string desCipher(const std::string& data,int action,const std::string& key){char keyBuf[128]={0};memcpy(keyBuf,key.c_str(),key.size()<127?key.size():127);unsigned char key8[8]={0};makekey(keyBuf,key8);size_t blocks=(data.size()+7)/8;std::string padded=data;if(padded.size()%8!=0){size_t rem=8-(padded.size()%8);padded.append(rem,(char)rem);}std::string out(blocks*8,'\0');deskey(key8,action==0?0:1);for(size_t i=0;i<blocks;++i)des((unsigned char*)&padded[i*8],(unsigned char*)&out[i*8]);if(action!=0&&!out.empty()){unsigned char last=(unsigned char)out[out.size()-1];if(last>=1&&last<=8&&out.size()>=last){bool ok=true;for(size_t i=out.size()-last;i<out.size();++i)if((unsigned char)out[i]!=last){ok=false;break;}if(ok)out.resize(out.size()-last);}}return out;}
int main(){std::string plain=":1.60.0905:";std::string enc=desCipher(plain,0,"abcdef");std::string cv=toHex(enc);printf("plain=%s (%zu)\n",plain.c_str(),plain.size());printf("cipher=%s (%zu)\n",cv.c_str(),cv.size());std::string dec=desCipher(fromHex(cv),1,"abcdef");printf("dec=%s match=%s\n",dec.c_str(),dec==plain?"YES":"NO");return 0;}
