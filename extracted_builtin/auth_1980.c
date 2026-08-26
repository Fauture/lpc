// auth.c
// This file would be put into driver as a built-in file

#pragma include:/v

#include "pwd_gen_1980.c"

// Command no
#define CMD_AUTHENTICATE            0
#define CMD_REQUEST_PASSWORD        1
#define CMD_LOG_TO_DAS              2

// Request mode
#define TYPE_UDP    11
#define TYPE_TCP    22

// Telnet configuration
static mapping telnet_settings = (const mapping) ([ ]);

// Get the telnet configuration
public const mapping get_telnet_settings() { return telnet_settings; }

// Internal routines
private mixed send_request(int mode, int ip, int port, int cmd, int send_time, int seed, int para, buffer ex_para, int wait_responding);
private int   check_authenticate(buffer ack, int send_time, int seed);
private mixed check_request_password(buffer ack, int send_time, int seed);
private int   get_auth_addr(mixed &ip, mixed &port);

// Request format
/*
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   | Ver |c.|00| send_time |    seed   |    para   |
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   |  ex_para...                                   |
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   |  ...                                          |
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   |  ...                                          |
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
private mixed send_request(int mode, int ip, int port,
                           int cmd, int send_time, int seed, int para = 0,
                           buffer ex_para = allocate_buffer(0),
                           int wait_responding = 1)
{
    buffer buf, req;
    int    fd;
    int    i;

    // Send request to destination

    if (mode == TYPE_UDP)
    {
        // Via UDP datagram
        fd = socket(AF_INET, SOCK_DGRAM);
    } else
    {
        // Via TCP connection
        fd = socket(AF_INET, SOCK_STREAM);
        if (connect(fd, ip, port) != OK)
        {
            printf("Failed to connect to %s:%d.\n", inettoa(ip), port);
            return;
        }
    }

    for (i = 0; i < 8; i++)
    {
        string peer_ip;
        int    peer_port;

        // Build request
        req = net16(1980) + net8(cmd, 0) + net32(send_time, seed, para) +
              ex_para;
        if (sizeof(ex_para) < 48)
            req += allocate_buffer(48 - sizeof(ex_para));

        // Encrypt request
        req = des(req, DES_EN0, "doing");

        // Send request to server
        if (mode == TYPE_UDP)
            // Via UDP
            send_to(fd, req, ip, port);
        else
            // Via TCP
            send(fd, req);

        if (! wait_responding)
        {
            // Do not need responding
            buf = allocate_buffer(0);
            break;
        }

        // Wait for responding
        array rarr;
        int   ret;
        rarr = ({ fd });
        if (select(&rarr, 0, 0, 30) < 1)
            // Timeout to wait acknowlegement
            break;

        // Wait for result
        if (mode == TYPE_UDP)
        {
            // Via UDP
            if ((ret = recv_from(fd, &buf, 256, &peer_ip, &peer_port)) > 0)
                break;
        } else
        {
            // Via TCP
            if ((ret = recv(fd, &buf, 256)) > 0)
                break;
        }

        printf("Bad responding! (ret = %d)\n", ret);
        sleep(3);
    }

    // Close the socket
    close_socket(fd);

    if (is_buffer(buf))
    {
        buf = des(buf, DES_DE1, "phxio");
        trace("auth", "Got acknowledgement (size = %d).\n", sizeof(buf));
    }

    return buf;
}

// Verify responding of CMD_AUTHENTICATE
private int check_authenticate(buffer ack, int send_time, int seed)
{
    int srv_time;
    int result;
    int r1, r2, r3, r4;

    if (ack[2] != CMD_AUTHENTICATE)
    {
        // Acknowledgement mismatched
        trace("auth", "Command(%d) is mismatched with (%x).\n",
              ack[2], CMD_AUTHENTICATE);
        return 0;
    }

    // Get service time & compare with the seeds
    srv_time = (int) ack[4..7] + send_time;
    if (seed != (int) ack[8..11])
    {
        // Seed is mismatched
        trace("auth", "Seed (%x) is mismatched with (%x) of AUTHENTICATE.\n",
              seed, (int) ack[8..11]);
        return 0;
    }

    // Get result
    result = (int) ack[12..15];
    r1 = (int) ack[16..19];
    r2 = (int) ack[20..23];
    r3 = (int) ack[24..27];
    r4 = (int) ack[28..31];
    result ^= r1 ^ r2 ^ r3 ^ r4;

    // Get service & type of the telnet service
    telnet_settings = (const mapping) ([
        "servive" : (int) ack[32..35],
        "type"    : (string) ack[36..47],
    ]);

    if (result != (int) md5(net32(srv_time + seed))[0..3])
    {
        trace("auth", "Bad result.\n");
        return 0;
    }

    return 1;
}

// Verify responding of CMD_REQUEST_PASSWORD
private mixed check_request_password(buffer ack, int send_time, int seed)
{
    int    pwd_len;
    string pwd;

    if (ack[2] != CMD_REQUEST_PASSWORD)
    {
        // Acknowledgement mismatched
        trace("auth", "Command(%d) is mismatched with (%x).\n",
              ack[2], CMD_REQUEST_PASSWORD);
        return 0;
    }

    if (seed != (int) ack[8..11])
    {
        // Seed is mismatched
        trace("auth", "Seed (%x) is mismatched with (%x) of REQUEST_PASSWORD.\n",
              seed, (int) ack[8..11]);
        return 0;
    }

    // Get password
    pwd_len = (int) ack[12..15];
    pwd = (string) ack[16..15 + pwd_len];

    return pwd;
}

// Get auth-server's address
private int get_auth_addr(mixed &ip, mixed &port)
{
    string auth_addr;

    // Get authentication address
    auth_addr = expand_macro("AUTH");
    if (! is_string(auth_addr))
    {
        printf("Auth address is not specified, expected /D AUTH=address:port.\n");
        return 0;
    }

    if (sscanf(auth_addr, "%s:%d", &ip, &port) != 2)
    {
        // No port? Use default
        ip = auth_addr;
        port = 9977;
    }

    return 1;
}

// Ask for password
nomask primitive mixed get_telnet_password(string key)
{
    string  ip;         // Auth server IP
    int     port;       // Auth server Port
    int     send_time;  // Generated time of request
    int     seed;       // Seed of request
    buffer  buf;
    string  pwd;

    // Generate send time & seed
    send_time = time();
    seed = random() & 0xFFFFFFFF;

    if (! get_auth_addr(&ip, &port))
        // No specified auth server
        return 0;

    // Send request to destination
    printf("Connect to DAS for password...\n");

    // Send request
    buf = send_request(TYPE_TCP, atoinet(ip), port,
                       CMD_REQUEST_PASSWORD,
                       send_time, seed,
                       strlen(key), (buffer) key);

    if (! is_buffer(buf))
    {
        // No result
        printf("TIMEOUT\n");
        return 0;
    }

    if (sizeof(buf) < 64)
    {
        printf("Bad responding of CMD_REQUEST_PASSWORD! buffer size < 64\n");
        return 0;
    }

    if (! (pwd = check_request_password(buf, send_time, seed)))
    {
        // Bad result
        printf("NO RESPONDING\n");
        return 0;
    }

    printf("GOT PASSWORD\n");
    return pwd;
}

// Log message to DAS server
nomask primitive int log_to_das(string msg)
{
    string  ip;         // Auth server IP
    int     port;       // Auth server Port
    int     send_time;  // Generated time of request
    int     seed;       // Seed of request
    buffer  buf;

    // Generate send time & seed
    send_time = time();
    seed = random() & 0xFFFFFFFF;

    if (! get_auth_addr(&ip, &port))
        // No specified auth server
        return 0;

    // Send request & without responding
    buf = send_request(TYPE_UDP, atoinet(ip), port,
                       CMD_LOG_TO_DAS,
                       send_time, seed,
                       strlen(msg), (buffer) msg, 0);

    if (! is_buffer(buf))
    {
        // No result
        printf("TIMEOUT\n");
        return 0;
    }

    return 1;
}

// Do authencation
nomask primitive int auth_driver()
{
    int     timer_id;
    string  ip;         // Auth server IP
    int     port;       // Auth server Port
    int     send_time;  // Generated time of request
    int     seed;       // Seed of request
    int     old_process_count;
    buffer  buf;

    // Get max simultaneous threads count and set it to 1
    old_process_count = get_pconfig(1);
    change_pconfig(1, 1);

    // Shutdown after 30s
    set_timer(30, &timer_id, "shutdown");

    // Generate send time & seed
    send_time = time();
    seed = random() & 0xFFFFFFFF;

    if (! get_auth_addr(&ip, &port))
        // No specified auth server
        return 0;

    // Send request to destination
    printf("Connect to DAS for authenticating...\n");

    // Send request
    buf = send_request(TYPE_TCP, atoinet(ip), port,
                       CMD_AUTHENTICATE,
                       send_time, seed);

    if (! is_buffer(buf))
    {
        // No result
        printf("TIMEOUT\n");
        return 0;
    }

    if (sizeof(buf) < 64)
    {
        printf("Bad responding of CMD_AUTHENTICATE! buffer size < 64\n");
        return 0;
    }

    if (! check_authenticate(buf, send_time, seed))
    {
        // Bad result
        printf("FAILED\n");
        return 0;
    }

    // Reset scheduler threads's count
    change_pconfig(1, old_process_count);

    // Stop the timer
    delete_timer(timer_id);
    printf("OK\n");
    return 1;
}

// Try authenticating when startup
if (! auth_driver())
    // Failed to authenticate
    suspend_thread();
