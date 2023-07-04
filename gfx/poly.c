/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 * Modified 2023 for Sunela by Werner Almesberger
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * Graphics Draw Functions
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <assert.h>

#include "gfx.h"


// Fill poly - each member of vertices is 1/16th pixel
void gfx_poly(struct gfx_drawable *da, int points, const short *vertices,
    gfx_color color) {
  typedef struct {
    short x,y;
  } VertXY;
  VertXY *v = (VertXY*)vertices;

  int i,j,y;
  int miny = (int)(da->h-1);
  int maxy = 0;
  for (i=0;i<points;i++) {
    assert(v[i].x < (int) da->w);
    // work out min and max
    y = v[i].y;
    if (y<miny) miny=y;
    if (y>maxy) maxy=y;
  }
  assert(maxy < (int) da->h);

  const int MAX_CROSSES = 64;

  // for each scanline
  for (y=miny;y<=maxy;y++) {
    int yl = y;
    short cross[MAX_CROSSES];
    bool slopes[MAX_CROSSES];
    int crosscnt = 0;
    // work out all the times lines cross the scanline
    j = points-1;
    for (i=0;i<points;i++) {
      if ((v[i].y<=y && v[j].y>y) || (v[j].y<=y && v[i].y>y)) {
        if (crosscnt < MAX_CROSSES) {
          int l = v[j].y - v[i].y;
          if (l) { // don't do horiz lines - rely on the ends of the lines that join onto them
            cross[crosscnt] = (short)(v[i].x + ((y - v[i].y) * (v[j].x-v[i].x)) / l);
            slopes[crosscnt] = (l>1)?1:0;
            crosscnt++;
          }
        }
      }
      j = i;
    }

    // bubble sort
    for (i=0;i<crosscnt-1;) {
      if (cross[i]>cross[i+1]) {
        short t=cross[i];
        cross[i]=cross[i+1];
        cross[i+1]=t;
        bool ts=slopes[i];
        slopes[i]=slopes[i+1];
        slopes[i+1]=ts;
        if (i) i--;
      } else i++;
    }

    //  Fill the pixels between node pairs.
    int x = 0,s = 0;
    for (i=0;i<crosscnt;i++) {
      if (s==0) x=cross[i];
      if (slopes[i]) s++; else s--;
      if (!s || i==crosscnt-1) {
        int x1 = x;
        int x2 = cross[i];
        if (x2>x1) gfx_rect_xy(da, x1, yl, x2 - x1 - 1, 1, color);
      }
    }
  }
}
