/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Eric Enright
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "pkt.h"

/* Number of bytes to pad the buffers to for checksum purposes */
#define BUF_PADDING	4

/*---------------------------------------------------------------------
 * pkt_alloc()
 *
 * 	This function allocates a new packet.
 *
 * Parameters:
 *
 * 	base	- base size of data buffer to use
 *
 * Returns:
 *
 * 	Pointer to an allocated and initialized pack on success.
 * 	NULL on error
 */
struct en_net_pkt * pkt_alloc(size_t base)
{
	struct en_net_pkt *pkt = NULL;

	/* Allocate the base structure */
	pkt = malloc(sizeof(struct en_net_pkt));
	if (pkt == NULL)
		goto out_err;

	memset(pkt, 0, sizeof(struct en_net_pkt));

	/* Pad to an even size so we can cast the buffer as uint16_t
 	 * for checksum purposes */
	if (base % BUF_PADDING)
		base += BUF_PADDING - (base % BUF_PADDING);

	/* Allocate data buffer */
	if (base > 0) {
		pkt->data = malloc(base);
		if (pkt->data == NULL)
			goto out_err;
		pkt->allocated = base;
		memset(pkt->data, 0, pkt->allocated);
	}

	// @@@ spin_lock_init(&pkt->lock);

	return pkt;

out_err:

	if (pkt != NULL) {
		if (pkt->data != NULL)
			free(pkt->data);

		free(pkt);
	}

	return NULL;
}

/*---------------------------------------------------------------------
 * pkt_free()
 *
 * 	This function frees a packet.
 *
 * Parameters:
 *
 * 	pkt	- the packet to free
 *
 * Returns:
 *
 * 	None
 */
void pkt_free(struct en_net_pkt *pkt)
{
	if (pkt->data != NULL)
		free(pkt->data);

	free(pkt);
}

/*---------------------------------------------------------------------
 * pkt_grow()
 *
 * 	This function grows a packet's data buffer.
 *
 * Parameters:
 *
 * 	pkt	- the packet to grow
 * 	len	- new buffer size
 *
 * Returns:
 *
 * 	Zero on success,
 * 	-errno on error.
 */
int pkt_grow(struct en_net_pkt *pkt, size_t len)
{
	void *p;

	if (len <= 0 || len < pkt->allocated)
		return 0;

	/* Pad to an even size so we can cast the buffer as uint16_t
 	 * for checksum purposes */
	if (len % BUF_PADDING)
		len += BUF_PADDING - (len % BUF_PADDING);

	/* Allocate the new buffer */
	p = malloc(len);
	if (p == NULL)
		// @@@ return -ENOMEM;
		return -1;
	pkt->allocated = len;

	/* Copy old data in */
	memcpy(p, pkt->data, pkt->len);

	/* Zero remaining data */
	memset(p + pkt->len, 0, pkt->allocated - pkt->len);

	/* Free old buffer */
	free(pkt->data);

	/* Replace old pointer */
	pkt->data = p;

	return 0;
}

/*---------------------------------------------------------------------
 * pkt_reserve()
 *
 * 	This function ensures the specified space is reserved, allocating
 * 	memory if not.
 *
 * Parameters:
 *
 * 	pkt	- the packet to modify
 * 	len	- length of the data to reserve
 *
 * Returns:
 *
 * 	Zero on success,
 * 	-errno on error.
 */
static inline int pkt_reserve(struct en_net_pkt *pkt, size_t len)
{
	int rc = 0;

	/* Is there room for this data? */
	if (len > pkt->allocated) {
		/* No, make room */
		rc = pkt_grow(pkt, len);
		if (rc)
			goto out;
	}

out:

	return rc;
}

/*---------------------------------------------------------------------
 * pkt_add_head()
 *
 * 	This function adds data to the start of the packet data.
 *
 * Parameters:
 *
 * 	pkt	- the packet to modify
 * 	buf	- the data to add
 * 	len	- length of the data to add
 *
 * Returns:
 *
 * 	Zero on success,
 * 	-errno on error.
 */
int pkt_add_head(struct en_net_pkt *pkt, const void *buf, size_t len)
{
	int rc = 0;

	/* Ensure there is room for this data */
	rc = pkt_reserve(pkt, pkt->allocated + len);
	if (rc)
		goto out;

	/* Push existing data down */
	memmove(pkt->data + len, pkt->data, pkt->len);

	/* Copy in new data */
	memcpy(pkt->data, buf, len);
	pkt->len += len;

out:

	return rc;
}

/*---------------------------------------------------------------------
 * pkt_add_tail()
 *
 * 	This function adds data to the end of the packet data.
 *
 * Parameters:
 *
 * 	pkt	- the packet to modify
 * 	buf	- the data to add
 * 	len	- length of the data to add
 *
 * Returns:
 *
 * 	Zero on success,
 * 	-errno on error.
 */
int pkt_add_tail(struct en_net_pkt *pkt, const void *buf, size_t len)
{
	int rc = 0;
	
	/* Ensure there is room for this data */
	rc = pkt_reserve(pkt, pkt->allocated + len);
	if (rc)
		goto out;

	/* Copy new data in */
	memcpy(pkt->data + pkt->len, buf, len);
	pkt->len += len;

out:

	return rc;
}

/*---------------------------------------------------------------------
 * pkt_add()
 *
 * 	This function adds data to the start and/or end of the packet data.
 *
 * Parameters:
 *
 * 	pkt	- the packet to modify
 * 	hbuf	- the data to add to the start
 * 	hlen	- length of the data to add to the start
 * 	tbuf	- the data to add to the end
 * 	tlen	- length of the data to add to the end
 *
 * Returns:
 *
 * 	Zero on success,
 * 	-errno on error.
 */
int pkt_add(struct en_net_pkt *pkt, const void *hbuf, size_t hlen,
	const void *tbuf, size_t tlen)
{
	int rc = 0;
	
	/* Ensure there is room for this data */
	rc = pkt_reserve(pkt, pkt->allocated + hlen + tlen);
	if (rc)
		goto out;

	/* Add the data. */
	if (hbuf != NULL) {
		rc = pkt_add_head(pkt, hbuf, hlen);
		if (rc)
			goto out;
	}

	if (tbuf != NULL) {
		rc = pkt_add_tail(pkt, tbuf, tlen);
		if (rc)
			goto out;
	}

out:

	return rc;
}

/*---------------------------------------------------------------------
 * pkt_del_head()
 *
 * 	This function removes data from the start of the packet data.
 * 	The buffer is not shrunk.
 *
 * Parameters:
 *
 * 	pkt	- the packet to modify
 * 	len	- length of the data to remove
 *
 * Returns:
 *
 * 	Zero on success,
 * 	-errno on error.
 * 		-EINVAL	the data contained in the packet is less than the
 * 			data requested to be removed
 */
int pkt_del_head(struct en_net_pkt *pkt, size_t len)
{
	int rc = 0;

	/* Ensure the packet has the data requested */
	if (len > pkt->len) {
		// @@@ rc = -EINVAL;
		rc = -1;
		goto out;
	}

	memmove(pkt->data, pkt->data + len, pkt->len - len);
	pkt->len -= len;

out:

	return rc;
}

