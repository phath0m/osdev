#include <authlib.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>

static int
do_login(struct passwd *pwd)
{
    static char *sh_argv[2] = {
        "-sh",
        NULL
    };

    pid_t child;

    child = fork();

    if (child == 0) {   
        setuid(pwd->pw_uid);
        setgid(pwd->pw_gid);

        setenv("HOME", pwd->pw_dir, true);

        execv(pwd->pw_shell, sh_argv);

        perror("execv");
        exit(-1);
    } else {
        wait(NULL);
    }

    return 0;
}

static void
show_motd()
{
    size_t nread;
    char buf[512];

    FILE *fp;

    fp = fopen("/etc/motd", "r");

    if (!fp) {
        return;
    }

    while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0) {
        fwrite(buf, 1, nread, stdout);
    }
    
    fflush(stdout);
    fclose(fp);
}

static int
attempt_login(const char *login, const char *passwd)
{
    int login_flags;

    struct login login_buf;
    struct passwd *pwd;

    pwd = getpwnam(login);

    if (!pwd) {
        return -1;
    }

    login_flags = AUTH_UPDATE_UTMP | AUTH_UPDATE_WTMP | AUTH_UPDATE_LASTLOG | AUTH_UPDATE_BTMP;
    
    if (authlib_login(&login_buf, login, passwd, login_flags) != 0) {
        return -1;
    } 

    puts("");
    show_motd();
    puts("");

    do_login(pwd);

    authlib_logout(&login_buf);
    
    return 0;
}

int
main(int argc, char *argv[])
{
    time_t now;;

    char login[512];
    char password[512];
 
    struct termios termios;
    struct utsname uname_buf;
 
    char *tty;
    char *tty_name;
    struct tm *ptm;

    setsid();
    ioctl(STDIN_FILENO, TIOCSCTTY, NULL);

    tty = ttyname(STDIN_FILENO);

    if (!tty) {
        return -1;
    }

    tty_name = strrchr(tty, '/') + 1;
    now = time(NULL);
    ptm = gmtime(&now);

    puts(asctime(ptm));
    
    uname(&uname_buf);
    printf("%s/%s (%s)\n\n", uname_buf.sysname, uname_buf.machine, tty_name);

    tcgetattr(STDIN_FILENO, &termios);

    for (;;) {
        fputs("login: ", stderr);
        fgets(login, 512, stdin);
        fputs("Password: ", stderr);
        
        /* disable echo for password */
        termios.c_lflag &= ~(ECHO);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios);
        fgets(password, 512, stdin);

        /* renable echo */
        termios.c_lflag |= ECHO;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios);

        login[strcspn(login, "\n")] = 0;
        password[strcspn(password, "\n")] = 0;

        if (attempt_login(login, password) != 0) {
            fputs("Login incorrect\n", stderr);
            sleep(2);
        }
    }

    return 0;
}