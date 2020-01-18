#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

static void
do_login(struct passwd *pwd)
{
    static char *sh_argv[2] = {
        "-sh",
        NULL
    };

    execv(pwd->pw_shell, sh_argv);
}

static void
show_motd()
{
    FILE *fp = fopen("/etc/motd", "r");

    if (!fp) {
        return;
    }

    size_t nread;
    char buf[512];

    while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0) {
        fwrite(buf, 1, nread, stdout);
    }
    
    fflush(stdout);
}

static int
attempt_login(const char *login, const char *passwd)
{
    struct passwd *pwd = getpwnam(login);

    if (!pwd) {
        return -1;
    }

    puts("");
    show_motd();
    puts("");

    do_login(pwd);

    return 0;
}

int
main(int argc, char *argv[])
{
    
    char login[512];
    char password[512];

    char *tty = ttyname(0);

    if (!tty) {
        return -1;
    }

    char *tty_name = strrchr(tty, '/') + 1;

    time_t now = time(NULL);
    
    struct tm *ptm = gmtime(&now);

    puts(asctime(ptm));
    
    struct utsname uname_buf;
    uname(&uname_buf);
    printf("%s/%s (%s)\n\n", uname_buf.sysname, uname_buf.machine, tty_name);

    for (;;) {
        fputs("login: ", stderr);
        fgets(login, 512, stdin);

        fputs("Password: ", stderr);
        fgets(password, 512, stdin);

        login[strcspn(login, "\n")] = 0;
        password[strcspn(password, "\n")] = 0;

        if (attempt_login(login, password) != 0) {
            fputs("Login incorrect\n", stderr);
        }
    }
    return 0;
}

