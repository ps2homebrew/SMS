// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
//
// Simple singleton for keeping track of breakpoints set in the editor.
//

#include <list>
#include "datatypes.h"

enum Breakpoint_TYPES { BRK_NONE, BRK_INTREG, BRK_FLOATREG, BRK_MEMORY };

typedef struct t_breakpoint {
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
    void				Breakpoint::Add(uint32 row, uint32 type, uint32 index,
							int32 value_x, int32 value_y, int32 value_z, int32
							value_w);
    void				Breakpoint::Delete();
    void				Breakpoint::List();
protected:
    Breakpoint();
    ~Breakpoint();
private:
    static Breakpoint*	_instance;
    typedef std::list<t_breakpoint>     t_list;
    typedef t_list::iterator            iterator;
    typedef t_list::const_iterator      const_iterator;
    t_list              m_list;
    bplist				*m_cur, *m_head;
    int32               m_count;
};
