// Reproduce calc_account_checksum for account 'q' and compare with DB checksum.
string account = "q";
string password = "BFA1AF25984AEF0EAC96FC722AF11F91";
string privilege = "0";
string blocked_time = "";
string gold_coin = "2000";
string silver_coin = "2000";
string coin_password = "";
string unlock_coin_password_time = "";
string trade_lock_time = "";
string permit_ip = "";
string permit_id = "";
string str = account + password
    + sprintf("%08X", atoi(privilege))
    + blocked_time
    + sprintf("%08X", atoi(gold_coin))
    + sprintf("%08X", atoi(silver_coin))
    + coin_password + unlock_coin_password_time + trade_lock_time
    + permit_ip + permit_id + "ABCDEF";
string checksum = save_string(md5(str))[1..<2];
printf("str=%s\n", str);
printf("checksum=%s\n", checksum);
printf("db=A54225FEFD45BC3173ABB880721A456B match=%s\n", (checksum=="A54225FEFD45BC3173ABB880721A456B")?"YES":"NO");
