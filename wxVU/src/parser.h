class disasm {
public:
    void    upper(uint32 *upper, char *upper);
};
void dlower(uint32 *lower, char *low, char *lparam);
void dupper(uint32 *upper, char *upp, char *uparam);
int insert(char *upper, char *lower, char *uparam, char *lparam, uint32 index);
int LoadCode(char *file);
int LoadInstructions(char *file);
