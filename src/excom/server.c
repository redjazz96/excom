#include "excom.h"

#include <stdio.h>
#include <errno.h>

#ifdef EXCOM_POSIX
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <netdb.h>
#endif

#include <fcntl.h>

/*!
 * For now, we'll assume the number of threads we'll need is 5.  This
 * may be a terrible assumption, but hey.
 */
#define EXCOM_THREADS 5

void excom_server_init(excom_server_t* server)
{
  server->sock = -1;
  server->addr = EXCOM_DEFAULT_ADDR;
  server->port = EXCOM_DEFAULT_PORT;
  server->clients = NULL;
}

#define ERROR_CHECK(value) do \
{                             \
  if(value < 0)               \
  {                           \
    err = errno;              \
    errno = 0;                \
    return err;               \
  }                           \
} while(0)

static int socket_non_blocking(int fd)
{
  int flags, err;
  unsigned long param;
#ifdef EXCOM_POSIX
  (void) param;
  flags = fcntl(fd, F_GETFL);
  ERROR_CHECK(flags);

  flags |= O_NONBLOCK;

  err = fcntl(fd, F_SETFL, flags);
  ERROR_CHECK(err);

#else
  param = 1;
  (void) flags;

  ioctlsocket(fd, FIONBIO, &param);
#endif

  return 0;
}

int excom_server_bind(excom_server_t* server)
{
  bool val = true;
  int err;
  size_t len = sizeof(bool);
  struct sockaddr_in name;

  server->sock = socket(AF_INET, SOCK_STREAM, 0);
  ERROR_CHECK(server->sock);

  name.sin_family      = AF_INET;
  name.sin_port        = htons(server->port);
  name.sin_addr.s_addr = htonl(server->addr);

  err = bind(server->sock, (struct sockaddr*) &name, sizeof(name));
  ERROR_CHECK(err);
  printf("server: %d\n", server->sock);

  err = listen(server->sock, 128);
  ERROR_CHECK(err);

  val = true;
  setsockopt(server->sock, SOL_SOCKET, SO_KEEPALIVE, &val, len);

  err = socket_non_blocking(server->sock);
  ERROR_CHECK(err);

  return 0;
}

static void _accept(excom_event_t event, excom_server_t* server)
{
  int err;
  excom_server_client_t* client;

  while(1)
  {
    err = accept(event.fd, NULL, NULL);
    if(err == -1 && (errno != EAGAIN && errno != EWOULDBLOCK))
    {
      excom_check_error(errno, 0, "Failed to accept client");
      return;
    }
    else if(err == -1)
    {
      return;
    }

    client = excom_malloc(sizeof(excom_server_client_t));
    client->server = server;

    client->event.fd    = err;
    client->event.flags = EXCOM_EVENT_READ |
      EXCOM_EVENT_ERROR | EXCOM_EVENT_CLOSE;
    client->event.data  = client;
    client->_prev       = server->clients;
    client->_next       = NULL;

    if(server->clients)
    {
      server->clients->_next = client;
    }
    err = socket_non_blocking(err);
    if(err < 0)
    {
      close(client->event.fd);
    }
    else
    {
      excom_server_client_connect(event, client);
      excom_event_add(&server->base, &client->event);
      client->_next = server->clients;
      server->clients = client;
      excom_server_client_read(client->event, client);
    }
  }
}

static void _disconnect(excom_event_t event, excom_server_t* server)
{
  excom_server_client_t* client = event.data;
  if(client)
  {
    if(server->clients == client)
    {
      server->clients = client->_next;
    }
    if(client->_prev)
    {
      client->_prev->_next = client->_next;
    }
    if(client->_next)
    {
      client->_next->_prev = client->_prev;
    }

    excom_free(client);
    event.data = NULL;
  }

  close(event.fd);
}

static void on_event(excom_event_t event, void* ptr)
{
  excom_server_t* server = (excom_server_t*) ptr;
  int err;

  // this is our listen socket!
  if(event.fd == server->sock)
  {
    // we can accept a new client :3
    if(event.flags & EXCOM_EVENT_READ)
    {
      _accept(event, server);
    }
    else if(event.flags & EXCOM_EVENT_ERROR)
    {
      err = read(event.fd, NULL, 0);
      if((err == 0) && (errno == 0)) errno = EIO;
      excom_check_error(errno, 0, "Error on accept socket");
      excom_event_loop_end(&server->base);
      return;
    }
    else if(event.flags & EXCOM_EVENT_CLOSE)
    {
      // wuh?  It was closed?

      excom_check_error(EBADF, 1, "on_event");
    }
  }
  else // it's a client socket?? :o!!
  {
    if(event.flags & EXCOM_EVENT_READ)
    {
      printf("EXCOM EVENT READ\n");
      excom_server_client_read(event, event.data);
    }
    if(event.flags & EXCOM_EVENT_WRITE)
    {
      printf("EXCOM EVENT WRITE\n");
      excom_server_client_write(event, event.data);
    }
    if(event.flags & EXCOM_EVENT_ERROR)
    {
      printf("EXCOM EVENT ERROR\n");
      excom_server_client_error(event, event.data);
    }
    if(event.flags & EXCOM_EVENT_CLOSE)
    {
      printf("EXCOM EVENT CLOSE\n");
      excom_server_client_close(event, event.data);
      _disconnect(event, server);
    }
  }
}

int excom_server_run(excom_server_t* server)
{
  int err;
  (void) server;

  excom_event_t listen;

  err = excom_event_base_init(&server->base,
    on_event);
  if(err != 0)
  {
    excom_return_errno();
  }

  listen.fd    = server->sock;
  listen.flags = EXCOM_EVENT_READ | EXCOM_EVENT_ERROR |
    EXCOM_EVENT_CLOSE;
  listen.data  = server;

  excom_event_add(&server->base, &listen);

  excom_event_loop(&server->base, server);

  return 0;

}

void excom_server_destroy(excom_server_t* server)
{
  excom_server_client_t *client, *next;
  excom_event_t shutdown = {
    .base   = &server->base,
    .fd     = server->sock,
    .flags  = EXCOM_EVENT_CLOSE,
    .data   = NULL,
    .root   = NULL
  };

  client = server->clients;

  while(client != NULL)
  {
    shutdown.data = client;
    next = client->_next;
    excom_server_client_close(shutdown, client);
    _disconnect(shutdown, server);
    client = next;
  }

  server->clients = NULL;
  close(server->sock);
  server->sock = -1;

}
