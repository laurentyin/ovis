/* -*- c-basic-offset: 8 -*-
 * Copyright (c) 2016 Open Grid Computing, Inc. All rights reserved.
 * Copyright (c) 2016 Sandia Corporation. All rights reserved.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Export of this program may require a license from the United States
 * Government.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the BSD-type
 * license below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *      Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *      Neither the name of Sandia nor the names of any contributors may
 *      be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *      Neither the name of Open Grid Computing nor the names of any
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *      Modified source versions must be plainly marked as such, and
 *      must not be misrepresented as being the original software.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __TIMER_BASE_H
#define __TIMER_BASE_H

#include "ldms.h"
#include "ldmsd.h"
#include "tsampler.h"
#include <sys/queue.h>
#include <pthread.h>

struct tsampler_timer_entry {
	struct tsampler_timer timer;
	TAILQ_ENTRY(tsampler_timer_entry) entry;
};

struct timer_base {
	struct ldmsd_sampler base;
	enum {
		ST_INIT,
		ST_CONFIGURED,
		ST_RUNNING,
	} state;
	ldms_set_t set;
	ldms_schema_t schema;
	uint64_t compid;
	pthread_mutex_t mutex;
	TAILQ_HEAD(, tsampler_timer_entry) timer_list;
	char buff[1024]; /* string buffer for internal timer_base use */
	char iname[1024]; /* iname for internal use */
	char pname[1024]; /* producer name for internal use */
};

void timer_base_init(struct timer_base *tb);

int timer_base_config(struct ldmsd_plugin *self, struct attr_value_list *kwl,
		struct attr_value_list *avl);

int timer_base_create_set(struct timer_base *tb);

ldms_set_t timer_base_get_set(struct ldmsd_sampler *self);

void timer_base_term(struct ldmsd_plugin *self);

int timer_base_sample(struct ldmsd_sampler *self);

void timer_base_cleanup(struct timer_base *tb);

int timer_base_add_hfmetric(struct timer_base *tb,
				const char *name,
				enum ldms_value_type type,
				int n,
				const struct timeval *interval,
				tsampler_sample_cb cb,
				void *ctxt);
#endif
