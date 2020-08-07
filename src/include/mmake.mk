# SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
# X-SPDX-Copyright-Text: (c) Copyright 2005-2020 Xilinx, Inc.
all:

clean:
	-@rm *.h $(EXTRA_CLEANS) 2>/dev/null

ifneq ($(DRIVER),1)
DRIVER=0

######################################################################
# Autogenerated headers to find out details about libc features
libc_compat.h: $(TOPPATH)/scripts/libc_compat.sh
	CC="${CC}" CFLAGS="${CFLAGS} ${MMAKE_CFLAGS}" $(TOPPATH)/scripts/libc_compat.sh libc_compat.h


all: libc_compat.h
endif
