// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
//
// Simple singleton for keeping track of breakpoints set in the editor.
//

#include "datatypes.h"

enum Breakpoint_TYPES { BRK_NONE, BRK_INTREG, BRK_FLOATREG, BRK_MEMORY };

typedef struct node {
    uint32  row;            // row number in code mem
    uint32  index;          // index into a type we want to check
    uint32  type;           // type is NONE, INT REG, FLOAT REG or MEM
    int32   value_x;
    int32   value_y;
    int32   value_z;
    int32   value_w;
    void    *next;
} bplist;

class Breakpoint {
public:
    static Breakpoint*	Instance();
    bool				Breakpoint::check();
    void				Breakpoint::add(uint32 row, uint32 type, uint32 index,
							int32 value_x, int32 value_y, int32 value_z, int32
							value_w);
    void				Breakpoint::remove();
    void				Breakpoint::list();
protected:
    Breakpoint();
    ~Breakpoint();
private:
    static Breakpoint*	_instance;
    bplist				*m_cur, *m_head;
};
