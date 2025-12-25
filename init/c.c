#define INIT_TASK                           \
    /* state etc */ {                       \
        0, /*这里*/                         \
            15,                             \
            15,                             \
            /* signals */ 0,                \
            {                               \
                {},                         \
            },                              \
            0,                              \
            /* ec,brk... */ 0,              \
            0,                              \
            0,                              \
            0,                              \
            0,                              \
            0,                              \
            /* pid etc.. */ 0,              \
            -1,                             \
            0,                              \
            0,                              \
            0,                              \
            /* uid etc */ 0,                \
            0,                              \
            0,                              \
            0,                              \
            0,                              \
            0,                              \
            /* alarm */ 0,                  \
            0,                              \
            0,                              \
            0,                              \
            0,                              \
            0,                              \
            /* math */ 0,                   \
            /* fs info */ -1,               \
            0022,                           \
            NULL,                           \
            NULL,                           \
            NULL,                           \
            0,                              \
            /* filp */ {                    \
                NULL,                       \
            },                              \
            {                               \
                {0, 0},                     \
                /* ldt */ {0x9f, 0xc0fa00}, \
                {0x9f, 0xc0f200},           \
            },                              \
        /*tss*/ {                           \
            0, PAGE_SIZE + (long)&init_task, 0x10,
0, 0, 0, 0, (long)&pg_dir,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0x17, 0x17, 0x17, 0x17,
    0x17, 0x17, _LDT(0),
    0x80000000, {}
}
,
}