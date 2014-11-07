/*
 * Copyright (c) 2014, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Disque nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __DISQUE_JOBS_H
#define __DISQUE_JOBS_H

#define JOB_ID_LEN 40   /* 40 bytes hex, actually 20 bytes / 160 bits. */

/* This represent a Job across the system.
 *
 * The Job ID is the unique identifier of the job, both in the client
 * protocol and in the cluster messages between nodes.
 *
 * Times:
 *
 * When the expire time is reached, the job can be destroied even if it
 * was not successfully processed. The requeue time is the amount of time
 * that should elapse for this job to be queued again (put into an active
 * queue), if it was not yet processed. The queue time is the unix time at
 * which the job was queued last time.
 *
 * Note that nodes receiving the job from other nodes via REPLJOB messages
 * set their local time as ctime and etime (they recompute the expire time
 * doing etime-ctime in the received fields).
 *
 * List of nodes and ACKs garbage collection:
 *
 * We keep a list of nodes that *may* have the message, so that the creating
 * node is able to gargage collect ACKs even if not all the nodes in the
 * cluster are reachable, but only the nodes that may have a copy of this
 * job. The list includes nodes that we send the message to but never received
 * the confirmation, this is why we can have more listed nodes than
 * the 'repl' count.
 *
 * This optimized GC is possible when a client ACKs the message or when we
 * receive a SETACK message from another node. Nodes having just the
 * ACK but not a copy of the job, need to go for the usual path for
 * ACKs GC, that need a confirmation from all the nodes.
 *
 * Body:
 *
 * The body can be anything, including the empty string. Disque is
 * totally content-agnostic. */

#define JOB_STATE_ACTIVE  0  /* Not acked, this node never queued it. */
#define JOB_STATE_QUEUED  1  /* Not acked, and queued. */
#define JOB_STATE_WAITACK 2  /* Not acked, queued, delivered, no ACK. */
#define JOB_STATE_ACKED   3  /* Acked, no longer active, to garbage collect. */

struct job {
    int state;              /* JOB_STATE_* states. */
    char id[JOB_ID_LEN];    /* Job ID. */
    uint32_t ctime;         /* Job creation time, local node clock. */
    uint32_t etime;         /* Job expire time. */
    uint32_t qtime;         /* Job queued time: unix time job was queued. */
    uint32_t rtime;         /* Job re-queue time: re-queue period in seconds. */
    uint16_t repl;          /* Replication factor. */
    uint16_t numnodes;      /* Number of nodes that may have a copy. */
    uint64_t bodylen;       /* Job body length in bytes. */
    /* The following fields are dynamically allocated depending on
     * 'numnodes' and 'bodylen' fields.
     * --------------------------------------------------
     * List of nodes: numnodes * NODE_ID_LEN entries
     * Bocy: actual body, size is according to bodylen. */
};

#endif
