/*
 * Copyright (c) Tord Lindström and Khaled Daham
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
 * 3. The name of the author(s) may not be used to endorse or promote products 
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <string.h>
#include "linkproto_stub.h"

int
pko_dump2pc_req(int sock) {
    unsigned short len;
    pko_pkt_dump2pc_req req;
    len = sizeof(pko_pkt_dump2pc_req);
    req.cmd = htonl(PKO_NETDUMP_CMD);
    req.len = htons(len);
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}

int
pko_dump2screen_req(int sock) {
    unsigned short len;
    pko_pkt_dump2screen_req req;
    len = sizeof(pko_pkt_dump2screen_req);
    req.cmd = htonl(PKO_SCRDUMP_CMD);
    req.len = htons(len);
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}

int
pko_dumpmemory_req(int sock, char *file, unsigned int offset, unsigned int size) {
    unsigned short len;
    pko_pkt_dumpmem_req req;
    len = sizeof(pko_pkt_dumpmem_req);
    req.cmd = htonl(PKO_DUMPMEM_CMD);
    req.len = htons(len);
    req.offset = htonl(offset);
    req.size = htonl(size);
    strncpy(req.argv, file, PKO_MAX_PATH);
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}

int
pko_dumpregs_req(int sock, char *file, unsigned int regs) {
    unsigned short len;
    pko_pkt_dumpregs_req req;
    len = sizeof(pko_pkt_dumpregs_req);
    req.cmd = htonl(PKO_DUMPREGS_CMD);
    req.len = htons(len);
    req.regs = htonl(regs);
    strncpy(req.argv, file, PKO_MAX_PATH);
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}

int
pko_execee_req(int sock, char *argv, unsigned int argvlen, unsigned int argc) {
    unsigned short len;
    pko_pkt_execee_req req;
    len = sizeof(pko_pkt_execee_req);
    req.cmd = htonl(PKO_EXECEE_CMD);
    req.len = htons(len);
    req.argc = htonl(argc);
    memcpy(req.argv, argv, argvlen);
    req.argv[argvlen] = '\0';
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}

int
pko_execiop_req(int sock, char *argv, unsigned int argvlen, unsigned int argc) {
    unsigned short len;
    pko_pkt_execiop_req req;
    len = sizeof(pko_pkt_execiop_req);
    req.cmd = htonl(PKO_EXECIOP_CMD);
    req.len = htons(len);
    req.argc = htonl(argc);
    memcpy(req.argv, argv, argvlen);
    req.argv[argvlen] = '\0';
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}

int
pko_gsexec_req(int sock, char *file, unsigned int size) {
    unsigned short len;
    pko_pkt_gsexec_req req;
    len = sizeof(pko_pkt_gsexec_req);
    req.cmd = htonl(PKO_GSEXEC_CMD);
    req.len = htons(len);
    req.size = htons(size);
    memcpy(req.file, file, PKO_MAX_PATH);
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}

int
pko_poweroff_req(int sock) {
    unsigned short len;
    pko_pkt_poweroff_req req;
    len = sizeof(pko_pkt_poweroff_req);
    req.cmd = htonl(PKO_POWEROFF_CMD);
    req.len = htons(len);
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}

int
pko_reset_req(int sock) {
    unsigned short len;
    pko_pkt_reset_req req;
    len = sizeof(pko_pkt_reset_req);
    req.cmd = htonl(PKO_RESET_CMD);
    req.len = htons(len);
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}

int
pko_stop_vu(int sock, unsigned int vpu) {
    unsigned short len;
    pko_pkt_stopvu_req req;
    len = sizeof(pko_pkt_stopvu_req);
    req.cmd = htonl(PKO_STOPVU_CMD);
    req.len = htons(len);
    req.vpu = htons(vpu);
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}

int
pko_start_vu(int sock, unsigned int vpu) {
    unsigned short len;
    pko_pkt_startvu_req req;
    len = sizeof(pko_pkt_startvu_req);
    req.cmd = htonl(PKO_STARTVU_CMD);
    req.len = htons(len);
    req.vpu = htons(vpu);
#ifndef WIN32
    return send(sock, &req, len, 0);
#else
    return send(sock, (const char *)&req, len, 0);
#endif
}
