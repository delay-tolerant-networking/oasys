// XXX/demmer add copyright

#include "Formatter.h"
#include <stdarg.h>

/**
 * @file
 *
 * The bulk of this file was copied from the FreeBSD 5.0 distribution,
 * in /usr/src/lib/libc/stdio/vfprintf.c since that's the function
 * that performs the real work of format string processing.
 *
 * Changes to the file include several modifications to make it
 * compile, changes to implement snprintf / vsnprintf, and of course,
 * the changes to implement the Formatter::format() callback hook.
 * 
 * Wherever possible, the original implementation has been retained to
 * simplify merging in future changes from the FreeBSD camp.
 *
 */
#define __FORMATTER__

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)vfprintf.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */
#include <sys/cdefs.h>
//__FBSDID("$FreeBSD: src/lib/libc/stdio/vfprintf.c,v 1.59 2003/04/19 23:53:19 das Exp $");

/*
 * Actual printf innards.
 *
 * This code is large and complicated...
 */

//#include "namespace.h"
#include <sys/types.h>

#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <stdarg.h>
/*
#include "un-namespace.h"

#include "libc_private.h"
#include "local.h"
*/

#ifndef __FORMATTER__
#include "fvwrite.h"
#else
/*
 * I/O descriptors for __sfvwrite().
 */
/*
struct __siov {
	void	*iov_base;
	size_t	iov_len;
};
struct __suio {
	struct	__siov *uio_iov;
	int	uio_iovcnt;
	int	uio_resid;
};
*/
#endif 

#ifdef __FORMATTER__
#include <limits.h>
#define INTMAX_MAX LLONG_MAX
#define reallocf realloc
#define __THROW
#endif

/* Define FLOATING_POINT to get floating point. */
#define	FLOATING_POINT

union arg {
	int	intarg;
	u_int	uintarg;
	long	longarg;
	u_long	ulongarg;
	long long longlongarg;
	unsigned long long ulonglongarg;
	ptrdiff_t ptrdiffarg;
	size_t	sizearg;
	intmax_t intmaxarg;
	uintmax_t uintmaxarg;
	void	*pvoidarg;
	char	*pchararg;
	signed char *pschararg;
	short	*pshortarg;
	int	*pintarg;
	long	*plongarg;
	long long *plonglongarg;
	ptrdiff_t *pptrdiffarg;
	size_t	*psizearg;
	intmax_t *pintmaxarg;
#ifdef FLOATING_POINT
	double	doublearg;
	long double longdoublearg;
#endif
	wint_t	wintarg;
	wchar_t	*pwchararg;
};

/*
 * Type ids for argument type table.
 */
enum typeid_t {
	T_UNUSED, TP_SHORT, T_INT, T_U_INT, TP_INT,
	T_LONG, T_U_LONG, TP_LONG, T_LLONG, T_U_LLONG, TP_LLONG,
	T_PTRDIFFT, TP_PTRDIFFT, T_SIZET, TP_SIZET,
	T_INTMAXT, T_UINTMAXT, TP_INTMAXT, TP_VOID, TP_CHAR, TP_SCHAR,
	T_DOUBLE, T_LONG_DOUBLE, T_WINT, TP_WCHAR
};


/*
static int	__sprint(FILE *, struct __suio *);
static int	__sbprintf(FILE *, const char *, va_list) __printflike(2, 0);
*/
static char	*__ujtoa(uintmax_t, char *, int, int, const char *, int, char,
		    const char *);
static char	*__ultoa(u_long, char *, int, int, const char *, int, char,
		    const char *);
static char	*__wcsconv(wchar_t *, int);
static void	__find_arguments(const char *, va_list, union arg **);
static int	__grow_type_table(int, enum typeid_t **, int *);

/*
 * Flush out all the vectors defined by the given uio,
 * then reset it so that it can be reused.
 */
// static int
// __sprint(FILE *fp, struct __suio *uio)
// {
// 	int err;

// 	if (uio->uio_resid == 0) {
// 		uio->uio_iovcnt = 0;
// 		return (0);
// 	}
// 	err = __sfvwrite(fp, uio);
// 	uio->uio_resid = 0;
// 	uio->uio_iovcnt = 0;
// 	return (err);
// }

/*
 * Helper function for `fprintf to unbuffered unix file': creates a
 * temporary buffer.  We only work on write-only files; this avoids
 * worries about ungetc buffers and so forth.
 */
// static int
// __sbprintf(FILE *fp, const char *fmt, va_list ap)
// {
// 	int ret;
// 	FILE fake;
// 	unsigned char buf[BUFSIZ];

// 	/* copy the important variables */
// 	fake._flags = fp->_flags & ~__SNBF;
// 	fake._file = fp->_file;
// 	fake._cookie = fp->_cookie;
// 	fake._write = fp->_write;
// 	fake._extra = fp->_extra;

// 	/* set up the buffer */
// 	fake._bf._base = fake._p = buf;
// 	fake._bf._size = fake._w = sizeof(buf);
// 	fake._lbfsize = 0;	/* not actually used, but Just In Case */

// 	/* do the work, then copy any error status */
// 	ret = __vfprintf(&fake, fmt, ap);
// 	if (ret >= 0 && __fflush(&fake))
// 		ret = EOF;
// 	if (fake._flags & __SERR)
// 		fp->_flags |= __SERR;
// 	return (ret);
// }

/*
 * Macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

/*
 * Convert an unsigned long to ASCII for printf purposes, returning
 * a pointer to the first character of the string representation.
 * Octal numbers can be forced to have a leading zero; hex numbers
 * use the given digits.
 */
static char *
__ultoa(u_long val, char *endp, int base, int octzero, const char *xdigs,
	int needgrp, char thousep, const char *grp)
{
	char *cp = endp;
	long sval;
	int ndig;

	/*
	 * Handle the three cases separately, in the hope of getting
	 * better/faster code.
	 */
	switch (base) {
	case 10:
		if (val < 10) {	/* many numbers are 1 digit */
			*--cp = to_char(val);
			return (cp);
		}
		ndig = 0;
		/*
		 * On many machines, unsigned arithmetic is harder than
		 * signed arithmetic, so we do at most one unsigned mod and
		 * divide; this is sufficient to reduce the range of
		 * the incoming value to where signed arithmetic works.
		 */
		if (val > LONG_MAX) {
			*--cp = to_char(val % 10);
			ndig++;
			sval = val / 10;
		} else
			sval = val;
		do {
			*--cp = to_char(sval % 10);
			ndig++;
			/*
			 * If (*grp == CHAR_MAX) then no more grouping
			 * should be performed.
			 */
			if (needgrp && ndig == *grp && *grp != CHAR_MAX
					&& sval > 9) {
				*--cp = thousep;
				ndig = 0;
				/*
				 * If (*(grp+1) == '\0') then we have to
				 * use *grp character (last grouping rule)
				 * for all next cases
				 */
				if (*(grp+1) != '\0')
					grp++;
			}
			sval /= 10;
		} while (sval != 0);
		break;

	case 8:
		do {
			*--cp = to_char(val & 7);
			val >>= 3;
		} while (val);
		if (octzero && *cp != '0')
			*--cp = '0';
		break;

	case 16:
		do {
			*--cp = xdigs[val & 15];
			val >>= 4;
		} while (val);
		break;

	default:			/* oops */
		abort();
	}
	return (cp);
}

/* Identical to __ultoa, but for intmax_t. */
static char *
__ujtoa(uintmax_t val, char *endp, int base, int octzero, const char *xdigs, 
	int needgrp, char thousep, const char *grp)
{
	char *cp = endp;
	intmax_t sval;
	int ndig;

	/* quick test for small values; __ultoa is typically much faster */
	/* (perhaps instead we should run until small, then call __ultoa?) */
	if (val <= ULONG_MAX)
		return (__ultoa((u_long)val, endp, base, octzero, xdigs,
		    needgrp, thousep, grp));
	switch (base) {
	case 10:
		if (val < 10) {
			*--cp = to_char(val % 10);
			return (cp);
		}
		ndig = 0;
		if (val > INTMAX_MAX) {
			*--cp = to_char(val % 10);
			ndig++;
			sval = val / 10;
		} else
			sval = val;
		do {
			*--cp = to_char(sval % 10);
			ndig++;
			/*
			 * If (*grp == CHAR_MAX) then no more grouping
			 * should be performed.
			 */
			if (needgrp && *grp != CHAR_MAX && ndig == *grp
					&& sval > 9) {
				*--cp = thousep;
				ndig = 0;
				/*
				 * If (*(grp+1) == '\0') then we have to
				 * use *grp character (last grouping rule)
				 * for all next cases
				 */
				if (*(grp+1) != '\0')
					grp++;
			}
			sval /= 10;
		} while (sval != 0);
		break;

	case 8:
		do {
			*--cp = to_char(val & 7);
			val >>= 3;
		} while (val);
		if (octzero && *cp != '0')
			*--cp = '0';
		break;

	case 16:
		do {
			*--cp = xdigs[val & 15];
			val >>= 4;
		} while (val);
		break;

	default:
		abort();
	}
	return (cp);
}

/*
 * Convert a wide character string argument for the %ls format to a multibyte
 * string representation. ``prec'' specifies the maximum number of bytes
 * to output. If ``prec'' is greater than or equal to zero, we can't assume
 * that the wide char. string ends in a null character.
 */
static char *
__wcsconv(wchar_t *wcsarg, int prec)
{
	char buf[MB_LEN_MAX];
	wchar_t *p;
	char *convbuf, *mbp;
	size_t clen = 0, nbytes;
	mbstate_t mbs;

	/*
	 * Determine the number of bytes to output and allocate space for
	 * the output.
	 */
	memset(&mbs, 0, sizeof(mbs));
	if (prec >= 0) {
		nbytes = 0;
		p = wcsarg;
		for (;;) {
			clen = wcrtomb(buf, *p++, &mbs);
			if (clen == 0 || clen == (size_t)-1 ||
			    nbytes + clen > (size_t)prec)
				break;
			nbytes += clen;
		}
	} else {
		p = wcsarg;
		nbytes = wcsrtombs(NULL, (const wchar_t **)&p, 0, &mbs);
		if (nbytes == (size_t)-1)
			return (NULL);
	}
	if ((convbuf = (char*)malloc(nbytes + 1)) == NULL)
		return (NULL);

	/*
	 * Fill the output buffer with the multibyte representations of as
	 * many wide characters as will fit.
	 */
	mbp = convbuf;
	p = wcsarg;
	memset(&mbs, 0, sizeof(mbs));
	while (mbp - convbuf < (int)nbytes) {
		clen = wcrtomb(mbp, *p++, &mbs);
		if (clen == 0 || clen == (size_t)-1)
			break;
		mbp += clen;
	}
	if (clen == (size_t)-1) {
		free(convbuf);
		return (NULL);
	}
	*mbp = '\0';

	return (convbuf);
}

/*
 * MT-safe version
 */
/*
int
vfprintf(FILE * __restrict fp, const char * __restrict fmt0, va_list ap)

{
	int ret;

	FLOCKFILE(fp);
	ret = __vfprintf(fp, fmt0, ap);
	FUNLOCKFILE(fp);
	return (ret);
}
*/

#ifdef FLOATING_POINT

/*
#define	dtoa		__dtoa
#define	freedtoa	__freedtoa
*/

#include <float.h>
#include <math.h>

#ifndef __FORMATTER__
#include "floatio.h"
#else
/*
 * (copied from floatio.h)
 */
/*
 * MAXEXPDIG is the maximum number of decimal digits needed to store a
 * floating point exponent in the largest supported format.  It should
 * be ceil(log10(LDBL_MAX_10_EXP)) or, if hexadecimal floating point
 * conversions are supported, ceil(log10(LDBL_MAX_EXP)).  But since it
 * is presently never greater than 5 in practice, we fudge it.
 */
#define	MAXEXPDIG	6
#if LDBL_MAX_EXP > 999999
#error "floating point buffers too small"
#endif /* __FORMATTER__ */

static char *__ldtoa(long double *, int, int, int *, int *, char **);
#endif

#include "gdtoa.h"

#define	DEFPREC		6

static int exponent(char *, int, int);

#endif /* FLOATING_POINT */

/*
 * The size of the buffer we use as scratch space for integer
 * conversions, among other things.  Technically, we would need the
 * most space for base 10 conversions with thousands' grouping
 * characters between each pair of digits.  100 bytes is a
 * conservative overestimate even for a 128-bit uintmax_t.
 */
#define	BUF	100

#define STATIC_ARG_TBL_SIZE 8           /* Size of static argument table. */

/*
 * Flags used during conversion.
 */
#define	ALT		0x001		/* alternate form */
#define	LADJUST		0x004		/* left adjustment */
#define	LONGDBL		0x008		/* long double */
#define	LONGINT		0x010		/* long integer */
#define	LLONGINT	0x020		/* long long integer */
#define	SHORTINT	0x040		/* short integer */
#define	ZEROPAD		0x080		/* zero (as opposed to blank) pad */
#define	FPT		0x100		/* Floating point number */
#define	GROUPING	0x200		/* use grouping ("'" flag) */
					/* C99 additional size modifiers: */
#define	SIZET		0x400		/* size_t */
#define	PTRDIFFT	0x800		/* ptrdiff_t */
#define	INTMAXT		0x1000		/* intmax_t */
#define	CHARINT		0x2000		/* print char using int format */

/*
 * Non-MT-safe version
 */
int
//__vfprintf(FILE *fp, const char *fmt0, va_list ap)
vsnprintf(char *str, size_t strsz, const char *fmt0, va_list ap) __THROW
{
	char *fmt;		/* format string */
	int ch;			/* character from fmt */
	int n, n2;		/* handy integer (short term usage) */
        size_t sz;		/* handy size_t (short term usage) */
	char *cp;		/* handy char pointer (short term usage) */
//	struct __siov *iovp;	/* for PRINT macro */
	int flags;		/* flags as above */
	int ret;		/* return value accumulator */
	int width;		/* width from format (%8d), or 0 */
	int prec;		/* precision from format; <0 for N/A */
	char sign;		/* sign prefix (' ', '+', '-', or \0) */
	char thousands_sep;	/* locale specific thousands separator */
	const char *grouping;	/* locale specific numeric grouping rules */
#ifdef FLOATING_POINT
	/*
	 * We can decompose the printed representation of floating
	 * point numbers into several parts, some of which may be empty:
	 *
	 * [+|-| ] [0x|0X] MMM . NNN [e|E|p|P] [+|-] ZZ
	 *    A       B     ---C---      D       E   F
	 *
	 * A:	'sign' holds this value if present; '\0' otherwise
	 * B:	ox[1] holds the 'x' or 'X'; '\0' if not hexadecimal
	 * C:	cp points to the string MMMNNN.  Leading and trailing
	 *	zeros are not in the string and must be added.
	 * D:	expchar holds this character; '\0' if no exponent, e.g. %f
	 * F:	at least two digits for decimal, at least one digit for hex
	 */
	char *decimal_point;	/* locale specific decimal point */
	int signflag;		/* true if float is negative */
	union {			/* floating point arguments %[aAeEfFgG] */
		double dbl;
		long double ldbl;
	} fparg;
	int expt;		/* integer value of exponent */
	char expchar = 0;	/* exponent character: [eEpP\0] */
	char *dtoaend;		/* pointer to end of converted digits */
	int expsize = 0;	/* character count for expstr */
	int lead = 0;		/* sig figs before decimal or group sep */
	int ndig = 0;		/* actual number of digits returned by dtoa */
	char expstr[MAXEXPDIG+2];	/* buffer for exponent string: e+ZZZ */
	char *dtoaresult;	/* buffer allocated by dtoa */
	int nseps = 0;		/* number of group separators with ' */
	int nrepeats = 0;	/* number of repeats of the last group */
#endif
	u_long	ulval = 0;	/* integer arguments %[diouxX] */
	uintmax_t ujval = 0;	/* %j, %ll, %q, %t, %z integers */
	int base;		/* base for [diouxX] conversion */
	int dprec;		/* a copy of prec if [diouxX], 0 otherwise */
	int realsz;		/* field size expanded by dprec, sign, etc */
	int size;		/* size of converted field or string */
	int prsize;             /* max size of printed field */
	const char *xdigs = 0; 	/* digits for %[xX] conversion */
// #define NIOV 8
// 	struct __suio uio;	/* output information: summary */
// 	struct __siov iov[NIOV];/* ... and individual io vectors */
	char buf[BUF];		/* buffer with space for digits of uintmax_t */
	char ox[2];		/* space for 0x; ox[1] is either x, X, or \0 */
	union arg *argtable;    /* args, built due to positional arg */
	union arg statargtable [STATIC_ARG_TBL_SIZE];
	int nextarg;            /* 1-based argument index */
	va_list orgap;          /* original argument pointer */
	char *convbuf;		/* wide to multibyte conversion result */
	Formatter* fmtobj;	/* pointer to object of type Formatter */

	/*
	 * Choose PADSIZE to trade efficiency vs. size.  If larger printf
	 * fields occur frequently, increase PADSIZE and make the initialisers
	 * below longer.
	 */
#define	PADSIZE	16		/* pad chunk size */
	static char blanks[PADSIZE] =
	 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
	static char zeroes[PADSIZE] =
	 {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};

	static const char xdigs_lower[] = "0123456789abcdef";
	static const char xdigs_upper[] = "0123456789ABCDEF";

	/*
	 * BEWARE, these `goto error' on error, and PAD uses `n'.
	 */
/*
#define	PRINT(ptr, len) { \
 	iovp->iov_base = (ptr); \
 	iovp->iov_len = (len); \
 	uio.uio_resid += (len); \
 	iovp++; \
 	if (++uio.uio_iovcnt >= NIOV) { \
 		if (__sprint(fp, &uio)) \
 			goto error; \
 		iovp = iov; \
 	} \
}
*/

#ifdef __FORMATTER__
#define PRINT(ptr, len) {                               \
        sz = (((size_t)len) <= strsz) ? len : strsz;    \
        memcpy(str, ptr, sz);                           \
        str += sz;                                      \
        strsz -= sz;                                    \
}
#endif
                                          
#define	PAD(howmany, with) { \
	if ((n = (howmany)) > 0) { \
		while (n > PADSIZE) { \
			PRINT(with, PADSIZE); \
			n -= PADSIZE; \
		} \
		PRINT(with, n); \
	} \
}

#define	PRINTANDPAD(p, ep, len, with) do {	\
	n2 = (ep) - (p);       			\
	if (n2 > (len))				\
		n2 = (len);			\
	if (n2 > 0)				\
		PRINT((p), n2);			\
	PAD((len) - (n2 > 0 ? n2 : 0), (with));	\
} while(0)

/*
#define	FLUSH() { \
	if (uio.uio_resid && __sprint(fp, &uio)) \
		goto error; \
	uio.uio_iovcnt = 0; \
	iovp = iov; \
}
*/
        
#ifdef __FORMATTER__
#define FLUSH() {}
#endif
	/*
	 * Get the argument indexed by nextarg.   If the argument table is
	 * built, use it to get the argument.  If its not, get the next
	 * argument (and arguments must be gotten sequentially).
	 */
#define GETARG(type) \
	((argtable != NULL) ? *((type*)(&argtable[nextarg++])) : \
	    (nextarg++, va_arg(ap, type)))

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#define	SARG() \
	(flags&LONGINT ? GETARG(long) : \
	    flags&SHORTINT ? (long)(short)GETARG(int) : \
	    flags&CHARINT ? (long)(signed char)GETARG(int) : \
	    (long)GETARG(int))
#define	UARG() \
	(flags&LONGINT ? GETARG(u_long) : \
	    flags&SHORTINT ? (u_long)(u_short)GETARG(int) : \
	    flags&CHARINT ? (u_long)(u_char)GETARG(int) : \
	    (u_long)GETARG(u_int))
#define	INTMAX_SIZE	(INTMAXT|SIZET|PTRDIFFT|LLONGINT)
#define SJARG() \
	(flags&INTMAXT ? GETARG(intmax_t) : \
	    flags&SIZET ? (intmax_t)GETARG(size_t) : \
	    flags&PTRDIFFT ? (intmax_t)GETARG(ptrdiff_t) : \
	    (intmax_t)GETARG(long long))
#define	UJARG() \
	(flags&INTMAXT ? GETARG(uintmax_t) : \
	    flags&SIZET ? (uintmax_t)GETARG(size_t) : \
	    flags&PTRDIFFT ? (uintmax_t)GETARG(ptrdiff_t) : \
	    (uintmax_t)GETARG(unsigned long long))

	/*
	 * Get * arguments, including the form *nn$.  Preserve the nextarg
	 * that the argument can be gotten once the type is determined.
	 */
#define GETASTER(val) \
	n2 = 0; \
	cp = fmt; \
	while (is_digit(*cp)) { \
		n2 = 10 * n2 + to_digit(*cp); \
		cp++; \
	} \
	if (*cp == '$') { \
		int hold = nextarg; \
		if (argtable == NULL) { \
			argtable = statargtable; \
			__find_arguments (fmt0, orgap, &argtable); \
		} \
		nextarg = n2; \
		val = GETARG (int); \
		nextarg = hold; \
		fmt = ++cp; \
	} else { \
		val = GETARG (int); \
	}


	thousands_sep = '\0';
	grouping = NULL;
	convbuf = NULL;
#ifdef FLOATING_POINT
	dtoaresult = NULL;
	decimal_point = localeconv()->decimal_point;
#endif
	/* sorry, fprintf(read_only_file, "") returns EOF, not 0 */
// 	if (cantwrite(fp))
// 		return (EOF);

	/* optimise fprintf(stderr) (and other unbuffered Unix files) */
// 	if ((fp->_flags & (__SNBF|__SWR|__SRW)) == (__SNBF|__SWR) &&
// 	    fp->_file >= 0)
// 		return (__sbprintf(fp, fmt0, ap));

	fmt = (char *)fmt0;
	argtable = NULL;
	nextarg = 1;

// old gcc doesn't export va_copy by default
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define va_copy __va_copy
#endif

	va_copy(orgap, ap);
// 	uio.uio_iov = iovp = iov;
// 	uio.uio_resid = 0;
// 	uio.uio_iovcnt = 0;
	ret = 0;

	if (strsz == 0) {
		return -1;
	}
	
	strsz -= 1;

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
			/* void */;
		if ((n = fmt - cp) != 0) {
			if ((unsigned)ret + n > INT_MAX) {
				ret = EOF;
				goto error;
			}
			PRINT(cp, n);
			ret += n;
		}
		if (ch == '\0')
			goto done;
		fmt++;		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';
		ox[1] = '\0';

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		case ' ':
			/*-
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '*':
			/*-
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			GETASTER (width);
			if (width >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '\'':
			flags |= GROUPING;
			thousands_sep = *(localeconv()->thousands_sep);
			grouping = localeconv()->grouping;
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				GETASTER (prec);
				goto rflag;
			}
			prec = 0;
			while (is_digit(ch)) {
				prec = 10 * prec + to_digit(ch);
				ch = *fmt++;
			}
			goto reswitch;
		case '0':
			/*-
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			} while (is_digit(ch));
			if (ch == '$') {
				nextarg = n;
				if (argtable == NULL) {
					argtable = statargtable;
					__find_arguments (fmt0, orgap,
					    &argtable);
				}
				goto rflag;
			}
			width = n;
			goto reswitch;
#ifdef FLOATING_POINT
		case 'L':
			flags |= LONGDBL;
			goto rflag;
#endif
		case 'h':
			if (flags & SHORTINT) {
				flags &= ~SHORTINT;
				flags |= CHARINT;
			} else
				flags |= SHORTINT;
			goto rflag;
		case 'j':
			flags |= INTMAXT;
			goto rflag;
		case 'l':
			if (flags & LONGINT) {
				flags &= ~LONGINT;
				flags |= LLONGINT;
			} else
				flags |= LONGINT;
			goto rflag;
		case 'q':
			flags |= LLONGINT;	/* not necessarily */
			goto rflag;
		case 't':
			flags |= PTRDIFFT;
			goto rflag;
		case 'z':
			flags |= SIZET;
			goto rflag;
		case 'C':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'c':
			if (flags & LONGINT) {
				mbstate_t mbs;
				size_t mbseqlen;

				memset(&mbs, 0, sizeof(mbs));
				mbseqlen = wcrtomb(cp = buf,
				    (wchar_t)GETARG(wint_t), &mbs);
				if (mbseqlen == (size_t)-1) {
//					fp->_flags |= __SERR;
					ret = EOF;
					goto error;
				}
				size = (int)mbseqlen;
			} else {
				*(cp = buf) = GETARG(int);
				size = 1;
			}
			sign = '\0';
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			if (flags & INTMAX_SIZE) {
				ujval = SJARG();
				if ((intmax_t)ujval < 0) {
					ujval = -ujval;
					sign = '-';
				}
			} else {
				ulval = SARG();
				if ((long)ulval < 0) {
					ulval = -ulval;
					sign = '-';
				}
			}
			base = 10;
			goto number;
#ifdef FLOATING_POINT
#ifdef HEXFLOAT
		case 'a':
		case 'A':
			if (ch == 'a') {
				ox[1] = 'x';
				xdigs = xdigs_lower;
				expchar = 'p';
			} else {
				ox[1] = 'X';
				xdigs = xdigs_upper;
				expchar = 'P';
			}
			/*
			 * XXX We don't actually have a conversion
			 * XXX routine for this yet.
			 */
			if (flags & LONGDBL) {
				fparg.ldbl = (double)GETARG(long double);
				dtoaresult = cp =
				    __hldtoa(fparg.ldbl, xdigs, prec,
				    &expt, &signflag, &dtoaend);
			} else {
				fparg.dbl = GETARG(double);
				dtoaresult = cp =
				    __hdtoa(fparg.dbl, xdigs, prec,
				    &expt, &signflag, &dtoaend);
			}
			goto fp_begin;
#endif
		case 'e':
		case 'E':
			expchar = ch;
			if (prec < 0)	/* account for digit before decpt */
				prec = DEFPREC + 1;
			else
				prec++;
			goto fp_begin;
		case 'f':
		case 'F':
			expchar = '\0';
			goto fp_begin;
		case 'g':
		case 'G':
			expchar = ch - ('g' - 'e');
			if (prec == 0)
				prec = 1;
fp_begin:
			if (prec < 0)
				prec = DEFPREC;
			if (dtoaresult != NULL)
				freedtoa(dtoaresult);
			if (flags & LONGDBL) {
				fparg.ldbl = GETARG(long double);
				dtoaresult = cp =
				    __ldtoa(&fparg.ldbl, expchar ? 2 : 3, prec,
				    &expt, &signflag, &dtoaend);
			} else {
				fparg.dbl = GETARG(double);
				dtoaresult = cp =
				    dtoa(fparg.dbl, expchar ? 2 : 3, prec,
				    &expt, &signflag, &dtoaend);
				if (expt == 9999)
					expt = INT_MAX;
			}
			if (signflag)
				sign = '-';
			if (expt == INT_MAX) {	/* inf or nan */
				if (*cp == 'N') {
					cp = (char*) ((ch >= 'a') ? "nan" : "NAN");
					sign = '\0';
				} else
					cp = (char*) ((ch >= 'a') ? "inf" : "INF");;
				size = 3;
				break;
			}
			flags |= FPT;
			ndig = dtoaend - cp;
			if (ch == 'g' || ch == 'G') {
				if (expt > -4 && expt <= prec) {
					/* Make %[gG] smell like %[fF] */
					expchar = '\0';
					if (flags & ALT)
						prec -= expt;
					else
						prec = ndig - expt;
					if (prec < 0)
						prec = 0;
				} else {
					/*
					 * Make %[gG] smell like %[eE], but
					 * trim trailing zeroes if no # flag.
					 */
					if (!(flags & ALT))
						prec = ndig;
				}
			}
			if (expchar) {
				expsize = exponent(expstr, expt - 1, expchar);
				size = expsize + prec;
				if (prec > 1 || flags & ALT)
					++size;
			} else {
				/* space for digits before decimal point */
				if (expt > 0)
					size = expt;
				else	/* "0" */
					size = 1;
				/* space for decimal pt and following digits */
				if (prec || flags & ALT)
					size += prec + 1;
				if (grouping && expt > 0) {
					/* space for thousands' grouping */
					nseps = nrepeats = 0;
					lead = expt;
					while (*grouping != CHAR_MAX) {
						if (lead <= *grouping)
							break;
						lead -= *grouping;
						if (*(grouping+1)) {
							nseps++;
							grouping++;
						} else
							nrepeats++;
					}
					size += nseps + nrepeats;
				} else
					lead = expt;
			}
			break;
#endif /* FLOATING_POINT */
		case 'n':
			/*
			 * Assignment-like behavior is specified if the
			 * value overflows or is otherwise unrepresentable.
			 * C99 says to use `signed char' for %hhn conversions.
			 */
			if (flags & LLONGINT)
				*GETARG(long long *) = ret;
			else if (flags & SIZET)
				*GETARG(ssize_t *) = (ssize_t)ret;
			else if (flags & PTRDIFFT)
				*GETARG(ptrdiff_t *) = ret;
			else if (flags & INTMAXT)
				*GETARG(intmax_t *) = ret;
			else if (flags & LONGINT)
				*GETARG(long *) = ret;
			else if (flags & SHORTINT)
				*GETARG(short *) = ret;
			else if (flags & CHARINT)
				*GETARG(signed char *) = ret;
			else
				*GETARG(int *) = ret;
			continue;	/* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			if (flags & INTMAX_SIZE)
				ujval = UJARG();
			else
				ulval = UARG();
			base = 8;
			goto nosign;
		case 'p':
			/*-
			 * If the format string is *%p, then treat
			 * the pointer as one of type Formatter* and
			 * call ptr->format().
			 *      -- NOT ANSI
			 */
			if (ret > 0 && str[-1] == '*') {
				str--;
				strsz++;
				ret--;

				fmtobj = GETARG(Formatter *);

                                if (! fmtobj) {
                                    strncpy(str, "(null)", strsz);
                                    sz = 6;
                                } else {
                                    Formatter::assert_valid(fmtobj);
                                    sz = fmtobj->format(str, strsz);
                                }
                                
				if (sz < strsz) {
					str += sz;
					strsz -= sz;
				} else {
					str += strsz;
					strsz = 0;
				}
				ret += sz;
				continue;
			}
			/*-
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */

			ujval = (uintmax_t)(uintptr_t)GETARG(void *);
			base = 16;
			xdigs = xdigs_lower;
			flags = flags | INTMAXT;
			ox[1] = 'x';
			goto nosign;
		case 'S':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 's':
			if (flags & LONGINT) {
				wchar_t *wcp;

				if (convbuf != NULL)
					free(convbuf);
				if ((wcp = GETARG(wchar_t *)) == NULL)
					cp = "(null)";
				else {
					convbuf = __wcsconv(wcp, prec);
					if (convbuf == NULL) {
//						fp->_flags |= __SERR;
						ret = EOF;
						goto error;
					}
					cp = convbuf;
				}
			} else if ((cp = GETARG(char *)) == NULL)
				cp = "(null)";
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				char *p = (char*)memchr(cp, 0, (size_t)prec);

				if (p != NULL) {
					size = p - cp;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = strlen(cp);
			sign = '\0';
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			if (flags & INTMAX_SIZE)
				ujval = UJARG();
			else
				ulval = UARG();
			base = 10;
			goto nosign;
		case 'X':
			xdigs = xdigs_upper;
			goto hex;
		case 'x':
			xdigs = xdigs_lower;
hex:
			if (flags & INTMAX_SIZE)
				ujval = UJARG();
			else
				ulval = UARG();
			base = 16;
			/* leading 0x/X only if non-zero */
			if (flags & ALT &&
			    (flags & INTMAX_SIZE ? ujval != 0 : ulval != 0))
				ox[1] = ch;

			flags &= ~GROUPING;
			/* unsigned conversions */
nosign:			sign = '\0';
			/*-
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*-
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			cp = buf + BUF;
			if (flags & INTMAX_SIZE) {
				if (ujval != 0 || prec != 0)
					cp = __ujtoa(ujval, cp, base,
					    flags & ALT, xdigs,
					    flags & GROUPING, thousands_sep,
					    grouping);
			} else {
				if (ulval != 0 || prec != 0)
					cp = __ultoa(ulval, cp, base,
					    flags & ALT, xdigs,
					    flags & GROUPING, thousands_sep,
					    grouping);
			}
			size = buf + BUF - cp;
			if (size > BUF)	/* should never happen */
				abort();
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = '\0';
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print the
		 * string proper, then emit zeroes required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * size excludes decimal prec; realsz includes it.
		 */
		realsz = dprec > size ? dprec : size;
		if (sign)
			realsz++;
		else if (ox[1])
			realsz += 2;

		prsize = width > realsz ? width : realsz;
		if ((unsigned)ret + prsize > INT_MAX) {
			ret = EOF;
			goto error;
		}

		/* right-adjusting blank padding */
		if ((flags & (LADJUST|ZEROPAD)) == 0)
			PAD(width - realsz, blanks);

		/* prefix */
		if (sign) {
			PRINT(&sign, 1);
		} else if (ox[1]) {	/* ox[1] is either x, X, or \0 */
			ox[0] = '0';
			PRINT(ox, 2);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD)
			PAD(width - realsz, zeroes);

		/* leading zeroes from decimal precision */
		PAD(dprec - size, zeroes);

		/* the string or number proper */
#ifdef FLOATING_POINT
		if ((flags & FPT) == 0) {
			PRINT(cp, size);
		} else {	/* glue together f_p fragments */
			if (!expchar) {	/* %[fF] or sufficiently short %[gG] */
				if (expt <= 0) {
					PRINT(zeroes, 1);
					if (prec || flags & ALT)
						PRINT(decimal_point, 1);
					PAD(-expt, zeroes);
					/* already handled initial 0's */
					prec += expt;
				} else {
					PRINTANDPAD(cp, dtoaend, lead, zeroes);
					cp += lead;
					if (grouping) {
						while (nseps>0 || nrepeats>0) {
							if (nrepeats > 0)
								nrepeats--;
							else {
								grouping--;
								nseps--;
							}
							PRINT(&thousands_sep,
							    1);
							PRINTANDPAD(cp,dtoaend,
							    *grouping, zeroes);
							cp += *grouping;
						}
						if (cp > dtoaend)
							cp = dtoaend;
					}
					if (prec || flags & ALT)
						PRINT(decimal_point,1);
				}
				PRINTANDPAD(cp, dtoaend, prec, zeroes);
			} else {	/* %[eE] or sufficiently long %[gG] */
				if (prec > 1 || flags & ALT) {
					buf[0] = *cp++;
					buf[1] = *decimal_point;
					PRINT(buf, 2);
					PRINT(cp, ndig-1);
					PAD(prec - ndig, zeroes);
				} else	/* XeYYY */
					PRINT(cp, 1);
				PRINT(expstr, expsize);
			}
		}
#else
		PRINT(cp, size);
#endif
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST)
			PAD(width - realsz, blanks);

		/* finally, adjust ret */
		ret += prsize;

		FLUSH();	/* copy out the I/O vectors */
	}
done:
	FLUSH();
error:
#ifdef FLOATING_POINT
	if (dtoaresult != NULL)
		freedtoa(dtoaresult);
#endif
	if (convbuf != NULL)
		free(convbuf);
// 	if (__sferror(fp))
// 		ret = EOF;
	if ((argtable != NULL) && (argtable != statargtable))
		free (argtable);

	*str = '\0';
	
	return (ret);
	/* NOTREACHED */
}

int
snprintf(char *str, size_t strsz, const char *fmt, ...) __THROW
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsnprintf(str, strsz, fmt, ap);
	va_end(ap);

	return ret;
}

/*
 * Find all arguments when a positional parameter is encountered.  Returns a
 * table, indexed by argument number, of pointers to each arguments.  The
 * initial argument table should be an array of STATIC_ARG_TBL_SIZE entries.
 * It will be replaces with a malloc-ed one if it overflows.
 */ 
static void
__find_arguments (const char *fmt0, va_list ap, union arg **argtable)
{
	char *fmt;		/* format string */
	int ch;			/* character from fmt */
	int n, n2;		/* handy integer (short term usage) */
	char *cp;		/* handy char pointer (short term usage) */
	int flags;		/* flags as above */
	int width;		/* width from format (%8d), or 0 */
	enum typeid_t *typetable; /* table of types */
	enum typeid_t stattypetable [STATIC_ARG_TBL_SIZE];
	int tablesize;		/* current size of type table */
	int tablemax;		/* largest used index in table */
	int nextarg;		/* 1-based argument index */

	/*
	 * Add an argument type to the table, expanding if necessary.
	 */
#define ADDTYPE(type) \
	((nextarg >= tablesize) ? \
		__grow_type_table(nextarg, &typetable, &tablesize) : 0, \
	(nextarg > tablemax) ? tablemax = nextarg : 0, \
	typetable[nextarg++] = type)

#define	ADDSARG() \
	((flags&INTMAXT) ? ADDTYPE(T_INTMAXT) : \
		((flags&SIZET) ? ADDTYPE(T_SIZET) : \
		((flags&PTRDIFFT) ? ADDTYPE(T_PTRDIFFT) : \
		((flags&LLONGINT) ? ADDTYPE(T_LLONG) : \
		((flags&LONGINT) ? ADDTYPE(T_LONG) : ADDTYPE(T_INT))))))

#define	ADDUARG() \
	((flags&INTMAXT) ? ADDTYPE(T_UINTMAXT) : \
		((flags&SIZET) ? ADDTYPE(T_SIZET) : \
		((flags&PTRDIFFT) ? ADDTYPE(T_PTRDIFFT) : \
		((flags&LLONGINT) ? ADDTYPE(T_U_LLONG) : \
		((flags&LONGINT) ? ADDTYPE(T_U_LONG) : ADDTYPE(T_U_INT))))))

	/*
	 * Add * arguments to the type array.
	 */
#define ADDASTER() \
	n2 = 0; \
	cp = fmt; \
	while (is_digit(*cp)) { \
		n2 = 10 * n2 + to_digit(*cp); \
		cp++; \
	} \
	if (*cp == '$') { \
		int hold = nextarg; \
		nextarg = n2; \
		ADDTYPE (T_INT); \
		nextarg = hold; \
		fmt = ++cp; \
	} else { \
		ADDTYPE (T_INT); \
	}
	fmt = (char *)fmt0;
	typetable = stattypetable;
	tablesize = STATIC_ARG_TBL_SIZE;
	tablemax = 0; 
	nextarg = 1;
	memset (typetable, T_UNUSED, STATIC_ARG_TBL_SIZE);

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
			/* void */;
		if (ch == '\0')
			goto done;
		fmt++;		/* skip over '%' */

		flags = 0;
		width = 0;

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		case ' ':
		case '#':
			goto rflag;
		case '*':
			ADDASTER ();
			goto rflag;
		case '-':
		case '+':
		case '\'':
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				ADDASTER ();
				goto rflag;
			}
			while (is_digit(ch)) {
				ch = *fmt++;
			}
			goto reswitch;
		case '0':
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			} while (is_digit(ch));
			if (ch == '$') {
				nextarg = n;
				goto rflag;
			}
			width = n;
			goto reswitch;
#ifdef FLOATING_POINT
		case 'L':
			flags |= LONGDBL;
			goto rflag;
#endif
		case 'h':
			if (flags & SHORTINT) {
				flags &= ~SHORTINT;
				flags |= CHARINT;
			} else
				flags |= SHORTINT;
			goto rflag;
		case 'j':
			flags |= INTMAXT;
			goto rflag;
		case 'l':
			if (flags & LONGINT) {
				flags &= ~LONGINT;
				flags |= LLONGINT;
			} else
				flags |= LONGINT;
			goto rflag;
		case 'q':
			flags |= LLONGINT;	/* not necessarily */
			goto rflag;
		case 't':
			flags |= PTRDIFFT;
			goto rflag;
		case 'z':
			flags |= SIZET;
			goto rflag;
		case 'C':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'c':
			if (flags & LONGINT)
				ADDTYPE(T_WINT);
			else
				ADDTYPE(T_INT);
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			ADDSARG();
			break;
#ifdef FLOATING_POINT
#ifdef HEXFLOAT
		case 'a':
		case 'A':
#endif
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			if (flags & LONGDBL)
				ADDTYPE(T_LONG_DOUBLE);
			else
				ADDTYPE(T_DOUBLE);
			break;
#endif /* FLOATING_POINT */
		case 'n':
			if (flags & INTMAXT)
				ADDTYPE(TP_INTMAXT);
			else if (flags & PTRDIFFT)
				ADDTYPE(TP_PTRDIFFT);
			else if (flags & SIZET)
				ADDTYPE(TP_SIZET);
			else if (flags & LLONGINT)
				ADDTYPE(TP_LLONG);
			else if (flags & LONGINT)
				ADDTYPE(TP_LONG);
			else if (flags & SHORTINT)
				ADDTYPE(TP_SHORT);
			else if (flags & CHARINT)
				ADDTYPE(TP_SCHAR);
			else
				ADDTYPE(TP_INT);
			continue;	/* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			ADDUARG();
			break;
		case 'p':
			ADDTYPE(TP_VOID);
			break;
		case 'S':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 's':
			if (flags & LONGINT)
				ADDTYPE(TP_WCHAR);
			else
				ADDTYPE(TP_CHAR);
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
		case 'X':
		case 'x':
			ADDUARG();
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			break;
		}
	}
done:
	/*
	 * Build the argument table.
	 */
	if (tablemax >= STATIC_ARG_TBL_SIZE) {
		*argtable = (union arg *)
		    malloc (sizeof (union arg) * (tablemax + 1));
	}

	(*argtable) [0].intarg = 0;
	for (n = 1; n <= tablemax; n++) {
		switch (typetable [n]) {
		    case T_UNUSED: /* whoops! */
			(*argtable) [n].intarg = va_arg (ap, int);
			break;
		    case TP_SCHAR:
			(*argtable) [n].pschararg = va_arg (ap, signed char *);
			break;
		    case TP_SHORT:
			(*argtable) [n].pshortarg = va_arg (ap, short *);
			break;
		    case T_INT:
			(*argtable) [n].intarg = va_arg (ap, int);
			break;
		    case T_U_INT:
			(*argtable) [n].uintarg = va_arg (ap, unsigned int);
			break;
		    case TP_INT:
			(*argtable) [n].pintarg = va_arg (ap, int *);
			break;
		    case T_LONG:
			(*argtable) [n].longarg = va_arg (ap, long);
			break;
		    case T_U_LONG:
			(*argtable) [n].ulongarg = va_arg (ap, unsigned long);
			break;
		    case TP_LONG:
			(*argtable) [n].plongarg = va_arg (ap, long *);
			break;
		    case T_LLONG:
			(*argtable) [n].longlongarg = va_arg (ap, long long);
			break;
		    case T_U_LLONG:
			(*argtable) [n].ulonglongarg = va_arg (ap, unsigned long long);
			break;
		    case TP_LLONG:
			(*argtable) [n].plonglongarg = va_arg (ap, long long *);
			break;
		    case T_PTRDIFFT:
			(*argtable) [n].ptrdiffarg = va_arg (ap, ptrdiff_t);
			break;
		    case TP_PTRDIFFT:
			(*argtable) [n].pptrdiffarg = va_arg (ap, ptrdiff_t *);
			break;
		    case T_SIZET:
			(*argtable) [n].sizearg = va_arg (ap, size_t);
			break;
		    case TP_SIZET:
			(*argtable) [n].psizearg = va_arg (ap, size_t *);
			break;
		    case T_INTMAXT:
			(*argtable) [n].intmaxarg = va_arg (ap, intmax_t);
			break;
		    case T_UINTMAXT:
			(*argtable) [n].uintmaxarg = va_arg (ap, uintmax_t);
			break;
		    case TP_INTMAXT:
			(*argtable) [n].pintmaxarg = va_arg (ap, intmax_t *);
			break;
#ifdef FLOATING_POINT
		    case T_DOUBLE:
			(*argtable) [n].doublearg = va_arg (ap, double);
			break;
		    case T_LONG_DOUBLE:
			(*argtable) [n].longdoublearg = va_arg (ap, long double);
			break;
#endif
		    case TP_CHAR:
			(*argtable) [n].pchararg = va_arg (ap, char *);
			break;
		    case TP_VOID:
			(*argtable) [n].pvoidarg = va_arg (ap, void *);
			break;
		    case T_WINT:
			(*argtable) [n].wintarg = va_arg (ap, wint_t);
			break;
		    case TP_WCHAR:
			(*argtable) [n].pwchararg = va_arg (ap, wchar_t *);
			break;
		}
	}

	if ((typetable != NULL) && (typetable != stattypetable))
		free (typetable);
}

/*
 * Increase the size of the type table.
 */
static int
__grow_type_table (int nextarg, enum typeid_t **typetable, int *tablesize)
{
	enum typeid_t *const oldtable = *typetable;
	const int oldsize = *tablesize;
	enum typeid_t *newtable;
	int newsize = oldsize * 2;

	if (newsize < nextarg + 1)
		newsize = nextarg + 1;
	if (oldsize == STATIC_ARG_TBL_SIZE) {
		if ((newtable = (enum typeid_t*)malloc(newsize)) == NULL)
			abort();			/* XXX handle better */
		bcopy(oldtable, newtable, oldsize);
	} else {
		if ((newtable = (enum typeid_t*)reallocf(oldtable, newsize)) == NULL)
			abort();			/* XXX handle better */
	}
	memset(&newtable[oldsize], T_UNUSED, newsize - oldsize);

	*typetable = newtable;
	*tablesize = newsize;
        return 0;
}


#ifdef FLOATING_POINT

static int
exponent(char *p0, int exp, int fmtch)
{
	char *p, *t;
	char expbuf[MAXEXPDIG];

	p = p0;
	*p++ = fmtch;
	if (exp < 0) {
		exp = -exp;
		*p++ = '-';
	}
	else
		*p++ = '+';
	t = expbuf + MAXEXPDIG;
	if (exp > 9) {
		do {
			*--t = to_char(exp % 10);
		} while ((exp /= 10) > 9);
		*--t = to_char(exp);
		for (; t < expbuf + MAXEXPDIG; *p++ = *t++);
	}
	else {
		/*
		 * Exponents for decimal floating point conversions
		 * (%[eEgG]) must be at least two characters long,
		 * whereas exponents for hexadecimal conversions can
		 * be only one character long.
		 */
		if (fmtch == 'e' || fmtch == 'E')
			*p++ = '0';
		*p++ = to_char(exp);
	}
	return (p - p0);
}

#endif /* FLOATING_POINT */

#ifdef __FORMATTER__

/**
 * The implementation of __ldtoa is included here also from FreeBSD
 * 5.0, in /usr/src/lib/libc/stdio/_ldtoa.c. In addition, the i386
 * version of fpmath.h is also included here.
 */

union IEEEl2bits {
	long double	e;
	struct {
		unsigned int	manl	:32;
		unsigned int	manh	:32;
		unsigned int	exp	:15;
		unsigned int	sign	:1;
		unsigned int	junk	:16;
	} bits;
};

#define	mask_nbit_l(u)	((u).bits.manh &= 0x7fffffff)

#define	LDBL_TO_ARRAY32(u, a) do {			\
	(a)[0] = (uint32_t)(u).bits.manl;		\
	(a)[1] = (uint32_t)(u).bits.manh;		\
} while(0)

/*-
 * Copyright (c) 2003 David Schultz <das@FreeBSD.ORG>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include "gdtoaimp.h"

/*
 * ldtoa() is a wrapper for gdtoa() that makes it smell like dtoa(),
 * except that the floating point argument is passed by reference.
 * When dtoa() is passed a NaN or infinity, it sets expt to 9999.
 * However, a long double could have a valid exponent of 9999, so we
 * use INT_MAX in ldtoa() instead.
 */
static char *
__ldtoa(long double *ld, int mode, int ndigits, int *decpt, int *sign,
    char **rve)
{
	static FPI fpi = {
		LDBL_MANT_DIG,			/* nbits */
		LDBL_MIN_EXP - LDBL_MANT_DIG,	/* emin */
		LDBL_MAX_EXP - LDBL_MANT_DIG,	/* emax */
		FPI_Round_near,	       		/* rounding */
#ifdef Sudden_Underflow	/* unused, but correct anyway */
		1
#else
		0
#endif
	};
	int be, kind;
	char *ret;
	union IEEEl2bits u;
	uint32_t bits[(LDBL_MANT_DIG + 31) / 32];

	u.e = *ld;
	*sign = u.bits.sign;
	be = u.bits.exp - (LDBL_MAX_EXP - 1) - (LDBL_MANT_DIG - 1);
	LDBL_TO_ARRAY32(u, bits);

	switch (fpclassify(u.e)) {
	case FP_NORMAL:
		kind = STRTOG_Normal;
#ifdef	LDBL_IMPLICIT_NBIT
		bits[LDBL_MANT_DIG / 32] |= 1 << ((LDBL_MANT_DIG - 1) % 32);
#endif /* LDBL_IMPLICIT_NBIT */
		break;
	case FP_ZERO:
		kind = STRTOG_Zero;
		break;
	case FP_SUBNORMAL:
		kind = STRTOG_Denormal;
		break;
	case FP_INFINITE:
		kind = STRTOG_Infinite;
		break;
	case FP_NAN:
		kind = STRTOG_NaN;
		break;
	default:
		abort();
	}

	ret = gdtoa(&fpi, be, (ULong *)bits, &kind, mode, ndigits, decpt, rve);
	if (*decpt == -32768)
		*decpt = INT_MAX;
	return ret;
}

#endif
