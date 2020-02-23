/*
 * This is the actual core protocol implementation
 * libxtc is the client component of this.
 *
 * This will create an IPC socket and use it to accept connections
 * from clients.
 *
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>
#include <unistd.h>
#include <xtcprotocol.h>
#include <collections/list.h>
#include <libmemgfx/canvas.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "window.h"
#include "wm.h"

#define SESSION_MAX_WINDOWS     256

struct xtc_session {
    int             sfd;
    struct wmctx *  ctx;
    struct window * windows[SESSION_MAX_WINDOWS];
};

static void
set_no_exec(int fd)
{
    int flags;

    fcntl(fd, F_GETFD, &flags);
    flags |= FD_CLOEXEC;
    fcntl(fd, F_SETFD, &flags);
}

static int
send_response(struct xtc_session *session, uint32_t *parameters, int paramc, int err)
{
    struct xtc_msg_hdr resp;

    memset(resp.parameters, 0, sizeof(resp.parameters));
    
    if (parameters && paramc > 0) {
        memcpy(resp.parameters, parameters, paramc * sizeof(uint32_t));
    }

    resp.error = err;
    resp.payload_size = 0;

    if (write(session->sfd, &resp, sizeof(resp)) == -1) {
        return -1;
    }

    return 0;
}

static int
send_response_ex(struct xtc_session *session, uint32_t *params, int paramc, int err ,void *data, size_t size)
{
    struct xtc_msg_hdr resp;

    memset(resp.parameters, 0, sizeof(resp.parameters));

    if (params && paramc > 0) {
        memcpy(resp.parameters, params, paramc * sizeof(uint32_t));
    }

    resp.error = err;
    resp.payload_size = size;

    if (write(session->sfd, &resp, sizeof(resp)) == -1) {
        return -1;
    }

    if (write(session->sfd, data, resp.payload_size) == -1) {
        return -1;
    }

    return 0;
}

static void
handle_new_win(struct xtc_session *session, struct xtc_msg_hdr *hdr)
{
    int width = hdr->parameters[0];
    int height = hdr->parameters[1];

    struct window *win = window_new(10, 10, width, height);

    uint32_t window_id;

    for (window_id = 0; window_id < SESSION_MAX_WINDOWS; window_id++) {
        if (!session->windows[window_id]) {
            break;
        }
    }
    
    if (window_id >= SESSION_MAX_WINDOWS) {
        send_response(session, NULL, 0, XTC_ERR_NOMOREWINDOWS);
        free(win);
        return;
    }
    
    session->windows[window_id] = win;

    wm_add_window(session->ctx, win);

    send_response(session, &window_id, 1, 0);
}

static void
handle_set_title(struct xtc_session *session, struct xtc_msg_hdr *hdr, void *payload)
{
    uint32_t window_id = hdr->parameters[0];

    if (window_id >= SESSION_MAX_WINDOWS ||
            !session->windows[window_id])
    {
        send_response(session, NULL, 0, XTC_ERR_NOSUCHWIN);
        printf("XTC_ERR_NOSUCHWIN\n");
        return;
    }

    struct window *win = session->windows[window_id];
    
    window_set_title(win, (const char*)payload);

    send_response(session, NULL, 0, 0);
}

static void
handle_put_string(struct xtc_session *session, struct xtc_msg_hdr *hdr, void *payload)
{
    uint32_t window_id = hdr->parameters[0];

    if (window_id >= SESSION_MAX_WINDOWS ||
            !session->windows[window_id])
    {
        send_response(session, NULL, 0, XTC_ERR_NOSUCHWIN);
        printf("XTC_ERR_NOSUCHWIN\n");
        return;
    }

    struct window *win = session->windows[window_id];

    int x = hdr->parameters[1];
    int y = hdr->parameters[2];
    int c = 0;

    canvas_puts(win->canvas, x, y, (const char*)payload, c);

    send_response(session, NULL, 0, 0);
}

static void
handle_clear(struct xtc_session *session, struct xtc_msg_hdr *hdr)
{
    uint32_t window_id = hdr->parameters[0];

    if (window_id >= SESSION_MAX_WINDOWS ||
            !session->windows[window_id])
    {
        send_response(session, NULL, 0, XTC_ERR_NOSUCHWIN);
        printf("XTC_ERR_NOSUCHWIN\n");
        return;
    }

    struct window *win = session->windows[window_id];

    canvas_clear(win->canvas, hdr->parameters[1]);

    send_response(session, NULL, 0, 0);
}

static void
handle_open_canvas(struct xtc_session *session, struct xtc_msg_hdr *hdr)
{
    uint32_t window_id = hdr->parameters[0];

    if (window_id >= SESSION_MAX_WINDOWS ||
            !session->windows[window_id])
    {
        send_response(session, NULL, 0, XTC_ERR_NOSUCHWIN);
        printf("XTC_ERR_NOSUCHWIN\n");
        return;
    }

    struct window *win = session->windows[window_id];

    uint32_t params[2] = {
        win->width,
        win->height
    };

    send_response_ex(session, params, 2, 0, win->shm_name, 256);
}

static void
handle_paint(struct xtc_session *session, struct xtc_msg_hdr *hdr)
{
    uint32_t window_id = hdr->parameters[0];

    if (window_id >= SESSION_MAX_WINDOWS ||
            !session->windows[window_id])
    {
        send_response(session, NULL, 0, XTC_ERR_NOSUCHWIN);
        printf("XTC_ERR_NOSUCHWIN\n");
        return;
    }

    struct window *win = session->windows[window_id];

    canvas_paint(win->canvas);

    send_response(session, NULL, 0, 0);
}

static void
handle_next_event(struct xtc_session *session, struct xtc_msg_hdr *hdr)
{
    uint32_t window_id = hdr->parameters[0];

    if (window_id >= SESSION_MAX_WINDOWS ||
            !session->windows[window_id])
    {
        send_response(session, NULL, 0, XTC_ERR_NOSUCHWIN);
        printf("XTC_ERR_NOSUCHWIN\n");
        return;
    }

    struct window *win = session->windows[window_id];

    while (LIST_SIZE(&win->events) == 0) {
        thread_cond_wait(&win->event_cond);
    }

    thread_spin_lock(&win->event_lock);

    struct window_event *event;

    list_remove_front(&win->events, (void**)&event);

    thread_spin_unlock(&win->event_lock);

    uint32_t params[3];
    params[0] = event->type;
    params[1] = event->x;
    params[2] = event->y;
    send_response(session, params, 3, 0);

    free(event);
}

static void
handle_request(struct xtc_session *session, struct xtc_msg_hdr *hdr, void *payload)
{
    switch (hdr->opcode) {
        case XTC_NEWWIN:
            handle_new_win(session, hdr);
            break;
        case XTC_SETWINTITLE:
            handle_set_title(session, hdr, payload);
            break;
        case XTC_NEXTEVENT:
            handle_next_event(session, hdr);
            break;
        case XTC_PUTSTRING:
            handle_put_string(session, hdr, payload);
            break;
        case XTC_CLEAR:
            handle_clear(session, hdr);
            break;
        case XTC_PAINT:
            handle_paint(session, hdr);
            break;
        case XTC_OPEN_CANVAS:
            handle_open_canvas(session, hdr);
            break;
    }
}

static void *
handle_connection(void *arg)
{
    
    struct xtc_session *session = arg;
     
    int sfd = session->sfd;

    for (;;) {
        void *payload = NULL;
        struct xtc_msg_hdr request;

        ssize_t nread = read(sfd, &request, sizeof(request));

        if (nread == -1) {
            break;
        }

        if (request.payload_size > 0) {
            payload = calloc(1, request.payload_size+1);
            read(sfd, payload, request.payload_size);
        }

        handle_request(session, &request, payload);

        if (payload) {
            free(payload);
        }        
    }

    close(sfd);

    for (int i = 0; i < SESSION_MAX_WINDOWS; i++) {
        if (session->windows[i]) {
            wm_remove_window(session->ctx, session->windows[i]);
            window_destroy(session->windows[i]);
        }
    }

    free(session);    
    
    return NULL;
}

void
server_listen(void *ctx)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    set_no_exec(fd);

    struct sockaddr_un  addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;

    strcpy(addr.sun_path, "/tmp/xtc_socket");

    bind(fd, (struct sockaddr*)&addr, sizeof(addr));

    for (;;) {
        int sfd = accept(fd, NULL, NULL);

        set_no_exec(sfd);

        struct xtc_session *session = calloc(1, sizeof(struct xtc_session));
        session->ctx = ctx;
        session->sfd = sfd;

        thread_t thread;
        thread_create(&thread, handle_connection, (void*)session);
    }
    
    close(fd);
}
