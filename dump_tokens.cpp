#include <iostream>
#include "src/lexer.h"
int main(){ std::string line = "lib_pak_path = lib_pak_path[1..<2];"; lpc::Lexer lx(line); auto ts=lx.tokenize(); for(auto&t:ts){ std::cout << (int)t.kind << ":" << t.text << " "; } }
