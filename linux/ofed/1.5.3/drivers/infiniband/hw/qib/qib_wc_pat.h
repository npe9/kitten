/*
 * Copyright (c) 2008,2009 QLogic Corporation. All rights reserved.
 * Copyright (c) 2006-2007 Mellanox Technologies. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *        Redistribution and use in source and binary forms, with or
 *        without modification, are permitted provided that the following
 *        conditions are met:
 *
 *         - Redistributions of source code must retain the above
 *           copyright notice, this list of conditions and the following
 *           disclaimer.
 *
 *         - Redistributions in binary form must reproduce the above
 *           copyright notice, this list of conditions and the following
 *           disclaimer in the documentation and/or other materials
 *           provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef QIB_WC_PAT_H
#define QIB_WC_PAT_H

#include <asm/pgtable.h>

#include "qib.h"

extern unsigned qib_wc_pat;

int qib_enable_wc_pat(void);
void qib_disable_wc_pat(void);
int qib_wc_pat_enabled(void);
pgprot_t pgprot_writecombine(pgprot_t _prot);

#endif