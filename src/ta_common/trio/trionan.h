/*************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2001 Bjorn Reese <breese@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS AND
 * CONTRIBUTORS ACCEPT NO RESPONSIBILITY IN ANY CONCEIVABLE MANNER.
 *
 ************************************************************************/

#ifndef __TRIO_NAN_H__
#define __TRIO_NAN_H__

#include "triodef.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Return NaN (Not-a-Number).
 */
TRIO_PUBLIC double trio_nan(void);

/*
 * Return positive infinity.
 */
TRIO_PUBLIC double trio_pinf(void);

/*
 * Return negative infinity.
 */
TRIO_PUBLIC double trio_ninf(void);
  
/*
 * If number is a NaN return non-zero, otherwise return zero.
 */
TRIO_PUBLIC int trio_isnan(double number);

/*
 * If number is positive infinity return 1, if number is negative
 * infinity return -1, otherwise return 0.
 */
TRIO_PUBLIC int trio_isinf(double number);

#ifdef __cplusplus
}
#endif

#endif /* __TRIO_NAN_H__ */
