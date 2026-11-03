include $(MTK_PATH_BUILD)/common.mk
$(call all-modules-src-or-makefile,$(obj),CUSTOM_KERNEL_)
obj-n := dummy.o

ifeq ($(MTK_ALPS_BOX_SUPPORT), yes)
ccflags-y += -DMTK_ALPS_BOX_SUPPORT
endif

# ISP core needs the sensor dispatch layer even for a boot-only target with
# no concrete sensors selected. The list then contains only its terminator.
ifeq ($(strip $(CUSTOM_KERNEL_IMGSENSOR)),)
obj-y += imgsensor/kd_sensorlist.o
endif
