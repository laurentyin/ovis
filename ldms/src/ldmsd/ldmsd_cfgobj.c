/* -*- c-basic-offset: 8 -*-
 * Copyright (c) 2015 Open Grid Computing, Inc. All rights reserved.
 * Copyright (c) 2015 Sandia Corporation. All rights reserved.
 *
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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <errno.h>
#include <coll/rbt.h>
#include "ldmsd.h"

int cfgobj_cmp(void *a, const void *b)
{
	return strcmp(a, b);
}

struct rbt prdcr_tree = RBT_INITIALIZER(cfgobj_cmp);
pthread_mutex_t prdcr_tree_lock = PTHREAD_MUTEX_INITIALIZER;

struct rbt updtr_tree = RBT_INITIALIZER(cfgobj_cmp);
pthread_mutex_t updtr_tree_lock = PTHREAD_MUTEX_INITIALIZER;

struct rbt strgp_tree = RBT_INITIALIZER(cfgobj_cmp);
pthread_mutex_t strgp_tree_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t *cfgobj_locks[] = {
	[LDMSD_CFGOBJ_PRDCR] = &prdcr_tree_lock,
	[LDMSD_CFGOBJ_UPDTR] = &updtr_tree_lock,
	[LDMSD_CFGOBJ_STRGP] = &strgp_tree_lock,
};

struct rbt *cfgobj_trees[] = {
	[LDMSD_CFGOBJ_PRDCR] = &prdcr_tree,
	[LDMSD_CFGOBJ_UPDTR] = &updtr_tree,
	[LDMSD_CFGOBJ_STRGP] = &strgp_tree,
};

void ldmsd_cfgobj_init(void)
{
	rbt_init(&prdcr_tree, cfgobj_cmp);
	rbt_init(&updtr_tree, cfgobj_cmp);
	rbt_init(&strgp_tree, cfgobj_cmp);
}

void ldmsd_cfgobj___del(ldmsd_cfgobj_t obj)
{
	ldmsd_cfgobj_type_t t = obj->type;
	struct rbn *n;
	pthread_mutex_lock(cfgobj_locks[t]);
	n = rbt_find(cfgobj_trees[t], obj->name);
	if (n)
		rbt_del(cfgobj_trees[t], &obj->rbn);
	free(obj->name);
	free(obj);
	pthread_mutex_unlock(cfgobj_locks[t]);
}

void ldmsd_cfg_lock(ldmsd_cfgobj_type_t type)
{
	pthread_mutex_lock(cfgobj_locks[type]);
}

void ldmsd_cfg_unlock(ldmsd_cfgobj_type_t type)
{
	pthread_mutex_unlock(cfgobj_locks[type]);
}

void ldmsd_cfgobj_lock(ldmsd_cfgobj_t obj)
{
	pthread_mutex_lock(&obj->lock);
}

void ldmsd_cfgobj_unlock(ldmsd_cfgobj_t obj)
{
	pthread_mutex_unlock(&obj->lock);
}

/**
 * Allocate a configuration object of the requested size. A
 * configuration object with the same name and type must not already
 * exist. On success, the object is returned locked.
 */
ldmsd_cfgobj_t ldmsd_cfgobj_new(const char *name, ldmsd_cfgobj_type_t type, size_t obj_size,
				ldmsd_cfgobj_del_fn_t __del)
{
	ldmsd_cfgobj_t obj = NULL;

	pthread_mutex_lock(cfgobj_locks[type]);

	errno = EEXIST;
	struct rbn *n = rbt_find(cfgobj_trees[type], name);
	if (n)
		goto out_1;

	errno = ENOMEM;
	obj = calloc(1, obj_size);
	if (!obj)
		goto out_1;
	obj->name = strdup(name);
	if (!obj->name)
		goto out_2;

	obj->type = type;
	obj->ref_count = 1;
	if (__del)
		obj->__del = __del;
	else
		obj->__del = ldmsd_cfgobj___del;

	pthread_mutex_init(&obj->lock, NULL);
	pthread_mutex_lock(&obj->lock);
	rbn_init(&obj->rbn, obj->name);
	rbt_ins(cfgobj_trees[type], &obj->rbn);
	goto out_1;

out_2:
	free(obj);
	obj = NULL;

out_1:
	pthread_mutex_unlock(cfgobj_locks[type]);
	return obj;
}

ldmsd_cfgobj_t ldmsd_cfgobj_get(ldmsd_cfgobj_t obj)
{
	if (obj)
		__sync_fetch_and_add(&obj->ref_count, 1);
	return obj;
}

void ldmsd_cfgobj_put(ldmsd_cfgobj_t obj)
{
	if (!obj)
		return;
	if (0 == __sync_sub_and_fetch(&obj->ref_count, 1))
		obj->__del(obj);
}

/** This function is only useful if the cfgobj lock is held when the function is called. */
int ldmsd_cfgobj_refcount(ldmsd_cfgobj_t obj)
{
	return obj->ref_count;
}

ldmsd_cfgobj_t __cfgobj_find(const char *name, ldmsd_cfgobj_type_t type)
{
	ldmsd_cfgobj_t obj = NULL;
	struct rbn *n = rbt_find(cfgobj_trees[type], name);
	if (!n)
		goto out;
	obj = container_of(n, struct ldmsd_cfgobj, rbn);
out:
	return ldmsd_cfgobj_get(obj);
}

ldmsd_cfgobj_t ldmsd_cfgobj_find(const char *name, ldmsd_cfgobj_type_t type)
{
	ldmsd_cfgobj_t obj;
	pthread_mutex_lock(cfgobj_locks[type]);
	obj = __cfgobj_find(name, type);
	pthread_mutex_unlock(cfgobj_locks[type]);
	return obj;
}

void ldmsd_cfgobj_del(const char *name, ldmsd_cfgobj_type_t type)
{
	ldmsd_cfgobj_t obj;
	pthread_mutex_lock(cfgobj_locks[type]);
	obj = __cfgobj_find(name, type);
	if (obj)
		rbt_del(cfgobj_trees[type], &obj->rbn);
	pthread_mutex_lock(cfgobj_locks[type]);
}

/**
 * Return the first configuration object of the given type
 *
 * This function must be called with the cfgobj_type lock held
 */
ldmsd_cfgobj_t ldmsd_cfgobj_first(ldmsd_cfgobj_type_t type)
{
	struct rbn *n;
	n = rbt_min(cfgobj_trees[type]);
	if (n)
		return ldmsd_cfgobj_get(container_of(n, struct ldmsd_cfgobj, rbn));
	return NULL;
}

/**
 * Return the next configuration object of the given type
 *
 * This function must be called with the cfgobj_type lock held
 */
ldmsd_cfgobj_t ldmsd_cfgobj_next(ldmsd_cfgobj_t obj)
{
	struct rbn *n;
	ldmsd_cfgobj_t nobj = NULL;

	n = rbn_succ(&obj->rbn);
	if (!n)
		goto out;
	nobj = ldmsd_cfgobj_get(container_of(n, struct ldmsd_cfgobj, rbn));
out:
	ldmsd_cfgobj_put(obj);	/* Drop the next reference */
	return nobj;
}
