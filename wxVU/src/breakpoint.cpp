// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include <stdlib.h>
#include <stdio.h>
#include "breakpoint.h"
#include "vu.h"

// this is ugly, but it will suffice until vu.cpp gets replaced
extern VU VUchip;

Breakpoint *Breakpoint::_instance = 0;

Breakpoint *Breakpoint::Instance() {
    if(_instance == 0) {
        _instance = new Breakpoint;
    }
    return _instance;
}

Breakpoint::Breakpoint() {
    cur = NULL;
    head = NULL;
}

Breakpoint::~Breakpoint() {
}

bool
Breakpoint::check() {
    VFReg *vf;
    cur = head;
    while(cur) {
        if (cur->row == VUchip.PC) {
            switch(cur->type) {
                case BRK_INTREG:
                    if (VUchip.RegInt[cur->index].value() == cur->value_x) {
                        return true;
                    }
                    return false;
                case BRK_FLOATREG:
                    vf = &VUchip.RegFloat[cur->index];
                    if (vf->x() == cur->value_x &&
                        vf->y() == cur->value_y &&
                        vf->z() == cur->value_z &&
                        vf->w() == cur->value_w) {
                        return true;
                    }
                    return false;
                case BRK_MEMORY:
                    if (VUchip.dataMem[cur->index].x == cur->value_x &&
                        VUchip.dataMem[cur->index].y == cur->value_y &&
                        VUchip.dataMem[cur->index].z == cur->value_z &&
                        VUchip.dataMem[cur->index].w == cur->value_w) {
                        return true;
                    }
                    return false;
                default:
                    return true;
            }
        }
        cur = (bplist *)cur->next;
    }
    return false;
}

void
Breakpoint::add(uint32 row, uint32 type, uint32 index, int32 value_x, int32
    value_y, int32 value_z, int32 value_w) {
    cur = new bplist;
    cur->row = row;
    switch(type) {
        case BRK_NONE:
            cur->type = type;
            cur->index = index;
            break;
        case BRK_INTREG:
            cur->type = type;
            cur->index = index;
            cur->value_x = value_x;
            break;
        case BRK_FLOATREG:
        case BRK_MEMORY:
            cur->type = type;
            cur->index = index;
            cur->type = type;
            cur->index = index;
            cur->value_x = value_x;
            cur->value_y = value_y;
            cur->value_z = value_z;
            cur->value_w = value_w;
            break;
        default:
            cur->type = BRK_NONE;
            cur->index = 0;
            break;
    }
    cur->next = head;
    head = cur;
}

void
Breakpoint::list(void) {
    cur = head;
    while(cur) {
        printf("row: %d, type: %d, index: %d, x: %d\n", cur->row, cur->type,
            cur->index, cur->value_x);
        cur = (bplist *)cur->next;
    }
}

void
Breakpoint::remove() {
}
