/*
 * Copyright 2016 paulguy <paulguy119@gmail.com>
 * 
 * This file is part of InterAct SV2020 web.remote uinput driver.
 * 
 * InterAct SV2020 web.remote uinput driver is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * 
 * InterAct SV2020 web.remote uinput driver is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with InterAct SV2020 web.remote uinput driver.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/*
 * Some information acquired from remote.cpp from Calvin Harrigan's
 * SunAmp which is released under the GPL version 2 license.
 * <http://user.xmission.com/~rebling/pub/remote.cpp>
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include "sv2020.h"

int sv2020 = -1, uinput = -1;

void cleanupandexit(int exitcode);
void handler(int signal);
int uinput_setup();
int uinput_send(int fd, int type, int code, int value);
void uinput_close(int fd);

int keycodes[] = {
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_PAGEUP,
  KEY_PAGEDOWN,
  KEY_F1,
  KEY_F2,
  KEY_F3
};

int main(int argc, char **argv) {
  struct timespec ts;
  char buffer[PACKETSIZE];
  int ret;
  sv2020_state state_now;
  sv2020_key lastkey = SV2020_KNONE, nowkey;
  int lastmb = 0;
  int changed;
  
  struct sigaction act = {
    .sa_handler = handler,
    .sa_sigaction = NULL,
    .sa_mask = 0,
    .sa_flags = 0,
    .sa_restorer = NULL
  };

  if(argc < 2) {
    fprintf(stderr, "USAGE: %s <serial device>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  sv2020 = sv2020_open(argv[1]);
  if(sv2020 < 0) {
    fprintf(stderr, "Failed to open device %s.\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  uinput = uinput_setup();
  if(uinput < 0) {
    fprintf(stderr, "Failed to set up uinput.\n");
    cleanupandexit(EXIT_FAILURE);
  }

  if(sigaction(SIGINT, &act, NULL) < 0) {
    fprintf(stderr, "WARNING: Couldn't set up signal handler.\n");
    perror("sigaction");
  }
  
  printf("CTRL+C to quit.\n");

  for(;;) {
    ret = sv2020_getPacket(buffer, sv2020);

    if(ret == -2) {
      fprintf(stderr, "Short read, something bad happened.\n");
      break;
    } else if(ret == -1) {
      fprintf(stderr, "Error getting packet.\n");
      break;
    } else if(ret == 0) {
      sv2020_stateFromPacket(&state_now, buffer);
      
      changed = 0;
      if(state_now.rel_x != 0 || state_now.rel_y != 0) {
        //fprintf(stderr, "Mouse moved %d, %d.\n", state_now.rel_x, state_now.rel_y);
        if(state_now.rel_x != 0) {
          if(uinput_send(uinput, EV_REL, REL_X, state_now.rel_x) < 0)
            perror("uinput_send");
          else
            changed = 1;
        }
        if(state_now.rel_y != 0) {
          if(uinput_send(uinput, EV_REL, REL_Y, state_now.rel_y) < 0)
            perror("uinput_send");
          else
            changed = 1;
        }
      }
      if(!(state_now.mousebuttons & 0x2) && (lastmb & 0x2)) {
        //fprintf(stderr, "Left button released.\n");
        if(uinput_send(uinput, EV_KEY, BTN_LEFT, 0) < 0)
          perror("uinput_send");
        else
          changed = 1;
      }
      if((state_now.mousebuttons & 0x2) && !(lastmb & 0x2)) {
        //fprintf(stderr, "Left button pressed.\n");
        if(uinput_send(uinput, EV_KEY, BTN_LEFT, 1) < 0)
          perror("uinput_send");
        else
          changed = 1;
      }
      if(!(state_now.mousebuttons & 0x1) && (lastmb & 0x1)) {
        //fprintf(stderr, "Right button released.\n");
        if(uinput_send(uinput, EV_KEY, BTN_RIGHT, 0) < 0)
          perror("uinput_send");
        else
          changed = 1;
      }
      if((state_now.mousebuttons & 0x1) && !(lastmb & 0x1)) {
        //fprintf(stderr, "Right button pressed.\n");
        if(uinput_send(uinput, EV_KEY, BTN_RIGHT, 1) < 0)
          perror("uinput_send");
        else
          changed = 1;
      }
      nowkey = sv2020_keyFromState(&state_now);
      if(nowkey != SV2020_KINVALID) {
        if(lastkey != nowkey) {
          if(lastkey != SV2020_KNONE) {
            //fprintf(stderr, "%s button released.\n", KEY_NAMES[lastkey]);
            if(uinput_send(uinput, EV_KEY, keycodes[lastkey - 1], 0) < 0)
              perror("uinput_send");
            else
              changed = 1;
          }
          if(nowkey != SV2020_KNONE) {
           //fprintf(stderr, "%s button pressed.\n", KEY_NAMES[nowkey]);
            if(uinput_send(uinput, EV_KEY, keycodes[nowkey - 1], 1) < 0)
              perror("uinput_send");
            else
              changed = 1;
          }
        }
      } else {
        fprintf(stderr, "Invalid key state: %05X", nowkey);
        if(uinput_send(uinput, EV_KEY, lastkey, 0) < 0)
          perror("uinput_send");
        else
          changed = 1;
      }

      if(changed == 1) {
        changed = 0;
        if(uinput_send(uinput, EV_SYN, SYN_REPORT, 0) < 0)
          perror("uinput_send");
      }
      
      lastkey = nowkey;
      lastmb = state_now.mousebuttons;
    } else { // Don't burn CPU.
      ts.tv_sec = 0;
      ts.tv_nsec = 1000000000 / (WORDSPERSECOND / PACKETSIZE); // max polling rate
      nanosleep(&ts, NULL);
    }
  }
  
  sv2020_close(sv2020);
  uinput_close(uinput);
  return(EXIT_FAILURE);
}

void cleanupandexit(int exitcode) {
  sv2020_close(sv2020);
  uinput_close(uinput);
  exit(exitcode);
}

void handler(int signal) {
  cleanupandexit(EXIT_SUCCESS);
}

int uinput_setup() {
  struct uinput_user_dev uidev;
  int fd;
  unsigned int i;
  
  fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if(fd < 0) {
    perror("open");
    return(-1);
  }

  if(ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0) {
    perror("ioctl");
    goto error0;
  }

  if(ioctl(fd, UI_SET_EVBIT, EV_REL) < 0) {
    perror("ioctl");
    goto error0;
  }
  if(ioctl(fd, UI_SET_RELBIT, REL_X) < 0) {
    perror("ioctl");
    goto error0;
  }
  if(ioctl(fd, UI_SET_RELBIT, REL_Y) < 0) {
    perror("ioctl");
    goto error0;
  }
  
  if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0) {
    perror("ioctl");
    goto error0;
  }
  for(i = 0; i < sizeof(keycodes) / sizeof(int); i++) {
    if(ioctl(fd, UI_SET_KEYBIT, keycodes[i]) < 0) {
      perror("ioctl");
      goto error0;
    }
  }
  if(ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) < 0) {
    perror("ioctl");
    goto error0;
  }
  if(ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT) < 0) {
    perror("ioctl");
    goto error0;
  }
  
  memset(&uidev, 0, sizeof(struct uinput_user_dev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Web.Remote SV2020");
  uidev.id.bustype = BUS_RS232;
  uidev.id.vendor  = 0x05FD; // InterAct, Inc.
  uidev.id.product = 0x2020; // Unused code that makes sense.
  uidev.id.version = 100;
  if(write(fd, &uidev, sizeof(struct uinput_user_dev)) < 0) {
    perror("write");
    goto error0;
  }

  if(ioctl(fd, UI_DEV_CREATE) < 0) {
    perror("ioctl");
    goto error0;
  }

  return(fd);

error0:
  close(fd);
  
  return(-1);
}

int uinput_send(int fd, int type, int code, int value) {
  struct input_event ev;
  
  memset(&ev, 0, sizeof(struct input_event));
  ev.type = type;
  ev.code = code;
  ev.value = value;
  
  return(write(fd, &ev, sizeof(struct input_event)));
}

void uinput_close(int fd) {
  if(fd > 0) {
    if(ioctl(fd, UI_DEV_DESTROY) < 0)
      perror("ioctl");

    close(fd);
  }
}
