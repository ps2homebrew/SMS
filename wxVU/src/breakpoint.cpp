// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include <stdlib.h>
#include <stdio.h>
#include "breakpoint.h"
#include "vu.h"

/////////////////////////////// PUBLIC ///////////////////////////////////////

// this is ugly, but it will suffice until vu.cpp gets replaced
extern Vu VUchip;

Breakpoint* Breakpoint::_instance = 0;

Breakpoint* Breakpoint::Instance() {
    if(_instance == 0) {
        _instance = new Breakpoint;
    }
    return _instance;
}

bool
Breakpoint::check() {
    VuFloatReg *vf;
    m_cur = m_head;
    // while(m_cur) {
    //     if (m_cur->row == VUchip.PC) {
    //         switch(m_cur->type) {
    //             case BRK_INTREG:
    //                 if (VUchip.RegInt[m_cur->index].value() == m_cur->value_x) {
    //                     return true;
    //                 }
    //                 return false;
    //             case BRK_FLOATREG:
    //                 vf = &VUchip.RegFloat[m_cur->index];
    //                 if (vf->x() == m_cur->value_x &&
    //                     vf->y() == m_cur->value_y &&
    //                     vf->z() == m_cur->value_z &&
    //                     vf->w() == m_cur->value_w) {
    //                     return true;
    //                 }
    //                 return false;
    //             case BRK_MEMORY:
    //                 // if (VUchip.dataMem[m_cur->index].x == m_cur->value_x &&
    //                 //     VUchip.dataMem[m_cur->index].y == m_cur->value_y &&
    //                 //     VUchip.dataMem[m_cur->index].z == m_cur->value_z &&
    //                 //     VUchip.dataMem[m_cur->index].w == m_cur->value_w) {
    //                 //     return true;
    //                 // }
    //                 return false;
    //             default:
    //                 return true;
    //         }
    //     }
    //     m_cur = (bplist *)m_cur->next;
    // }
    return false;
}

void
Breakpoint::add(uint32 row, uint32 type, uint32 index, int32 value_x, int32
    value_y, int32 value_z, int32 value_w) {
    m_cur = new bplist;
    m_cur->row = row;
    switch(type) {
        case BRK_NONE:
            m_cur->type = type;
            m_cur->index = index;
            break;
        case BRK_INTREG:
            m_cur->type = type;
            m_cur->index = index;
            m_cur->value_x = value_x;
            break;
        case BRK_FLOATREG:
        case BRK_MEMORY:
            m_cur->type = type;
            m_cur->index = index;
            m_cur->type = type;
            m_cur->index = index;
            m_cur->value_x = value_x;
            m_cur->value_y = value_y;
            m_cur->value_z = value_z;
            m_cur->value_w = value_w;
            break;
        default:
            m_cur->type = BRK_NONE;
            m_cur->index = 0;
            break;
    }
    m_cur->next = m_head;
    m_head = m_cur;
}

void
Breakpoint::list(void) {
    m_cur = m_head;
    while(m_cur) {
        printf("row: %d, type: %d, index: %d, x: %d\n", m_cur->row, m_cur->type,
            m_cur->index, m_cur->value_x);
        m_cur = (bplist *)m_cur->next;
    }
}

void
Breakpoint::remove() {
}

/////////////////////////////// PRIVATE ///////////////////////////////////////
Breakpoint::Breakpoint() {
    m_cur = NULL;
    m_head = NULL;
}

Breakpoint::~Breakpoint() {
}
