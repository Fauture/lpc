// Full version-check simulation with the REAL client version:
//   version = "20544DE38C2160D6EAC9BFE07486242E"  (from the real client capture)
//   decoded = (string)des(restore_value(":" + version + ":"), 1, "abcdef")
//   expect decoded == "1.60.0905"
string version = "20544DE38C2160D6EAC9BFE07486242E";
mixed restored = restore_value(":" + version + ":");
mixed decoded = des(restored, 1, "abcdef");
string sdec = (string)decoded;
printf("restoredType=%d\n", typeof(restored));
printf("decoded=[%s] len=%d\n", sdec, strlen(sdec));
printf("match=%s\n", (sdec == "1.60.0905") ? "YES" : "NO");
