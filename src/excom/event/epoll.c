#include "excom.h"
#ifdef EXCOM_EPOLL

int excom_event_base_epoll_init(excom_event_base_epoll_t* base,
  excom_event_runner_t* runner)
{
  base->loop      = true;
  base->epollfd   = epoll_create(1);
  base->timeout   = 500;
  base->maxevents = 32;
  base->runner    = runner;

  if(base->epollfd < 1)
  {
    excom_return_errno();
  }

  return 0;
}

int excom_event_epoll_update(excom_event_base_epoll_t* base,
  excom_event_t* event)
{
  int err;
  struct epoll_event epevent;

  epevent.events   = event->flags;
  epevent.data.ptr = event;

  err = epoll_ctl(base->epollfd, EPOLL_CTL_MOD, event->fd, &epevent);

  if(err < 0)
  {
    excom_return_errno();
  }

  return 0;
}

int excom_event_epoll_add(excom_event_base_epoll_t* base,
  excom_event_t* event)
{
  int err;
  struct epoll_event epevent;
  event->base = base;

  epevent.events  = event->flags;
  epevent.data.ptr = event;

  err = epoll_ctl(base->epollfd, EPOLL_CTL_ADD, event->fd, &epevent);

  if(err < 0)
  {
    excom_return_errno();
  }

  return 0;
}

int excom_event_epoll_remove(excom_event_base_epoll_t* base,
  excom_event_t* event)
{
  int err;

  // don't worry, the last argument is ignored.
  err = epoll_ctl(base->epollfd, EPOLL_CTL_DEL, event->fd,
    (struct epoll_event*) event);

  if(err < 0)
  {
    excom_return_errno();
  }

  return 0;
}

void excom_event_epoll_loop(excom_event_base_epoll_t* base, void* ptr)
{
  //struct epoll_event* events;
  excom_event_t event, *eptr;
  int n, i;
  struct epoll_event events[base->maxevents];
  event.base = base;

  while(base->loop)
  {
    n = epoll_wait(base->epollfd, events, base->maxevents,
      base->timeout);

    if(n < 0 && errno == EINTR)
    {
      base->loop = false;
    }

    for(i = 0; i < n; i++)
    {
      eptr = (excom_event_t*) events[i].data.ptr;
      event.root  = eptr;
      event.fd    = eptr->fd;
      event.data  = eptr->data;
      event.flags = events[i].events;
      (*base->runner)(event, ptr);
    }
  }
}

void excom_event_epoll_loop_end(excom_event_base_epoll_t* base)
{
  base->loop = false;
}

#endif
