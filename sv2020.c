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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "sv2020.h"

static struct termios oldtio;

extern const char const *KEY_NAMES[] = {
  "NONE",
  "1", "2", "3",
  "4", "5", "6",
  "7", "8", "9",
  "UP", "DOWN",
  "LEFT", "RIGHT",
  "PUP", "PDOWN",
  "CIRCLE", "SQUARE", "CROSS"
};

int sv2020_open(char *device) {
  int fd;
  struct termios tio = {
    .c_iflag = IGNPAR,
    .c_oflag = 0,
    .c_cflag = B2400 | CS7 | CSTOPB | CREAD | CLOCAL | CRTSCTS, // 2400 bps 7N2 hardware flow control
    .c_lflag = 0,
    .c_cc = {0},
  };

  fd = open(device, O_RDONLY | O_NOCTTY | O_NONBLOCK);
  if(fd < 0) {
    perror("open");
    return(-1);
  }
  
  if(tcgetattr(fd, &oldtio) < 0) {
    perror("tcgetattr");
    goto error0;
  }
 
  if(tcflush(fd, TCIOFLUSH) < 0) {
    perror("tcflush");
    goto error0;
  }
  
  if(tcsetattr(fd, TCSANOW, &tio) < 0) {
    perror("tcsetattr");
    goto error0;
  }
  
  return(fd);
  
error0:
  close(fd);
  
  return(-1);
}

void sv2020_close(int fd) {
  if(fd > 0) {
    if(tcsetattr(fd, TCSANOW, &oldtio) < 0) {
      fprintf(stderr, "WARNING: Couldn't restore serial port modes.");
      perror("tcsetattr");
    }
    
    close(fd);
  }
}

// Many attempts to avoid input lag were had.
int sv2020_getPacket(char *packet, int fd) {
  int bytesread;
  int packetread = 0;
  int sequence = 0;
  long perf = 0;
  long wait;
  struct timespec ts;
  
  for(;;) {
    bytesread = read(fd, &packet[packetread], PACKETSIZE - packetread);
    if(bytesread < 0) {
      perror("read");
      break;
    }
    
    if(bytesread == 0) {
      if(packetread == 0) {
        return(1); // Not ready
      } else { // We have some but not all, keep trying to make the entire packet
        if(sequence == 0) { // give it enough time to get the rest out
          ts.tv_sec = 0;
          wait = 1000000000 / (WORDSPERSECOND / (PACKETSIZE - packetread));
          ts.tv_nsec = wait;
          perf += wait;
          nanosleep(&ts, NULL);
          sequence++;
          continue;
        } else if(sequence <= 100) { // try a few more times without delays
          sequence++;
          continue;
        } else if(sequence < 110) { // try adding longer delays
          ts.tv_sec = 0;
          ts.tv_nsec = 10000000 * (sequence - 100); // 10 milliseconds increasing
          perf += ts.tv_nsec;
          nanosleep(&ts, NULL);
          sequence++;
          continue;
        } else {
          return(-2); // Short read
        }
      }
    } else {
      sequence = 0;
    }

    packetread += bytesread;
    if(packetread == 6) {
      //fprintf(stderr, "%ld\n", perf);
      return(0); // Success
    }
  }
  
  return(-1); // Error
}

void sv2020_stateFromPacket(sv2020_state *s, char *p) {
  // this code probably has problems
  s->rel_x = p[1] | ((p[0] >> 0 & 1) << 6);
  if((p[0] >> 1 & 1))
    s->rel_x = ~(128 - s->rel_x);
  s->rel_y = p[2] | ((p[0] >> 2 & 1) << 6);
  if((p[0] >> 3 & 1))
    s->rel_y = ~(128 - s->rel_y);
  s->mousebuttons = p[0] >> 4 & 3;
  s->remotebuttons = p[5] | (p[4] << 6) | (p[3] << 12);
}

sv2020_key sv2020_keyFromState(sv2020_state *s) {
  switch(s->remotebuttons) {
    case 0:
      return(SV2020_KNONE);
    case 0x00001:
      return(SV2020_K6);
    case 0x00002:
      return(SV2020_K5);
    case 0x00004:
      return(SV2020_K4);
    case 0x00008:
      return(SV2020_K9);
    case 0x00010:
      return(SV2020_K8);
    case 0x00020:
      return(SV2020_K7);
    case 0x00040:
      return(SV2020_KCROSS);
    case 0x00080:
      return(SV2020_KDOWN);
    case 0x00100:
      return(SV2020_KSQUARE);
    case 0x00200:
      return(SV2020_K3);
    case 0x00400:
      return(SV2020_K2);
    case 0x00800:
      return(SV2020_K1);
    case 0x01000:
      return(SV2020_KPDOWN);
    case 0x02000:
      return(SV2020_KUP);
    case 0x04000:
      return(SV2020_KPUP);
    case 0x08000:
      return(SV2020_KRIGHT);
    case 0x10000:
      return(SV2020_KCIRCLE);
    case 0x20000:
      return(SV2020_KLEFT);
    default:
      return(SV2020_KINVALID);
  }
}
