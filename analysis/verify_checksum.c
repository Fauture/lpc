// Verify the antibot checksum with the RAW-key des:
//   new_checksum = save_string(des(md5_value, 0, "uWvgwf"))[1..<2]
// md5_value = "6A40E1A1109E5D93636BF046E73861DE" (32 ASCII chars)
// expected client checksum = "21A76DE63D74BD47EB1211F382C08F9CF395854EF1FA2765A3E28479A393FDBA"
string md5_value = "6A40E1A1109E5D93636BF046E73861DE";
string expected = "21A76DE63D74BD47EB1211F382C08F9CF395854EF1FA2765A3E28479A393FDBA";
mixed enc = des(md5_value, 0, "uWvgwf");
string saved = save_string(enc);
string new_checksum = saved[1..<2];
printf("encType=%d saved=%s\n", typeof(enc), saved);
printf("new_checksum=%s\n", new_checksum);
printf("match=%s\n", (new_checksum == expected) ? "YES" : "NO");
