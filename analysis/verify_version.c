// Verify the version-field round trip through the VM's des/save_string/
// restore_value efuns, exactly as account_login does.  Top-level code so
// `lpcvm run file.c` executes it.
string plain = ":1.60.0905:";
mixed enc = des(plain, 0, "abcdef");
string saved = save_string(enc);
string clientVersion = saved[1..<2];
printf("plain=%s\n", plain);
printf("encType=%d saved=%s\n", typeof(enc), saved);
printf("clientVersion=%s len=%d\n", clientVersion, strlen(clientVersion));
mixed restored = restore_value(":" + clientVersion + ":");
printf("restoredType=%d\n", typeof(restored));
mixed dec = des(restored, 1, "abcdef");
printf("decoded=%s match=%s\n", dec, (dec == plain) ? "YES" : "NO");
