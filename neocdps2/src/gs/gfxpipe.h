// GFX-Pipe by Vzzrzzn, modifications by Sjeep

#ifndef _GFXPIPE_H_
#define _GFXPIPE_H_


#define GP_MINSPACE     18      // minimum space left in pipeline before flush (must be at least 36 dwords)

#define GS_TEX_SIZE_2    1
#define GS_TEX_SIZE_4    2
#define GS_TEX_SIZE_8    3
#define GS_TEX_SIZE_16   4
#define GS_TEX_SIZE_32   5
#define GS_TEX_SIZE_64   6
#define GS_TEX_SIZE_128  7
#define GS_TEX_SIZE_256  8
#define GS_TEX_SIZE_512  9
#define GS_TEX_SIZE_102 10

// filter method
#define GS_FILTER_NEAREST	0x00
#define GS_FILTER_LINEAR	0x01

#define IMAGE_MAX_QWORD	0x7FF0

int filterMethod;

typedef struct gfxpipe gfxpipe;
typedef struct vecTriList vecTriList;

//#pragma pack(1)
struct gfxpipe {
    unsigned long *dmatadrA;    // 'pipe 1' ... base of allotted pipeline memory
    unsigned long *dmatadrB;    // 'pipe 2' ... dmatadrA + (memsize / 2)
    unsigned int memsize;       // # of bytes allotted to the pipelines (total)

    unsigned long *curpipe;     // pointer to current 'pipe' .. may only be equal to
                                // either dmatadrA or dmatadrB
    unsigned long *curdmatadr;  // pointer to the the dma block currently being added to
    unsigned long *curgiftag;   // pointer to current "block" we can add prims to

    // need to add state information of zbuffer, alpha test, etc. in here
    int flags;          // not implemented yet
};
/*
#pragma pack(1)
struct vecTriList {
    int numv; int d0[3];

    unsigned long giftag[2];
    vec vlist[0];   // if you say 'vecTriList *vtl = malloc(1032);' then you can access
                    //    from 'vlist[0]' through 'vlist[1000]'.
};
*/
void gp_hardflush(gfxpipe *p);
void gp_checkflush(gfxpipe *p);
unsigned int gp_bytesLeft(gfxpipe *p);
// size must be less than 1MB
int createGfxPipe(gfxpipe *pipeline /*, void *buffer*/, int size);

void gp_line(gfxpipe *p, unsigned x1, unsigned y1, unsigned x2, unsigned y2, unsigned z, unsigned color);

void gp_ltriangle(gfxpipe *p, unsigned x1, unsigned y1, unsigned z1, unsigned c1,
                              unsigned x2, unsigned y2, unsigned z2, unsigned c2,
                              unsigned x3, unsigned y3, unsigned z3, unsigned c3);

void gp_triangle(gfxpipe *p, unsigned x1, unsigned y1, unsigned z1, unsigned c1,
                              unsigned x2, unsigned y2, unsigned z2, unsigned c2,
                              unsigned x3, unsigned y3, unsigned z3, unsigned c3);

void gp_ftriangle(gfxpipe *p, unsigned x1, unsigned y1, unsigned z1,
                              unsigned x2, unsigned y2, unsigned z2,
                              unsigned x3, unsigned y3, unsigned z3, unsigned color);

void gp_frect(gfxpipe *p, unsigned x1, unsigned y1, unsigned x2, unsigned y2, unsigned z, unsigned color);

void gp_uvftriangle(gfxpipe *p, unsigned x1, unsigned y1, unsigned z1, unsigned u1, unsigned v1,
                                unsigned x2, unsigned y2, unsigned z2, unsigned u2, unsigned v2,
                                unsigned x3, unsigned y3, unsigned z3, unsigned u3, unsigned v3,unsigned color);

/*
void gp_stftriangle(gfxpipe *p, unsigned x1, unsigned y1, unsigned z1, float s1, float t1,
                                unsigned x2, unsigned y2, unsigned z2, float s2, float t2,
                                unsigned x3, unsigned y3, unsigned z3, float s3, float t3,unsigned color);
*/

//void gp_drawVTL_inplace_cpn(gfxpipe *p, vecTriList *vtl);


// send a byte-packed texture from RDRAM to VRAM
// TBP = VRAM_address
// TBW = buffer_width_in_pixels  -- dependent on pxlfmt
// xofs, yofs in units of pixels
// pxlfmt = 0x00 (32-bit), 0x02 (16-bit), 0x13 (8-bit), 0x14 (4-bit)
// wpxls, hpxls = width, height in units of pixels
void gp_uploadTexture(gfxpipe *p, int TBP, int TBW, int xofs, int yofs, int pxlfmt, const void *tex, int wpxls, int hpxls);
//void gp_uploadTexture(gfxpipe *p, unsigned int TBP, int TBW, int xofs, int yofs, int pxlfmt, const void *tex, int wpxls, int hpxls);


void gp_enablezbuf(gfxpipe *p);
void gp_disablezbuf(gfxpipe *p);

void gp_setFilterMethod(int filter);


void gp_setTex(gfxpipe *p, u32 tbp, u32 tbw, u32 texwidth, u32 texheight, u32 tpsm, u32 cbp, u32 cbw, u32 cpsm);
void gp_texrect(gfxpipe *p, u32 x1, u32 y1, u32 u1, u32 v1, u32 x2, u32 y2, u32 u2, u32 v2, u32 z, u32 colour);
void gp_linerect(gfxpipe *p, unsigned x1, unsigned y1, unsigned x2, unsigned y2, unsigned z, unsigned color);
void gp_point(gfxpipe *p, unsigned x1, unsigned y1, unsigned z, unsigned colour);
void gp_gouradrect(gfxpipe *p, unsigned x1, unsigned y1, unsigned c1, unsigned x2, unsigned y2, unsigned c2, unsigned z);

extern gfxpipe thegp;

#endif
