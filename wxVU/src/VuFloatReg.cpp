// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
#include <string.h>
#include "VuFloatReg.h"

float VuFloatReg::x(){ return data.vf.x;}
float VuFloatReg::y(){ return data.vf.y;}
float VuFloatReg::z(){ return data.vf.z;}
float VuFloatReg::w(){ return data.vf.w;}
// write val to register
void VuFloatReg::set(float a){
    data.vf.x = a;
    data.vf.y = a;
    data.vf.z = a;
    data.vf.w = a;}
void VuFloatReg::x(float a){
    data.vf.x=a;
}
void VuFloatReg::y(float a){
    data.vf.y=a;
}
void VuFloatReg::z(float a){
    data.vf.z=a;
}
void VuFloatReg::w(float a){
    data.vf.w=a;
}
void VuFloatReg::mcopy(int *dest, int reg)
{
    switch(reg) {
        case 0:
            memcpy((void *)dest, (void *)&(data.vf.x), 4);
            break;
        case 1:
            memcpy((void *)dest, (void *)&(data.vf.y), 4);
            break;
        case 2:
            memcpy((void *)dest, (void *)&(data.vf.z), 4);
            break;
        case 3:
            memcpy((void *)dest, (void *)&(data.vf.w), 4);
            break;
    }
}

void VuFloatReg::mwrite(int *org, int reg)
{
    switch(reg) {
        case 0:
            memcpy( (void *)&(data.vf.x), (void *)org, 4);
            break;
        case 1:
            memcpy( (void *)&(data.vf.y), (void *)org, 4);
            break;
        case 2:
            memcpy( (void *)&(data.vf.z), (void *)org, 4);
            break;
        case 3:
            memcpy( (void *)&(data.vf.w), (void *)org, 4);
            break;
    }
}

void VuFloatReg::xtoi(float v, int a)
{
    int b,i;
    long double c,d;
    b=(int)v; //integer part
    c=(long double)(v-(float)b); //decimal part
    switch (a) {
        case 0:
            memcpy((void *)&(data.vf.x),(void *)&b, 4);
            break;
        case 4:
            d=1.0/16.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.x),b,i,4);
            break;
        case 12:
            d=1.0/4096.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.x),b,i,12);
            break;
        case 15:
            d=1.0/32768.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.x),b,i,15);
            break;
    }
}

void VuFloatReg::ytoi(float v, int a)
{
    int b,i;
    long double c,d;
    b=(int)v; //integer part
    c=(long double)(v-(float)b); //decimal part
    switch (a) {
        case 0:
            memcpy((void *)&(data.vf.y),(void *)&b, 4);
            break;
        case 4:
            d=1.0/16.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.y),b,i,4);
            break;
        case 12:
            d=1.0/4096.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.y),b,i,12);
            break;
        case 15:
            d=1.0/32768.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.y),b,i,15);
            break;
    }
}

void VuFloatReg::ztoi(float v, int a)
{
    int b,i;
    long double c,d;
    b=(int)v; //integer part
    c=(long double)(v-(float)b); //decimal part
    switch (a) {
        case 0:
            memcpy((void *)&(data.vf.z),(void *)&b, 4);
            break;
        case 4:
            d=1.0/16.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.z),b,i,4);
            break;
        case 12:
            d=1.0/4096.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.z),b,i,12);
            break;
        case 15:
            d=1.0/32768.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.z),b,i,15);
            break;
    }
}

void VuFloatReg::wtoi(float v, int a)
{
    int b,i;
    long double c,d;
    b=(int)v; //integer part
    c=(long double)(v-(float)b); //decimal part
    switch (a) {
        case 0:
            memcpy((void *)&(data.vf.w),(void *)&b, 4);
            break;
        case 4:
            d=1.0/16.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.w),b,i,4);
            break;
        case 12:
            d=1.0/4096.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.w),b,i,12);
            break;
        case 15:
            d=1.0/32768.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.w),b,i,15);
            break;
    }
}


void VuFloatReg::xtof(int v, int a)
{
    int b;
    float e;
    memcpy((void *)&b,(void *)&v, 4);

    switch (a) {
        case 0:
            data.vf.x=(float)b;
            break;
        case 4:
            e=(float)b/16.0;
            data.vf.x=e; //d+e;
            break;
        case 12:
            e=(float)b/4096.0;
            data.vf.x=e; //d+e;
            break;
        case 15:
            e=(float)b/32768.0;
            data.vf.x=e; //d+e;
            break;
    }
}

void VuFloatReg::ytof(int v, int a)
{
    int b;
    float e;
    memcpy((void *)&b,(void *)&v, 4);

    switch (a) {
        case 0:
            data.vf.y=(float)b;
            break;
        case 4:
            e=(float)b/16.0;
            data.vf.y=e;
            break;
        case 12:
            e=(float)b/4096.0;
            data.vf.y=e;
            break;
        case 15:
            e=(float)b/32768.0;
            data.vf.y=e;
            break;
    }
}

void VuFloatReg::ztof(int v, int a)
{
    int b;
    float e;
    memcpy((void *)&b,(void *)&v, 4);

    switch (a) {
        case 0:
            data.vf.z=(float)b;
            break;
        case 4:
            e=(float)b/16.0;
            data.vf.z=e;
            break;
        case 12:
            e=(float)b/4096.0;
            data.vf.z=e;
            break;
        case 15:
            e=(float)b/32768.0;
            data.vf.z=e;
            break;
    }
}

void VuFloatReg::wtof(int v, int a)
{
    int b;
    float e;
    memcpy((void *)&b,(void *)&v, 4);

    switch (a) {
        case 0:
            data.vf.w=(float)b;
            break;
        case 4:
            e=(float)b/16.0;
            data.vf.w=e;
            break;
        case 12:
            e=(float)b/4096.0;
            data.vf.w=e;
            break;
        case 15:
            e=(float)b/32768.0;
            data.vf.w=e;
            break;
    }
}


