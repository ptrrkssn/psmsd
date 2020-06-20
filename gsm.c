/*
* gsm.c - GSM character set handling routines
*
* Copyright (c) 2016-2020 Peter Eriksson <pen@lysator.liu.se>
 *
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gsm.h"


static struct {
    char c;
    int v;
} latin1tab[] =
    {
	{ '@', 0x00 },
	{ 0xA3, 0x01 }, /* 0xA3 '£' */
	{ '$', 0x02 },
	{ 0xA5, 0x03 }, /* 0xA5 '¥' */
	{ 0xE8, 0x04 }, /* 0xE8 'è' */
	{ 0xE9, 0x05 }, /* 0xE9 'é' */
	{ 0xF9, 0x06 }, /* 0xF9 'ù' */
	{ 0xEC, 0x07 }, /* 0xEC 'ì' */
	{ 0xF2, 0x08 }, /* 0xF2 'ò' */
	{ 0xC7, 0x09 }, /* 0xC7 'Ç' */
	{ 10,  0x0A },
	{ 0xD8, 0x0B }, /* 0xD8 'Ø' */
	{ 0xF8, 0x0C }, /* 0xF8 'ø' */
	{ 13,  0x0D },
	{ 0xC5, 0x0E }, /* 0xC5 'Å' */
	{ 0xE5, 0x0F }, /* 0xE5 'å' */

	{ '_', 0x11 },
	
	{ 0xC6, 0x1C }, /* 0xC6 'Æ' */
	{ 0xE6, 0x1D }, /* 0xE6 'æ' */
	
	{ 0xC9, 0x1F }, /* 0xC9 'É' */

	{ ' ', 0x20 },
	{ '!', 0x21 },
	{ '"', 0x22 },
	{ '#', 0x23 },
	{ 0xA4, 0x24}, /* 0xA4 '¤' */
	{ '%', 0x25 },
	{ '&', 0x26 },
	{ '\'',0x27 },
	{ '(', 0x28 },
	{ ')', 0x29 },
	{ '*', 0x2a },
	{ '+', 0x2b },
	{ ',', 0x2c },
	{ '-', 0x2d },
	{ '.', 0x2e },
	{ '/', 0x2f },
	{ '0', 0x30 },
	{ '1', 0x31 },
	{ '2', 0x32 },
	{ '3', 0x33 },
	{ '4', 0x34 },
	{ '5', 0x35 },
	{ '6', 0x36 },
	{ '7', 0x37 },
	{ '8', 0x38 },
	{ '9', 0x39 },
	{ ':', 0x3a },
	{ ';', 0x3b },
	{ '<', 0x3c },
	{ '=', 0x3d },
	{ '>', 0x3e },
	{ '?', 0x3f },
	{ 0xA1, 0x40}, /* 0xA1 '¡' */
	{ 'A', 0x41 },
	{ 'B', 0x42 },
	{ 'C', 0x43 },
	{ 'D', 0x44 },
	{ 'E', 0x45 },
	{ 'F', 0x46 },
	{ 'G', 0x47 },
	{ 'H', 0x48 },
	{ 'I', 0x49 },
	{ 'J', 0x4a },
	{ 'K', 0x4b },
	{ 'L', 0x4c },
	{ 'M', 0x4d },
	{ 'N', 0x4e },
	{ 'O', 0x4f },
	{ 'P', 0x50 },
	{ 'Q', 0x51 },
	{ 'R', 0x52 },
	{ 'S', 0x53 },
	{ 'T', 0x54 },
	{ 'U', 0x55 },
	{ 'V', 0x56 },
	{ 'W', 0x57 },
	{ 'X', 0x58 },
	{ 'Y', 0x59 },
	{ 'Z', 0x5a },
	{ 0xC4, 0x5b }, /* 0xC4 'Ä' */
	{ 0xD6, 0x5c }, /* 0xD6 'Ö' */
	{ 0xD1, 0x5d }, /* 0xD1 'Ñ' */
	{ 0xDC, 0x5e }, /* 0xDC 'Ü' */
	{ 0xA7, 0x5f }, /* 0xA7 '§' */

	{ 'a', 0x61 },
	{ 'b', 0x62 },
	{ 'c', 0x63 },
	{ 'd', 0x64 },
	{ 'e', 0x65 },
	{ 'f', 0x66 },
	{ 'g', 0x67 },
	{ 'h', 0x68 },
	{ 'i', 0x69 },
	{ 'j', 0x6a },
	{ 'k', 0x6b },
	{ 'l', 0x6c },
	{ 'm', 0x6d },
	{ 'n', 0x6e },
	{ 'o', 0x6f },
	{ 'p', 0x70 },
	{ 'q', 0x71 },
	{ 'r', 0x72 },
	{ 's', 0x73 },
	{ 't', 0x74 },
	{ 'u', 0x75 },
	{ 'v', 0x76 },
	{ 'w', 0x77 },
	{ 'x', 0x78 },
	{ 'y', 0x79 },
	{ 'z', 0x7a },
	{ 0xE4, 0x7b }, /* 0xE4 'ä' */
	{ 0xF6, 0x7c }, /* 0xF6 'ö' */
	{ 0xF1, 0x7d }, /* 0xF1 'ñ' */
	{ 0xFC, 0x7e }, /* 0xFC 'ü' */
	{ 0xE0, 0x7f }, /* 0xE0 'à' */

	{ 0xA7, 0x1B65 },  /* 0xA7 '§' */
	{ 12,  0x1B0A },
	{ '[', 0x1B3C },
	{ '\\' , 0x1B2F },
	{ ']', 0x1B3E },
	{ '^', 0x1B14 },
	{ '{', 0x1B28 },
	{ '|', 0x1B40 },
	{ '}', 0x1B29 },
	{ '~', 0x1B3D },
	{ 0, 0 }
    };


static int
lookup_latin1_to_gsm(int c)
{
    int i;

    for (i = 0; latin1tab[i].c != 0 && latin1tab[i].c != c; i++)
	;

    if (latin1tab[i].c)
	return latin1tab[i].v;

    return -1;
}

static int
lookup_gsm_to_latin1(int v)
{
    int i;
    
    for (i = 0; latin1tab[i].c != 0 && latin1tab[i].v != v; i++)
	;

    if (latin1tab[i].c)
	return latin1tab[i].c;

    return -1;
}


char *
latin1_to_gsm(const char *ls,
	      char *buf,
	      int bufsize)
{
    int i, j, c;


    for (i = j = 0; j+3 < bufsize-2 && ls[i]; i++)
    {
	c = lookup_latin1_to_gsm(ls[i]);
	if (c != -1)
	{
	    if (c > 0xFF)
		snprintf(buf+j, bufsize-j, "%04X", c);
	    else
		snprintf(buf+j, bufsize-j, "%02X", c);
	    while (buf[j])
		++j;
	}
    }

    buf[j] = '\0';
    return buf;
}

char *
gsm_to_latin1(const char *gs,
	      char *buf,
	      int bufsize)
{
    int i, j, c, v;


    for (i = j = 0; gs[i] && sscanf(gs+i, "%2x", &v) == 1 && j < bufsize-1; i += 2)
    {
	if (v == 0x1B && sscanf(gs+i+2, "%2x", &v) == 1)
	{
	    v |= 0x1B00;
	    i += 2;
	}

	c = lookup_gsm_to_latin1(v);
	if (c != -1)
	    buf[j++] = c;
    }
    buf[j] = '\0';
    
    return buf;
}


#ifdef MAIN
int
main(int argc,
     char *argv[])
{
    char buf[1024], buf2[1024];
    int buflen;
    int i, v, len;

    if (argc < 2)
    {
	fprintf(stderr, "Usage: %s [-r | -t]\n");
	exit(1);
    }
    
    if (strcmp(argv[1], "-t") == 0)
    {
	while (fgets(buf, sizeof(buf), stdin))
	{
	    len = latin1_to_gsm(buf, buf2, sizeof(buf2));
	    for (i = 0; i < len; i++)
		printf("%02X", buf2[i]);
	}
	putchar('\n');
    }
    else
    {
	i = 0;
	
	while (scanf("%02x", &v) == 1)
	    buf[i++] = v;
	
	puts(gsm_to_latin1(buf, i, buf2, sizeof(buf2)));
    }
    return 0;
}
#endif
