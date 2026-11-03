#include "partition_define.h"
static const struct excel_info PartInfo_Private[PART_NUM] = {
    {"preloader", 0x80000, 0x0, NAND, 0, USER},
    {"dsp_bl", 0x180000, 0x80000, NAND, 0, USER},
    {"nvram", 0x300000, 0x200000, NAND, 0, USER},
    {"seccfg", 0x40000, 0x500000, NAND, 0, USER},
    {"uboot", 0x80000, 0x540000, NAND, 0, USER},
    {"bootimg", 0x500000, 0x5c0000, NAND, 0, USER},
    {"recovery", 0x500000, 0xac0000, NAND, 0, USER},
    {"sec_ro", 0x100000, 0xfc0000, NAND, 0, USER},
    {"misc", 0x80000, 0x10c0000, NAND, 0, USER},
    {"logo", 0x200000, 0x1140000, NAND, 0, USER},
    {"expdb", 0xc0000, 0x1340000, NAND, 0, USER},
    {"fat", 0x500000, 0x1400000, NAND, 0, USER},
    {"android", 0xa780000, 0x1900000, NAND, 0, USER},
    {"cache", 0x400000, 0xc080000, NAND, 0, USER},
    {"usrdata", 0x12700000, 0xc480000, NAND, 0, USER},
};
