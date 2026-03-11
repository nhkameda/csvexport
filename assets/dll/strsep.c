/*
 * strsep() implementation for systems that don't have it.
 *
 * Copyright (c) 1990, 1993
 *  The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2018
 *  CompuPhase, The Netherlands.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
#include <stdio.h>
#include "strsep.h"

/** strsep()
 * Get next token from string *stringp, where tokens are possibly-empty strings
 * separated by characters from delim.
 *
 * Writes EOS into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 *
 * If the token at *stringp starts with a double-quote and STRSEP_QUOTETOKEN is
 * set, embedded delimiters between the double quotes are skipped.
 *
 * If STRSEP_SKIPWHITE is set, leading whitespace is skipped (if delim includes
 * a space, there is no trailing whitespace either).
 *
 * On return, *stringp points past the last EOS written (if there might be
 * further tokens), or is NULL (if there are definitely no more tokens).
 * If stringp is NULL, strsep returns NULL.
 * The combination of these rules makes that strsep() has a tendency of
 * returning an empty string after all tokens, and returns NULL when called once
 * more.
 */
char *strsep(char **stringp, const char *delim,int flags)
{
	register char *s;
	char *tok;

	if (*stringp == NULL)
		return NULL;

	s = *stringp;
	if (flags & STRSEP_SKIPWHITE) {
		while (*s != '\0' && *s <= ' ')
			s++;
	}

	tok = s;
	if (*s == '"' && (flags & STRSEP_QUOTETOKEN)) {
		/* skip up to the closing double quote */
		s++;
		while (*s != '"' || *(s + 1) == '"') {
			if ((*s == '"' || *s == '\\') && *(s + 1) == '"')
				s++;	/* skip escaped embedded quotes */
			s++;
		}
		if (*s == '"')
			s++;		/* include trailing quote */
	}

	for ( ;; ) {
		register int c = *s++;
		register const char *spanp = delim;
		register int sc;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return tok;
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}
