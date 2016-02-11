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

#define PACKETSIZE (6)

// no idea if this is correct, but it's a start
#define BAUDRATE (2400)
#define BITSPERWORD (9) // 7N2
#define WORDSPERSECOND (BAUDRATE / BITSPERWORD)

typedef enum { // order they appear in a packet
  SV2020_KNONE = 0,
  SV2020_K1,
  SV2020_K2,
  SV2020_K3,
  SV2020_K4,
  SV2020_K5,
  SV2020_K6,
  SV2020_K7,
  SV2020_K8,
  SV2020_K9,
  SV2020_KUP,
  SV2020_KDOWN,
  SV2020_KLEFT,
  SV2020_KRIGHT,
  SV2020_KPUP,
  SV2020_KPDOWN,
  SV2020_KCIRCLE,
  SV2020_KSQUARE,
  SV2020_KCROSS,
  SV2020_KINVALID
} sv2020_key;

const char const *KEY_NAMES[];

typedef struct {
  char mousebuttons; // both can be pressed
  int remotebuttons; // only one can be pressed
  int rel_x;
  int rel_y;
} sv2020_state;

int sv2020_open(char *device);
void sv2020_close(int fd);
int sv2020_getPacket(char *packet, int fd);
void sv2020_stateFromPacket(sv2020_state *s, char *p);
sv2020_key sv2020_keyFromState(sv2020_state *s);
