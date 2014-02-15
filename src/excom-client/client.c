#include "excom.h"

static void* event_loop(void*);
static void event_listener(excom_event_t, void*);

#define check_error do{if(err){return;}}while(0)

void excom_client_handle_packets(excom_client_t* client,
  excom_client_packet_handler_t* handler)
{
  int err;

  excom_event_t event;

  event.fd = client->sock;
  event.data = NULL;
  event.flags = EXCOM_EVENT_ALL;

  client->handler = handler;

  err = excom_event_base_init(&client->base, event_listener);
  check_error;
  err = excom_event_add(&client->base, &event);
  check_error;
  err = excom_thread_init(&client->thread, event_loop,
    client);
  check_error;
  err = excom_mutex_init(&client->mutex);
  check_error;
}

static void* event_loop(void* cl)
{
  excom_client_t* client = cl;

  excom_event_loop(&client->base, client);

  return NULL;
}

static void check_packet(excom_client_t* client)
{
  excom_packet_t* packet;
  uint32_t size;
  int err;

  excom_mutex_lock(&client->buf.in.mutex);

  // We need to at least have 4 bytes to read.  The four bytes should
  // represent the size of what is to come.
  if(excom_buffer_remaining(&client->buf.in) < 4)
  {
    excom_mutex_unlock(&client->buf.in.mutex);
    return;
  }

  excom_protocol_unpack_uint32_t((char*) client->buf.in.pos, &size);

  // The entire packet hasn't come through yet, so we need to wait
  // until it does.
  if(excom_buffer_remaining(&client->buf.in) < 4 + size)
  {
    excom_mutex_unlock(&client->buf.in.mutex);
    return;
  }

  packet = excom_malloc(sizeof(excom_packet_t));
  err = excom_protocol_read_packet(&client->buf.in, packet);
  excom_mutex_unlock(&client->buf.in.mutex);

  if(err)
  {
    excom_check_error(err, 0, "read_packet");
    excom_free(packet);
    return;
  }

  packet->_next = client->packets;
  client->packets = packet;

  client->handler(packet, client);
}
#undef client_data

static void clread(excom_event_t event,
  excom_client_t* client)
{
  char buf[32];
  ssize_t out;
  int err = 0;

  (void) event;

  while(err == 0 && !client->disconnected)
  {
    out = read(client->sock, buf, 32);

    if(out > 0)
    {
      excom_buffer_cappend(&client->buf.in, buf, out);
    }
    else if(out < 0)
    {
      err = errno;
    }
    else
    {
      err = -1;
    }
  }

  if(errno != 0)
  {
    check_packet(client);
  }
}

static void clwrite(excom_event_t event,
  excom_client_t* client)
{
  (void) event;

  excom_buffer_write(&client->buf.out, client->sock);
}

static void clerror(excom_event_t event,
  excom_client_t* client)
{
  // umm...
  // this is awkward.

  (void) event;
  (void) client;
}

static void clclose(excom_event_t event,
  excom_client_t* client)
{
  (void) event;
  (void) client;
  excom_packet_t* packet,* next;

  excom_buffer_destroy(&client->buf.in);
  excom_mutex_lock(&client->mutex);

  packet = client->packets;

  while(packet)
  {
    next = packet->_next;
    excom_free(packet);
    packet = next;
  }

  client->disconnected = true;

  excom_event_loop_end(&client->base);
  excom_mutex_unlock(&client->mutex);
}

static void event_listener(excom_event_t event, void* ptr)
{
  excom_client_t* client = ptr;

  if(event.flags & EXCOM_EVENT_READ)
  {
    clread(event, client);
  }

  if(event.flags & EXCOM_EVENT_WRITE)
  {
    clwrite(event, client);
  }

  if(event.flags & EXCOM_EVENT_CLOSE)
  {
    clclose(event, client);
  }

  if(event.flags & EXCOM_EVENT_ERROR)
  {
    clerror(event, client);
  }
}