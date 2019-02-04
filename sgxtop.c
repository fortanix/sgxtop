/*
 * BSD LICENSE
 *
 * Copyright (C) 2018 Fortanix, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Fortanix, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 *
 * Authors:
 *
 * Kevin Lahey <kevin.lahey@fortanix.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/queue.h>

#define SGX_STATS	"/proc/sgx_stats"
#define SGX_ENCLAVES	"/proc/sgx_enclaves"

struct stats {
	unsigned int enclaves_created;
	unsigned int enclaves_released;
	unsigned long pages_added;
	unsigned long pageins;
	unsigned long pageouts;
	unsigned int enclave_pages;
	unsigned int va_pages;
	unsigned int free_pages;
	struct timespec readtime;
};

LIST_HEAD(enclave_head, enclave);

struct enclave {
	pid_t pid;
	unsigned int id;
	unsigned long size;
	unsigned long eadd_cnt;
	unsigned long resident;
	unsigned long old_eadd_cnt;
	unsigned long old_resident;
	LIST_ENTRY(enclave) state_entry[2];
	LIST_ENTRY(enclave) hash_entry;
};

/*
 * Keep an old and a new  list of enclaves, and a hash table.
 * Look 'em up quickly in the hash table as we read them.
 * Put them in the new list as they are read.  Remove the old
 * list when we've read all of the new enclaves.
 *
 * XXX:  When do we sort 'em?  Create an array and then sort that?
 * Guess so.
 */

struct enclaves {
	unsigned int count;
	unsigned int state;
	struct enclave_head *hash_table;
	size_t hash_table_size;
	struct enclave_head state_list[2];
	struct timespec readtime;
};

int sleep_til(struct timespec *when)
{
	struct timespec rem;
	int rc;
	while (rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
				    when, NULL)) {
		if (rc != EINTR)
			return rc;
	}
	return rc;
}

int stats_read(struct stats *stats)
{
	FILE *fp;

	if (!(fp = fopen(SGX_STATS, "ro")))
		return -1;

	int r = fscanf(fp, "%u %u %lu %lu %lu %u %u %u",
		       &stats->enclaves_created, &stats->enclaves_released,
		       &stats->pages_added, &stats->pageins, &stats->pageouts,
		       &stats->enclave_pages, &stats->va_pages,
		       &stats->free_pages);
	fclose(fp);
	if (r != 8)
		return -1;

	r = clock_gettime(CLOCK_MONOTONIC, &stats->readtime);
	if (r)
		return r;

	return 0;
}


int stats_report(struct stats *old, struct stats *new)
{
	printf("%u/%u enclaves/created %lu added %lu pagins %lu pageouts %u total_resident %u va %u free\n",
	       new->enclaves_created - new->enclaves_released,
	       new->enclaves_created,
	       new->pages_added,
	       new->pageins - old->pageins,
	       new->pageouts - old->pageouts,
	       new->enclave_pages,
	       new->va_pages,
	       new->free_pages);
}

int enclave_read(FILE *fp, struct enclave *enclave)
{
	int r = fscanf(fp, "%d %u %lu %lu %lu",
		       &enclave->pid, &enclave->id,
		       &enclave->size, &enclave->eadd_cnt,
		       &enclave->resident);
	if (r != 5)
		return -1;

	return 0;
}

int enclave_update(struct enclave *o, struct enclave *n)
{
	o->old_eadd_cnt = o->eadd_cnt;
	o->old_resident = o->resident;
	o->eadd_cnt = n->eadd_cnt;
	o->resident = n->resident;
}

struct enclaves *enclave_create(size_t n)
{
	struct enclaves *e = calloc(sizeof(struct enclaves), 1);
	if (!e)
		return NULL;

	e->hash_table_size = n;
	e->hash_table = malloc(sizeof(struct enclave_head) * n);
	if (!e->hash_table)
		return NULL;

	for (int i = 0; i < n; i++)
		LIST_INIT(&e->hash_table[i]);

	e->state = 0;
	LIST_INIT(&e->state_list[0]);
	LIST_INIT(&e->state_list[1]);
}

size_t hash_func(struct enclaves *e, unsigned int id)
{
	// Ugh!
	return id % e->hash_table_size;
}

struct enclave *enclaves_find(struct enclaves *enclaves, unsigned int id)
{
	size_t bucket = hash_func(enclaves, id);
	struct enclave *e = LIST_FIRST(&enclaves->hash_table[bucket]);

	while (e) {
		if (e->id)
			return e;
	}
	return NULL;
}

int enclaves_insert(struct enclaves *enclaves, struct enclave *e)
{
	size_t bucket = hash_func(enclaves, e->id);
	LIST_INSERT_HEAD(&enclaves->hash_table[bucket], e, hash_entry);
}

int enclaves_delete(struct enclaves *enclaves, struct enclave *e)
{
	LIST_REMOVE(e, hash_entry);
}

int enclaves_read(struct enclaves *enclaves)
{
	FILE *fp;

	// We track old and new entries, and have to swap which list
	// represents new;
	enclaves->state = !enclaves->state;

	if (!(fp = fopen(SGX_ENCLAVES, "ro")))
		return -1;

	struct enclave enclave;
	struct enclave *e;
	while (!enclave_read(fp, &enclave)) {
		enclaves->count++;

		if (e = enclaves_find(enclaves, enclave.id)) {
			enclave_update(e, &enclave);
		} else {
			e = malloc(sizeof(struct enclave));
			if (!e)
				return 0;
			*e = enclave;
			enclaves_insert(enclaves, e);
		}
		LIST_INSERT_HEAD(&enclaves->state_list[enclaves->state],
				 e, state_entry[enclaves->state]);
	}
	fclose(fp);

	int r = clock_gettime(CLOCK_MONOTONIC, &enclaves->readtime);
	if (r)
		return r;

	/* Iterate over the table of enclaves and remove old ones. */
	while (!LIST_EMPTY(&enclaves->state_list[!enclaves->state])) {
		e = LIST_FIRST(&enclaves->state_list[!enclaves->state]);
		LIST_REMOVE(e, state_entry[!enclaves->state]);
		LIST_REMOVE(e, hash_entry);
	}

	return 0;
}

int enclaves_report(struct enclaves *enclaves)
{
	struct enclave *e;
	LIST_FOREACH(e, &enclaves->state_list[enclaves->state],
		     state_entry[enclaves->state]) {
		printf("%d %u size %luK eadded %luK resident %luK\n",
		       e->pid, e->id, e->size / 1024, e->eadd_cnt * 4,
		       e->resident * 4);
	}
}

int main(int argc, char **argv)
{
	struct stats old, new;
	struct enclaves *enclaves = enclave_create(100);

	if (!enclaves)
		exit(-1);

	if (stats_read(&new))
		exit(-1);

	struct timespec now = new.readtime;
	while (1) {
		old = new;
		now.tv_sec++;
		if (sleep_til(&now))
			exit(-1);
		stats_read(&new);
		enclaves_read(enclaves);
		stats_report(&old, &new);
		enclaves_report(enclaves);
	}
	return 0;
}
