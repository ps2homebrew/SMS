#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include "vu.h"

extern MicroCode Instr;

//All instructions functions can return 3 values
//0  all ok
//1 - 3 stall on param #
//4 warning P used before fixed (use waitp)
//5 warning q used before fixed (use waitq)
int VU::VU_ABS(VUInstruction &A)
{
    int st1;
    char dst[50];

    st1=Stalling(A.Params[0][1]);

    //ths instruction can not return a warning
    if(st1>0)
        return 2;

    RegFloat[A.Params[0][1].index].lastRead(PC);
    RegFloat[A.Params[0][0].index].lastWrite(PC);
    RegFloat[A.Params[0][0].index].stall(4);
    if ( A.Params[0][0].index == 0 ) {
        return 0;
    }

    if (A.dest[0][0])
        strcpy(dst,A.dest[0]);
    else
        strcpy(dst,"XYZW");
    if(strstr(dst,"X"))
        RegFloat[A.Params[0][0].index].x(fabs(RegFloat[A.Params[0][1].index].x()));
    if(strstr(dst,"Y"))
        RegFloat[A.Params[0][0].index].y(fabs(RegFloat[A.Params[0][1].index].y()));
    if(strstr(dst,"Z"))
        RegFloat[A.Params[0][0].index].z(fabs(RegFloat[A.Params[0][1].index].z()));
    if(strstr(dst,"W"))
        RegFloat[A.Params[0][0].index].w(fabs(RegFloat[A.Params[0][1].index].w()));

    return 0;
}

int
VU::VU_ADD(VUInstruction &A) {
    int st1,st2;
    char dst[50];
    VFReg *p1, *p2, *p3;
    float x,y,z,w;

    st1 = Stalling(A.Params[0][1]);
    st2 = Stalling(A.Params[0][2]);

    if(st1>0)
        return (st1==1)?2:st1;
    if(st2>0)
        return (st2==1)?3:st2;

    if (A.dest[0][0]) {
        strcpy(dst, A.dest[0]);
    } else {
        strcpy(dst,"XYZW");
    }

    switch(A.flavor[0]) {
        case 0:
        case 2:
        case 3:
        case 4:
        case 5:
            p1 =& (RegFloat[A.Params[0][0].index]);
            p2 =& (RegFloat[A.Params[0][1].index]);
            p3 =& (RegFloat[A.Params[0][2].index]);
            break;
        case 1:
            p1 =& (RegFloat[A.Params[0][0].index]);
            p2 =& (RegFloat[A.Params[0][1].index]);
            p3 =& I;
            break;
        case 6:
            p1 =& (RegFloat[A.Params[0][0].index]);
            p2 =& (RegFloat[A.Params[0][1].index]);
            p3 =& Q;
            break;
        case 7:
        case 10:
        case 11:
        case 12:
        case 13:
            p1 = &ACC;
            p2 = &(RegFloat[A.Params[0][1].index]);
            p3 = &(RegFloat[A.Params[0][2].index]);
            break;
        case 8:
            p1 = &ACC;
            p2 = &(RegFloat[A.Params[0][1].index]);
            p3 = &I;
            break;
        case 9:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&Q;
            break;
    }

    p1->lastWrite(PC);
    p2->lastRead(PC);
    p3->lastRead(PC);
    p1->stall(4);
    if ( A.Params[0][0].index == 0 && A.flavor[0] < 7) {
        return 0;
    }
    switch(A.flavor[0]) {
        //this switch set p2 to broadcast or normal
        case 0:
        case 1:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 6:
        case 7:
        case 8:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 9:
            x=p3->x();
            y=p3->y();
            z=p3->z();
            w=p3->w();
            break;
        case 2:
        case 10:
            x=p3->x();
            y=p3->x();
            z=p3->x();
            w=p3->x();
            break;
        case 3:
        case 11:
            x=p3->y();
            y=p3->y();
            z=p3->y();
            w=p3->y();
            break;
        case 4:
        case 12:
            x=p3->z();
            y=p3->z();
            z=p3->z();
            w=p3->z();
            break;
        case 5:
        case 13:
            x=p3->w();
            y=p3->w();
            z=p3->w();
            w=p3->w();
            break;
    }

    if(strstr(dst,"X"))
        p1->x(p2->x()+x);
    if(strstr(dst,"Y"))
        p1->y(p2->y()+y);
    if(strstr(dst,"Z"))
        p1->z(p2->z()+z);
    if(strstr(dst,"W"))
        p1->w(p2->w()+w);

    return 0;
}

int VU::VU_CLIPW(VUInstruction &A)
{
    int st0, st1;
    char dst[50];

    st0 = Stalling(A.Params[0][0]);
    st1 = Stalling(A.Params[0][1]);

    //this instruction can not return a warning
    if(st0>0)
        return 1;
    if(st1>0)
        return 2;

    //this instruction allways get "XYZ" as dest
    strcpy(dst,"XYZ");
    ClipFlag[3]=ClipFlag[2];
    ClipFlag[2]=ClipFlag[1];
    ClipFlag[1]=ClipFlag[0];

    ClipFlag[0]=0;
    if(RegFloat[A.Params[0][0].index].x()>fabs(RegFloat[A.Params[0][1].index].w()))
        ClipFlag[0]+=(uint16)1; //set +x
    if(RegFloat[A.Params[0][0].index].x()<-fabs(RegFloat[A.Params[0][1].index].w()))
        ClipFlag[0]+=(uint16)2; //set -x
    if(RegFloat[A.Params[0][0].index].y()>fabs(RegFloat[A.Params[0][1].index].w()))
        ClipFlag[0]+=(uint16)4; //set +y
    if(RegFloat[A.Params[0][0].index].y()<-fabs(RegFloat[A.Params[0][1].index].w()))
        ClipFlag[0]+=(uint16)8; //set -y
    if(RegFloat[A.Params[0][0].index].z()>fabs(RegFloat[A.Params[0][1].index].w()))
        ClipFlag[0]+=(uint16)16; //set +z
    if(RegFloat[A.Params[0][0].index].z()<-fabs(RegFloat[A.Params[0][1].index].w()))
        ClipFlag[0]+=(uint16)32; //set -z

    RegFloat[A.Params[0][1].index].lastRead(PC);
    RegFloat[A.Params[0][0].index].lastRead(PC);
    return 0;
}

int
VU::VU_FTOI(VUInstruction &A, int point) {
    int st1;
    char dst[50];


    st1 = Stalling(A.Params[0][1]);

    //ths instruction can not return a warning
    if(st1>0)
        return 2;

    RegFloat[A.Params[0][1].index].lastRead(PC);
    RegFloat[A.Params[0][0].index].lastWrite(PC);
    RegFloat[A.Params[0][0].index].stall(4);

    if ( A.Params[0][0].index == 0 ) {
        return 0;
    }
    if (A.dest[0][0])
        strcpy(dst,A.dest[0]);
    else
        strcpy(dst,"XYZW");
    if(strstr(dst,"X"))
        RegFloat[A.Params[0][0].index].xtoi(RegFloat[A.Params[0][1].index].x(),point);
    if(strstr(dst,"Y"))
        RegFloat[A.Params[0][0].index].ytoi(RegFloat[A.Params[0][1].index].y(),point);
    if(strstr(dst,"Z"))
        RegFloat[A.Params[0][0].index].ztoi(RegFloat[A.Params[0][1].index].z(),point);
    if(strstr(dst,"W"))
        RegFloat[A.Params[0][0].index].wtoi(RegFloat[A.Params[0][1].index].w(),point);
    return 0;
}

int VU::VU_ITOF(VUInstruction &A, int point)
{
    int st1, dx, dy, dz, dw;
    char dst[50];
    st1 = Stalling(A.Params[0][1]);

    if(st1>0)
        return 2;

    RegFloat[A.Params[0][1].index].lastRead(PC);
    RegFloat[A.Params[0][0].index].lastWrite(PC);
    RegFloat[A.Params[0][0].index].stall(4);
    if ( A.Params[0][0].index == 0 ) {
        return 0;
    }
    if (A.dest[0][0])
        strcpy(dst,A.dest[0]);
    else
        strcpy(dst,"XYZW");

    RegFloat[A.Params[0][1].index].mcopy(&dx,0);
    RegFloat[A.Params[0][1].index].mcopy(&dy,1);
    RegFloat[A.Params[0][1].index].mcopy(&dz,2);
    RegFloat[A.Params[0][1].index].mcopy(&dw,3);

    if(strstr(dst,"X"))
        RegFloat[A.Params[0][0].index].xtof(dx,point);
    if(strstr(dst,"Y"))
        RegFloat[A.Params[0][0].index].ytof(dy,point);
    if(strstr(dst,"Z"))
        RegFloat[A.Params[0][0].index].ztof(dz,point);
    if(strstr(dst,"W"))
        RegFloat[A.Params[0][0].index].wtof(dw,point);
    return 0;
}

int VU::VU_MADD(VUInstruction &A)
{
    int st1,st2;
    char dst[50];
    VFReg *p1, *p2, *p3;
    float x,y,z,w;

    st1=Stalling(A.Params[0][1]);
    st2=Stalling(A.Params[0][2]);

    if(st1>0)
        return (st1==1)?2:st1;
    if(st2>0)
        return (st2==1)?3:st2;

    if (A.dest[0][0])
        strcpy(dst,A.dest[0]);
    else
        strcpy(dst,"XYZW");

    switch(A.flavor[0]) {
        //this switch sets p1, p2, p3 to the right registers
        case 0:
        case 2:
        case 3:
        case 4:
        case 5:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&(RegFloat[A.Params[0][2].index]);
            break;
        case 1:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&I;
            break;
        case 6:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&Q;
            break;
        case 7:
        case 10:
        case 11:
        case 12:
        case 13:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&(RegFloat[A.Params[0][2].index]);
            break;
        case 8:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&I;
            break;
        case 9:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&Q;
            break;
    }

    p1->lastWrite(PC);
    ACC.lastRead(PC);
    p2->lastRead(PC);
    p3->lastRead(PC);
    p1->stall(4);
    if ( A.Params[0][0].index == 0 && A.flavor[0] < 7) {
        return 0;
    }

    switch(A.flavor[0]) {
        //this switch set p2 to broadcast or normal
        case 0:
        case 1:
        case 6:
        case 7:
        case 8:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 9:
            x=p3->x();
            y=p3->y();
            z=p3->z();
            w=p3->w();
            break;
        case 2:
        case 10:
            x=p3->x();
            y=p3->x();
            z=p3->x();
            w=p3->x();
            break;
        case 3:
        case 11:
            x=p3->y();
            y=p3->y();
            z=p3->y();
            w=p3->y();
            break;
        case 4:
        case 12:
            x=p3->z();
            y=p3->z();
            z=p3->z();
            w=p3->z();
            break;
        case 5:
        case 13:
            x=p3->w();
            y=p3->w();
            z=p3->w();
            w=p3->w();
            break;
    }

    if(strstr(dst,"X"))
        p1->x(ACC.x()+(p2->x()*x) );
    if(strstr(dst,"Y"))
        p1->y(ACC.y()+(p2->y()*y) );
    if(strstr(dst,"Z"))
        p1->z(ACC.z()+(p2->z()*z) );
    if(strstr(dst,"W"))
        p1->w(ACC.w()+(p2->w()*w) );
    return 0;
}

int VU::VU_MAX(VUInstruction &A)
{
    int st1,st2;
    char dst[50];
    VFReg *p1, *p2, *p3;
    float x,y,z,w;

    st1=Stalling(A.Params[0][1]);
    st2=Stalling(A.Params[0][2]);

    //ths instruction can not return a warning
    if(st1>0)
        return 2;
    if(st2>0)
        return 3;

    if (A.dest[0][0])
        strcpy(dst,A.dest[0]);
    else
        strcpy(dst,"XYZW");

    switch(A.flavor[0]) {
        //this switch sets p1, p2, p3 to the right registers
        case 0:
        case 2:
        case 3:
        case 4:
        case 5:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&(RegFloat[A.Params[0][2].index]);
            break;
        case 1:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&I;
            break;
    }

    p1->lastWrite(PC);
    p2->lastRead(PC);
    p3->lastRead(PC);
    p1->stall(4);
    if ( A.Params[0][0].index == 0 ) {
        return 0;
    }

    switch(A.flavor[0]) {
        //this switch set p2 to broadcast or normal
        case 0:
        case 1:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            x=p3->x();
            y=p3->y();
            z=p3->z();
            w=p3->w();
            break;
        case 2:
        case 10:
            x=p3->x();
            y=p3->x();
            z=p3->x();
            w=p3->x();
            break;
        case 3:
        case 11:
            x=p3->y();
            y=p3->y();
            z=p3->y();
            w=p3->y();
            break;
        case 4:
        case 12:
            x=p3->z();
            y=p3->z();
            z=p3->z();
            w=p3->z();
            break;
        case 5:
        case 13:
            x=p3->w();
            y=p3->w();
            z=p3->w();
            w=p3->w();
            break;
    }

    if(strstr(dst,"X")) {
        if(p2->x()>x)
            p1->x(p2->x());
        else
            p1->x(x);
    }

    if(strstr(dst,"Y")){
        if(p2->y()>y)
            p1->y(p2->y());
        else
            p1->y(y);
    }

    if(strstr(dst,"Z")){
        if(p2->z()>z)
            p1->z(p2->z());
        else
            p1->z(z);
    }

    if(strstr(dst,"W")){
        if(p2->w()>w)
            p1->w(p2->w());
        else
            p1->w(w);
    }
    return 0;
}

int VU::VU_MIN(VUInstruction &A)
{
    int st1,st2;
    char dst[50];
    VFReg *p1, *p2, *p3;
    float x,y,z,w;

    st1 = Stalling(A.Params[0][1]);
    st2 = Stalling(A.Params[0][2]);

    //ths instruction can not return a warning
    if(st1>0)
        return 2;
    if(st2>0)
        return 3;

    if (A.dest[0][0])
        strcpy(dst,A.dest[0]);
    else
        strcpy(dst,"XYZW");

    switch(A.flavor[0]) {
        //this switch sets p1, p2, p3 to the right registers
        case 0:
        case 2:
        case 3:
        case 4:
        case 5:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&(RegFloat[A.Params[0][2].index]);
            break;
        case 1:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&I;
            break;
    }

    if (A.Params[0][0].index == 0) {
        return 0;
    }

    p1->lastWrite(PC);
    p2->lastRead(PC);
    p3->lastRead(PC);
    p1->stall(4);

    switch(A.flavor[0]) {
        //this switch set p2 to broadcast or normal
        case 0:
        case 1:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            x=p3->x();
            y=p3->y();
            z=p3->z();
            w=p3->w();
            break;
        case 2:
        case 10:
            x=p3->x();
            y=p3->x();
            z=p3->x();
            w=p3->x();
            break;
        case 3:
        case 11:
            x=p3->y();
            y=p3->y();
            z=p3->y();
            w=p3->y();
            break;
        case 4:
        case 12:
            x=p3->z();
            y=p3->z();
            z=p3->z();
            w=p3->z();
            break;
        case 5:
        case 13:
            x=p3->w();
            y=p3->w();
            z=p3->w();
            w=p3->w();
            break;
    }

    if(strstr(dst,"X")) {
        if(p2->x()<x)
            p1->x(p2->x());
        else
            p1->x(x);
    }

    if(strstr(dst,"Y")){
        if(p2->y()<y)
            p1->y(p2->y());
        else
            p1->y(y);
    }

    if(strstr(dst,"Z")){
        if(p2->z()<z)
            p1->z(p2->z());
        else
            p1->z(z);
    }

    if(strstr(dst,"W")){
        if(p2->w()<w)
            p1->w(p2->w());
        else
            p1->w(w);
    }
    return 0;
}


int VU::VU_MSUB(VUInstruction &A)
{
    int st1,st2;
    char dst[50];
    VFReg *p1, *p2, *p3;
    float x,y,z,w;

    st1=Stalling(A.Params[0][1]);
    st2=Stalling(A.Params[0][2]);

    if(st1>0)
        return (st1==1)?2:st1;
    if(st2>0)
        return (st2==1)?3:st2;

    if (A.dest[0][0])
        strcpy(dst,A.dest[0]);
    else
        strcpy(dst,"XYZW");

    switch(A.flavor[0]) {
        //this switch sets p1, p2, p3 to the right registers
        case 0:
        case 2:
        case 3:
        case 4:
        case 5:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&(RegFloat[A.Params[0][2].index]);
            break;
        case 1:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&I;
            break;
        case 6:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&Q;
            break;
        case 7:
        case 10:
        case 11:
        case 12:
        case 13:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&(RegFloat[A.Params[0][2].index]);
            break;
        case 8:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&I;
            break;
        case 9:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&Q;
            break;
    }

    p1->lastWrite(PC);
    ACC.lastRead(PC);
    p2->lastRead(PC);
    p3->lastRead(PC);
    p1->stall(4);
    if ( A.Params[0][0].index == 0 && A.flavor[0] < 7) {
        return 0;
    }

    switch(A.flavor[0]) {
        //this switch set p2 to broadcast or normal
        case 0:
        case 1:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 6:
        case 7:
        case 8:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 9:
            x=p3->x();
            y=p3->y();
            z=p3->z();
            w=p3->w();
            break;
        case 2:
        case 10:
            x=p3->x();
            y=p3->x();
            z=p3->x();
            w=p3->x();
            break;
        case 3:
        case 11:
            x=p3->y();
            y=p3->y();
            z=p3->y();
            w=p3->y();
            break;
        case 4:
        case 12:
            x=p3->z();
            y=p3->z();
            z=p3->z();
            w=p3->z();
            break;
        case 5:
        case 13:
            x=p3->w();
            y=p3->w();
            z=p3->w();
            w=p3->w();
            break;
    }

    if(strstr(dst,"X"))
        p1->x(ACC.x()-(p2->x()*x) );
    if(strstr(dst,"Y"))
        p1->y(ACC.y()-(p2->y()*y) );
    if(strstr(dst,"Z"))
        p1->z(ACC.z()-(p2->z()*z) );
    if(strstr(dst,"W"))
        p1->w(ACC.w()-(p2->w()*w) );
    return 0;
}

int VU::VU_MUL(VUInstruction &A)
{
    int st1,st2;
    char dst[50];
    VFReg *p1, *p2, *p3;
    float x,y,z,w;

    st1 = Stalling(A.Params[0][1]);
    st2 = Stalling(A.Params[0][2]);

    if(st1>0)
        return (st1==1)?2:st1;
    if(st2>0)
        return (st2==1)?3:st2;

    if (A.dest[0][0])
        strcpy(dst,A.dest[0]);
    else
        strcpy(dst,"XYZW");

    switch(A.flavor[0]) {
        //this switch sets p1, p2, p3 to the right registers
        case 0:
        case 2:
        case 3:
        case 4:
        case 5:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&(RegFloat[A.Params[0][2].index]);
            break;
        case 1:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&I;
            break;
        case 6:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&Q;
            break;
        case 7:
        case 10:
        case 11:
        case 12:
        case 13:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&(RegFloat[A.Params[0][2].index]);
            break;
        case 8:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&I;
            break;
        case 9:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&Q;
            break;
    }

    p1->lastWrite(PC);
    p2->lastRead(PC);
    p3->lastRead(PC);
    p1->stall(4);
    if ( A.Params[0][0].index == 0 && A.flavor[0] < 7) {
        return 0;
    }
    switch(A.flavor[0]) {
        //this switch set p2 to broadcast or normal
        case 0:
            x = p3->x();
            y = p3->y();
            z = p3->z();
            w = p3->w();
			break;
        case 1:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 6:
        case 7:
            x = p3->x();
            y = p3->y();
            z = p3->z();
            w = p3->w();
			break;
        case 8:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 9:
            x=p3->x();
            y=p3->y();
            z=p3->z();
            w=p3->w();
            break;
        case 2:
        case 10:
            x=p3->x();
            y=p3->x();
            z=p3->x();
            w=p3->x();
            break;
        case 3:
        case 11:
            x=p3->y();
            y=p3->y();
            z=p3->y();
            w=p3->y();
            break;
        case 4:
        case 12:
            x=p3->z();
            y=p3->z();
            z=p3->z();
            w=p3->z();
            break;
        case 5:
        case 13:
            x=p3->w();
            y=p3->w();
            z=p3->w();
            w=p3->w();
            break;
    }

    if(strstr(dst,"X"))
        p1->x(p2->x()*x);
    if(strstr(dst,"Y"))
        p1->y(p2->y()*y);
    if(strstr(dst,"Z"))
        p1->z(p2->z()*z);
    if(strstr(dst,"W"))
        p1->w(p2->w()*w);
    return 0;
}

int
VU::VU_NOP(VUInstruction &A) {
    return 0;
}

int
VU::VU_OPMULA(VUInstruction &A) {
    int st1, st2;
    char dst[50];

    st1 = Stalling(A.Params[0][1]);
    st2 = Stalling(A.Params[0][2]);

    //ths instruction can not return a warning
    if(st1>0) {
        return 2;
    }
    if(st2>0) {
        return 3;
    }

    if (A.dest[0][0]) {
        strcpy(dst,A.dest[0]);
    } else {
        strcpy(dst,"XYZW");
    }

    if(strstr(dst,"X")) {
        ACC.x(RegFloat[A.Params[0][1].index].y() * RegFloat[A.Params[0][2].index].z());
    }
    if(strstr(dst,"Y")) {
        ACC.y(RegFloat[A.Params[0][1].index].z() * RegFloat[A.Params[0][2].index].x());
    }
    if(strstr(dst,"Z")) {
        ACC.z(RegFloat[A.Params[0][1].index].x() * RegFloat[A.Params[0][2].index].y());
    }

    RegFloat[A.Params[0][1].index].lastRead(PC);
    RegFloat[A.Params[0][2].index].lastRead(PC);
    RegFloat[A.Params[0][0].index].lastWrite(PC);
    RegFloat[A.Params[0][0].index].stall(4);
    return 0;
}

int
VU::VU_OPMSUB(VUInstruction &A) {
    int st1, st2, st3;
    char dst[50];

    st1=Stalling(A.Params[0][1]);
    st2=Stalling(A.Params[0][2]);
    st3=ACC.stall();

    //ths instruction can not return a warning
    if(st1>0)
        return 2;
    if(st2>0)
        return 3;

    ACC.lastRead(PC);
    RegFloat[A.Params[0][1].index].lastRead(PC);
    RegFloat[A.Params[0][2].index].lastRead(PC);
    RegFloat[A.Params[0][0].index].lastWrite(PC);
    RegFloat[A.Params[0][0].index].stall(4);
    if (A.Params[0][0].index == 0) {
        return 0;
    }

    if (A.dest[0][0])
        strcpy(dst,A.dest[0]);
    else
        strcpy(dst,"XYZW");
    if(strstr(dst,"X"))
        RegFloat[A.Params[0][0].index].x(ACC.x()-(RegFloat[A.Params[0][1].index].y() * RegFloat[A.Params[0][2].index].z()));
    if(strstr(dst,"Y"))
        RegFloat[A.Params[0][0].index].y(ACC.y()-(RegFloat[A.Params[0][1].index].z() * RegFloat[A.Params[0][2].index].x()));
    if(strstr(dst,"Z"))
        RegFloat[A.Params[0][0].index].z(ACC.z()-(RegFloat[A.Params[0][1].index].x() * RegFloat[A.Params[0][2].index].y()));
    return 0;
}

int VU::VU_SUB(VUInstruction &A)
{
    int st1,st2;
    char dst[50];
    VFReg *p1, *p2, *p3;
    float x,y,z,w;

    st1 = Stalling(A.Params[0][1]);
    st2 = Stalling(A.Params[0][2]);

    if(st1>0)
        return (st1==1)?2:st1;
    if(st2>0)
        return (st2==1)?3:st2;

    if (A.dest[0][0])
        strcpy(dst,A.dest[0]);
    else
        strcpy(dst,"XYZW");

    switch(A.flavor[0]) {
        case 0:
        case 2:
        case 3:
        case 4:
        case 5:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&(RegFloat[A.Params[0][2].index]);
            break;
        case 1:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&I;
            break;
        case 6:
            p1=&(RegFloat[A.Params[0][0].index]);
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&Q;
            break;
        case 7:
        case 10:
        case 11:
        case 12:
        case 13:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&(RegFloat[A.Params[0][2].index]);
            break;
        case 8:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&I;
            break;
        case 9:
            p1=&ACC;
            p2=&(RegFloat[A.Params[0][1].index]);
            p3=&Q;
            break;
    }

    p1->lastWrite(PC);
    p2->lastRead(PC);
    p3->lastRead(PC);
    p1->stall(4);
    if ( A.Params[0][0].index == 0 && A.flavor[0] < 7) {
        return 0;
    }

    switch(A.flavor[0]) {
        case 0:
            x = p3->x();
            y = p3->y();
            z = p3->z();
            w = p3->w();
			break;
        case 1:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 6:
        case 7:
        case 8:
            x = p3->x();
            y = p3->x();
            z = p3->x();
            w = p3->x();
            break;
        case 9:
            x=p3->x();
            y=p3->y();
            z=p3->z();
            w=p3->w();
            break;
        case 2:
        case 10:
            x=p3->x();
            y=p3->x();
            z=p3->x();
            w=p3->x();
            break;
        case 3:
        case 11:
            x=p3->y();
            y=p3->y();
            z=p3->y();
            w=p3->y();
            break;
        case 4:
        case 12:
            x=p3->z();
            y=p3->z();
            z=p3->z();
            w=p3->z();
            break;
        case 5:
        case 13:
            x=p3->w();
            y=p3->w();
            z=p3->w();
            w=p3->w();
            break;
    }

    if(strstr(dst,"X"))
        p1->x(p2->x()-x);
    if(strstr(dst,"Y"))
        p1->y(p2->y()-y);
    if(strstr(dst,"Z"))
        p1->z(p2->z()-z);
    if(strstr(dst,"W"))
        p1->w(p2->w()-w);
    return 0;
}
