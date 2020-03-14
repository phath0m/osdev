#include <dirent.h>
#include <fcntl.h>
#include <libini.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <collections/dict.h>
#include <sys/socket.h>
#include <sys/un.h>

typedef enum {
    SINGLE_USER = 1,
    MULTI_USER  = 2,
    GRAPHICAL   = 4
} runlevel_t;

struct service {
    char *      name; /* service name */
    char *      exec_start; /* command to start the service */
    pid_t       sid; /* session ID for service */
    runlevel_t  runlevels; /* valid runlevels to start service in */
    bool        running; /* whether or not this is running */
};

/* IPC request, used to interact with the daemon */
struct doit_request {
    int     opcode;
    int     arg;
    char    payload[512];
};

#define DOIT_SOCK_PATH "/var/run/doit.sock"

#define DOIT_CHANGE_RUNLEVEL    0x01

struct dict loaded_services;

static runlevel_t
parse_runlevel(const char *runlevel)
{
    char rlbuf[256];

    strncpy(rlbuf, runlevel, 256);

    runlevel_t mask = 0;
    
    char *tok = strtok(rlbuf, ",");

    while (tok) {

        int runlevel = atoi(tok);

        switch (runlevel) {
            case 1:
                mask |= SINGLE_USER;
                break;
            case 2:
                mask |= MULTI_USER;
                break;
            case 5:
                mask |= GRAPHICAL;
                break;
            default:
                return -1;
        }

        tok = strtok(NULL, ",");
    }
    
    return mask;
}

static int
load_service(const char *config_file)
{
    struct ini ini;
    
    if (ini_open(&ini, config_file)) {
        printf("doit: could not read %s!\n", config_file);
        return -1;
    }

    char *exec_start = ini_section_get(&ini, "Service", "ExecStart");
    char *service_name = ini_section_get(&ini, "Service", "Name");
    char *runlevels = ini_section_get(&ini, "Service", "RunLevel");

    if (!exec_start || !service_name) {
        printf("doit: invalid syntax in file: %s!\r\n", config_file);
        ini_close(&ini);
        return -1;
    }

    runlevel_t rlmask = parse_runlevel(runlevels);
    
    if (rlmask == -1) {
        ini_close(&ini);
        return -1;
    }

    struct service *service = calloc(1, sizeof(struct service));

    service->name = service_name;
    service->exec_start = exec_start;
    service->runlevels = rlmask;
    printf("doit: registering %s\r\n", service_name);
    dict_set(&loaded_services, service_name, service);

    return 0;
}

static int
shitty_arg_split(const char *cmd, char **argv, int max_args)
{
    int len = strlen(cmd);
    
    if (len > 512) {
        // sorry java... not today
        return -1;
    }

    char buf[512];
    
    strncpy(buf, cmd, sizeof(buf));

    int i = 0;
    int pos = 0;
    char *cur = buf;

    while (pos < len && buf[pos]) {
        if (buf[pos] == ' ') {
            buf[pos++] = '\x00';
            argv[i++] = cur;
            cur = &buf[pos];
        } else {
            pos++;
        }
    }

    argv[i++] = cur;
    argv[i++] = NULL;

    return 0;
}

static int
service_start(struct service *service)
{
    if (service->running) {
        return -1;
    }

    printf("doit: starting %s\r\n", service->name);

    service->running = true;

    pid_t pid = fork();

    if (pid == 0) {
        char *argv[24];

        setsid();

        if (shitty_arg_split(service->exec_start, argv, 24) == 0) {
            execv(argv[0], argv);
        }

        exit(-1);
    }   

    service->sid = pid;

    return 0;    
}

static void
change_runlevel(runlevel_t runlevel)
{
    list_iter_t iter;

    dict_get_keys(&loaded_services, &iter);

    const char *name;

    while (iter_move_next(&iter, (void**)&name)) {
        struct service *service;

        if (!dict_get(&loaded_services, name, (void**)&service)) continue;

        if ((service->runlevels & runlevel)) {
            service_start(service);
        }
    }
}

static void
do_directory(const char *directory)
{
    struct dirent *dirent;

    DIR *dirp = opendir(directory);

    while ((dirent = readdir(dirp))) {
        char *ext = strrchr(dirent->d_name, '.');

        if (ext && strncmp(ext, ".service", 8) == 0) {
            char full_path[256];
            sprintf(full_path, "%s/%s", directory, dirent->d_name);
            load_service(full_path);
            // load INI file
        }
    }

    closedir(dirp);
}

/* set the system time */
static void
set_system_time()
{
    extern int adjtime(const struct timeval *, struct timeval *);

    int fd = open("/dev/rtc", O_RDONLY);

    if (fd == -1) {
        fputs("doit: could not set time!\r\n", stderr);
        return;
    }

    time_t delta_seconds = 0;

    read(fd, &delta_seconds, 4);
    close(fd);

    struct timeval olddelta;
    struct timeval delta;
    adjtime(NULL, &olddelta);

    delta.tv_sec = delta_seconds + olddelta.tv_sec;
    delta.tv_usec = 0;

    adjtime(&delta, NULL);
}

/* this should only be called onced; initializes the actual daemon and starts services*/
static void
start_daemon(int argc, const char *argv[])
{
    const char *console = getenv("CONSOLE");

    for (int i = 0; i < 3; i++) {
        open(console, O_RDWR);
    }

    set_system_time();

    runlevel_t target = MULTI_USER;

    if (argc == 2) {
        target = parse_runlevel(argv[1]);
        
        if (target == -1) {
            printf("doit: invalid runlevel specified! assume multi-user!\r\n");
            target = MULTI_USER;
        } else {
            printf("doit: runlevel=%s\r\n", argv[1]);
        }
    }

    memset(&loaded_services, 0, sizeof(loaded_services));

    do_directory("/etc/doit.d");

    change_runlevel(target);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un  addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;

    strcpy(addr.sun_path, DOIT_SOCK_PATH);

    bind(fd, (struct sockaddr*)&addr, sizeof(addr));
 
    for (;;) {
        int client = accept(fd, NULL, NULL);

        struct doit_request request;
        struct doit_request response;

        read(client, &request, sizeof(request));

        switch (request.opcode) {
            case DOIT_CHANGE_RUNLEVEL:
                printf("doit: change runlevel to %d\r\n", request.arg);
                break;
        }

        write(client, &response, sizeof(response));
        close(client);
    }
}

static void
send_ipc_request(int opcode, int arg, const char *payload)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un  addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;

    strcpy(addr.sun_path, DOIT_SOCK_PATH);

    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    struct doit_request request;

    request.opcode = opcode;
    request.arg = arg;

    write(fd, &request, sizeof(request));
    read(fd, &request, sizeof(request));

    close(fd);
 }

static int
handle_runlevel_command(const char *runlevel)
{
    send_ipc_request(DOIT_CHANGE_RUNLEVEL, atoi(runlevel), NULL);
    
    return 0;
}

int
main(int argc, const char *argv[])
{
    if (getpid() == 1) {
        start_daemon(argc, argv);
        return 0;
    }


    if (argc != 3) {
        fprintf(stderr, "usage: doit (runlevel|start|stop) arg\n");
        return -1;
    }

    const char *command = argv[1];

    if (strcmp(command, "runlevel") == 0) {
        return handle_runlevel_command(argv[2]);
    }

    return 0;
}
