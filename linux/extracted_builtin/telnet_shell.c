// telnet_shell.c
// Created by doing  July/10/2010
// Script telnet shell which used to replace old C-implementation.
// More simpler.

// Interface
void start_buildin_telnet_shell(int i)
{
    create_thread(0, "buildin_telnet_daemon", i);
}

// Callback from C
void telnet_new_client(int i)
{
    start_buildin_telnet_shell(i);
}

// telnet shell daemon
void buildin_telnet_daemon(int i)
{
    string str;
    string prompt;
    int echo_enabled;
    int run_in_background;

    // Wait a moment
    sleep(0.1);

    // Save echo config and switch to echo 0 temporarily
    echo_enabled = telnet_config_info(i)["echo"];
    telnet_config(i, ([ "auth" : 0, "echo" : 0 ]));

    // Get prompt info
    prompt = get_prompt();

    // Authentication
    for (;;)
    {
        printf("Input password: ");
        str = trim(input_string());
        if (str == ".")
        {
            printf("Logout\n");
            close_telnet(i);
            return;
        }

        // Check password
        if (! telnet_check_password(str))
        {
            printf("Bad password!\n\n");
            continue;
        }

        // Got the password
        break;
    }

    // Recoverd original echo config
    telnet_config(i, ([ "auth" : 1, "echo" : echo_enabled ]));

    // Processing commands
    printf("New telnet cilent connected! i = %d\n", i);
    printf("%s", prompt);
    while ((str = trim(input_string())) != ".")
    {
        if ((strlen(str) > 0 && str[<1] == '&'))
        {
            run_in_background = 1;
            str = str[0..<2];
        } else
            run_in_background = 0;

        catch(execute_data(str, ! run_in_background));

        printf("\n%s", prompt);
    }

    // User input '.', close telnet cient
    close_telnet(i);
}
