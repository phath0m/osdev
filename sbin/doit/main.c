#include <dirent.h>
#include <fcntl.h>
#include <libini.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <collections/dict.h>

struct service {
    char *  name;
    char *  exec_start;
    bool    running;
};

struct dict loaded_services;

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

    if (!exec_start || !service_name) {
        ini_close(&ini);
        return -1;
    }

    struct service *service = calloc(1, sizeof(struct service));

    service->name = service_name;
    service->exec_start = exec_start;

    printf("doit: registering %s\n", service_name);
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

    service->running = true;

    pid_t pid = fork();

    if (pid == 0) {
        char *argv[24];

        if (shitty_arg_split(service->exec_start, argv, 24) == 0) {
            execv(argv[0], argv);
        }

        exit(-1);
    }   

    
    return 0;    
}

static void
start_services()
{
    list_iter_t iter;

    dict_get_keys(&loaded_services, &iter);

    const char *name;

    while (iter_move_next(&iter, (void**)&name)) {
        struct service *service;

        if (!dict_get(&loaded_services, name, (void**)&service)) continue;

        service_start(service);
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

int
main(int argc, const char *argv[])
{
    memset(&loaded_services, 0, sizeof(loaded_services));

    if (argc > 1) {
        const char *operation = argv[1];

        if (strcmp(operation, "--start-directory") == 0 && argc == 3) {
            do_directory(argv[2]);
        }
    } else {
        const char *console = getenv("CONSOLE");

        for (int i = 0; i < 3; i++) {
            open(console, O_RDWR);
        }

        printf("doit: Welcome!\n");

        do_directory("/etc/doit.d");
    }

    start_services();

    for (;;) {
        pause();
    }

    return 0;
}
