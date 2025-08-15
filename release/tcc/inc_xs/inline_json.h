


typedef enum {
    JNUM_NULL,
    JNUM_BOOL,
    JNUM_INT,
    JNUM_HEX,
    JNUM_LINT,
    JNUM_LHEX,
    JNUM_DOUBLE,
} jnum_type_t;

typedef union {
    bool vbool;
    int32_t vint;
    uint32_t vhex;
    int64_t vlint;
    uint64_t vlhex;
    double vdbl;
} jnum_value_t;

extern int jnum_itoa(int32_t num, char *buffer);
extern int jnum_ltoa(int64_t num, char *buffer);
extern int jnum_htoa(uint32_t num, char *buffer);
extern int jnum_lhtoa(uint64_t num, char *buffer);
extern int jnum_dtoa(double num, char *buffer);

extern int32_t jnum_atoi(const char *str);
extern int64_t jnum_atol(const char *str);
extern uint32_t jnum_atoh(const char *str);
extern uint64_t jnum_atolh(const char *str);
extern double jnum_atod(const char *str);


