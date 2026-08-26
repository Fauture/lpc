/* Simple runnable LPC server example for lpcvm.
 * Start with: lpcvm server examples/server.c 4000
 * Then telnet 127.0.0.1 4000 and try: hello / who / help
 */

int total_connections;

void create() {
    total_connections = 0;
    db_open("lpcvm.db");
    write("Example LPC server ready.\n");
}

string player_command(int fd, string cmd) {
    if (cmd == "hello") {
        return "Hello from LPC VM!";
    }
    if (cmd == "who") {
        return sprintf("total_connections=%d", total_connections);
    }
    if (cmd == "time") {
        return sprintf("db size=%d", sizeof(db_query()));
    }
    if (cmd == "help") {
        return "Commands: hello, who, time, dbdemo, help";
    }
    if (cmd == "dbdemo") {
        db_set("demo_key", "hello-db");
        return db_get("demo_key");
    }
    return "Unknown command. Try help.";
}

void heart_beat() {
    total_connections = total_connections + 1;
}
