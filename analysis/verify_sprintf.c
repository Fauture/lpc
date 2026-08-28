// Test sprintf %08X formatting (original driver passes through to snprintf).
string a = sprintf("%08X", 2000);
string b = sprintf("%08X", 0);
string c = sprintf("%d", 42);
string d = sprintf("%s-%s", "ab", "cd");
printf("a=[%s] expect 000007D0\n", a);
printf("b=[%s] expect 00000000\n", b);
printf("c=[%s] expect 42\n", c);
printf("d=[%s] expect ab-cd\n", d);
