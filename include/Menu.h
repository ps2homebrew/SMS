/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 bix64
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __Menu_H
# define __Menu_H

# define MENU_IF_SELECTED 0x00000001
# define MENU_IF_TEXT     0x00000002
# define MENU_IF_PALIDX   0x00000004

# define MENU_EV_OPTION_SELECT 0
# define MENU_EV_EXIT          1
# define MENU_EV_CONSUMED      4

struct BrowserContext;

typedef struct MenuItem {

 unsigned int     m_Flags;
 unsigned int     m_Y;
 char*            m_pOptionName;
 void*            m_pIconLeft;
 void*            m_pIconRight;
 struct MenuItem* m_pPrev;
 struct MenuItem* m_pNext;

 void ( *Handler ) ( void );

} MenuItem;

typedef struct MenuContext {

 struct BrowserContext* m_pBrowserCtx;
 MenuItem*              m_pItems;
 MenuItem*              m_pFirst;
 MenuItem*              m_pLast;
 MenuItem*              m_pCurr;
 char*                  m_pName;
 int                    m_Left;
 int                    m_Top;
 int                    m_Height;
 int                    m_Width;
 int                    m_Offset;
 unsigned int*          m_pSelIdx;
 int                    m_fText;

 void ( *Run ) ( void );

} MenuContext;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

MenuContext* MenuContext_Init ( struct BrowserContext* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __Menu_H */
