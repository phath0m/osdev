#ifndef _SYS_SHADOW
#define _SYS_SHADOW

struct spwd {
    char *              sp_namp;
    char *              sp_pwdp;
    long int            sp_lstchg;
    long int            sp_min;
    long int            sp_max;
    long int            sp_warn;
    long int            sp_inact;
    long int            sp_expire;
    unsigned long int   sp_flag;
};

struct spwd *getspnam(const char *name);

#endif
