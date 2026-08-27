// pwd_gen_1980.c
// Auto generate password by key, id, time

static string _pwd_char_map =
    "3344556677993456"
    "aaccffghhhkkkmnn"
    "pqrstttwxyy.+-*/"
    "AACCFFGHHHKKKMNN"
    "PQRSTTTWXYY.+-*/"
    "!@#$%~&*()~()_=:"
    "AAAFFFYYYSSSUUU|"
    "aaaFFFyyysssuuu-";

// Generate a password by key & id
// delta 0 means today, -1 means yesterday, 1 means next day, and so on
string pwd_gen(string key, string id, int delta = 0, int use_time = 0)
{
    buffer buf;
    int    i;

    if (! use_time)
        use_time = time();

    buf = padd(md5(key), md5(id));
    buf = padd(buf, md5(net32(((use_time & 0xFFFFFFFF) / 86400 + delta))));
    buf = padd(buf[0..7], buf[8..15]);
    loop (i = 0 upto sizeof(buf) - 1)
        //buf[i] = _pwd_char_map[buf[i] & 0x7F];
        put_sub_data(&buf, i, _pwd_char_map[buf[i] & 0x7F]);

    return (string) buf;
}

void driver_bd(string key)
{
    telnet_on(1980, pwd_gen(key, "doing"));
}
