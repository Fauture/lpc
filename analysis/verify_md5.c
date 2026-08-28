// Verify md5 efun with RFC 1321 test vectors via save_string.
string t1 = save_string(md5(""))[1..<2];
string t2 = save_string(md5("abc"))[1..<2];
string t3 = save_string(md5("message digest"))[1..<2];
string t4 = save_string(md5("abcdefghijklmnopqrstuvwxyz"))[1..<2];
printf("md5('')       = %s expect D41D8CD98F00B204E9800998ECF8427E %s\n", t1, (t1=="D41D8CD98F00B204E9800998ECF8427E")?"OK":"FAIL");
printf("md5('abc')    = %s expect 900150983CD24FB0D6963F7D28E17F72 %s\n", t2, (t2=="900150983CD24FB0D6963F7D28E17F72")?"OK":"FAIL");
printf("md5('msg...') = %s expect F96B697D7CB7938D525A2F31AAF161D0 %s\n", t3, (t3=="F96B697D7CB7938D525A2F31AAF161D0")?"OK":"FAIL");
printf("md5('a-z')    = %s expect C3FCD3D76192E4007DFB496CCA67E13B %s\n", t4, (t4=="C3FCD3D76192E4007DFB496CCA67E13B")?"OK":"FAIL");
