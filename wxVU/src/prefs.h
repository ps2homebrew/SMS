#ifndef _PREFS_H_
#define _PREFS_H_

// style bits types
#define mySTC_STYLE_BOLD 1
#define mySTC_STYLE_ITALIC 2
#define mySTC_STYLE_UNDERL 4
#define mySTC_STYLE_HIDDEN 8

//----------------------------------------------------------------------------
// CommonInfo
struct CommonInfo {
    // project
    int     regOrder;
    int     autoLoadLast;
    wxChar  *memStateFile;
    wxChar  *regStateFile;
};
extern const CommonInfo g_CommonTable;
extern CommonInfo g_CommonPrefs;

//----------------------------------------------------------------------------
// RemoteInfo
struct RemoteInfo {
    int     autoGSExec;
    wxChar  *binTmpFile;
    wxChar  *datTmpFile;
    wxChar  *regTmpFile;
    wxChar  *gsTmpFile;
};
extern const RemoteInfo g_RemoteTable;
extern RemoteInfo g_RemotePrefs;

//----------------------------------------------------------------------------
// StyleInfo
struct StyleInfo {
    wxChar  *name;
    wxChar  *foreground;
    wxChar  *background;
    wxChar  *fontname;
    int     fontsize;
    int     fontstyle;
    int     lettercase;
};
extern const StyleInfo g_StyleTable;
extern StyleInfo g_StylePrefs;

//----------------------------------------------------------------------------
// GIF
struct GIFInfo {
    int     xOffset;
    int     yOffset;
    int     prim;
    int     sendPrim;
    int     tagShow;  
    int     clrcol;
    int     scissorX;
    int     scissorY;
};
extern const GIFInfo g_GIFTable;
extern GIFInfo g_GIFPrefs;

//----------------------------------------------------------------------------
class Prefs {
public:
    // boa constrictor
    Prefs ();

    // de struct or
    ~Prefs ();

    void LoadValuesPageLoad(bool dflt = false);
    void SaveValuesPageLoad();
    void DeleteValuesPageLoad();
    void SetDefaultValuesLoad();

    void LoadValuesPageRemote(bool dflt = false);
    void SaveValuesPageRemote();
    void DeleteValuesPageRemote();
    void SetDefaultValuesRemote();

    void LoadValuesPageStyles(bool dflt = false);
    void SaveValuesPageStyles();
    void DeleteValuesPageStyles();
    void SetDefaultValuesStyles();

    void LoadValuesPageGif(bool dflt = false);
    void SaveValuesPageGif();
    void DeleteValuesPageGif();
    void SetDefaultValuesGif();

private:
    wxConfig *m_config;

};
#endif // _PREFS_H_
