obj-$(CONFIG_MTK_MTD_NAND) += mtk_nand.o
obj-$(CONFIG_MTK_MTD_NAND) += bmt.o
obj-$(CONFIG_MTK_MTD_NAND) += partition_mt.o

# First A690 OTP validation permits only native query/read operations.
ifeq ($(TARGET_PRODUCT),lcsh75_cu_gb2)
CFLAGS_mtk_nand.o += -DA690_NAND_OTP_READONLY
endif
