/*
 * serial.h - Serial I/O handling routines
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

#ifndef SERIAL_H
#define SERIAL_H

#define SERIAL_E_OK                      0
#define SERIAL_E_UNIX_ERROR             -1
#define SERIAL_E_INVALID_SPEED          -2
#define SERIAL_E_INVALID_DEVICE         -3
#define SERIAL_E_LOCK_FAILED            -4
#define SERIAL_E_OPEN_FAILED            -5
#define SERIAL_E_GETATTR_FAILED         -6
#define SERIAL_E_SETSPEED_FAILED        -7
#define SERIAL_E_SETATTR_FAILED         -8

extern const char *
serial_strerror(int code);

extern void
serial_close(int fd);

extern int
serial_open(const char *devname,
	    int speed,
	    int timeout);

extern int
serial_read(int fd,
	    char *buf,
	    int bufsize,
	    int timeout);

extern int
serial_write(int fd,
	     const char *buf,
	     int bufsize,
	     int timeout);




#endif
