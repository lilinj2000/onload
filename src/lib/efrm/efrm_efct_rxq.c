/* SPDX-License-Identifier: GPL-2.0 */
/* X-SPDX-Copyright-Text: (c) Copyright 2021 Xilinx, Inc. */
#include <ci/efrm/private.h>
#include <ci/efrm/pd.h>
#include <ci/efrm/efct_rxq.h>
#include <ci/driver/resource/linux_efhw_nic.h>
#include <ci/efhw/efct.h>
#include "efrm_internal.h"

struct efrm_efct_rxq {
	struct efrm_resource rs;
	struct efrm_pd *pd;
	unsigned qid;
	size_t n_hugepages;
};

static bool check_efct(const struct efrm_resource *rs)
{
	if (rs->rs_client->nic->devtype.arch != EFHW_ARCH_EFCT) {
		EFRM_TRACE("%s: Only EFCT NIC supports rxq resources."
		           " Expected arch=%d but got %d\n", __FUNCTION__,
		           EFHW_ARCH_EFCT, rs->rs_client->nic->devtype.arch);
		return false;
	}
	return true;
}

extern struct efrm_resource* efrm_rxq_to_resource(struct efrm_efct_rxq *rxq)
{
	return &rxq->rs;
}
EXPORT_SYMBOL(efrm_rxq_to_resource);


extern struct efrm_efct_rxq* efrm_rxq_from_resource(struct efrm_resource *rs)
{
	return container_of(rs, struct efrm_efct_rxq, rs);
}
EXPORT_SYMBOL(efrm_rxq_from_resource);


int efrm_rxq_alloc(struct efrm_pd *pd, int qid,
                   const struct cpumask *mask, bool timestamp_req,
                   size_t n_hugepages, struct efrm_efct_rxq **rxq_out)
{
	int rc;
	struct efrm_efct_rxq *rxq;
	struct efrm_resource *pd_rs = efrm_pd_to_resource(pd);

	if (!check_efct(pd_rs))
		return -EOPNOTSUPP;

	rxq = kzalloc(sizeof(struct efrm_efct_rxq), GFP_KERNEL);
	if (!rxq)
		return -ENOMEM;

	rxq->pd = pd;
	rxq->n_hugepages = n_hugepages;
	rc = efct_nic_rxq_bind(pd_rs->rs_client->nic, qid, mask, timestamp_req,
	                       n_hugepages);
	if (rc < 0) {
		kfree(rxq);
		return rc;
	}
	rxq->qid = rc;
	efrm_resource_init(&rxq->rs, EFRM_RESOURCE_EFCT_RXQ, 0);
	efrm_client_add_resource(pd_rs->rs_client, &rxq->rs);
	efrm_resource_ref(pd_rs);
	*rxq_out = rxq;
	return 0;
}
EXPORT_SYMBOL(efrm_rxq_alloc);


void efrm_rxq_release(struct efrm_efct_rxq *rxq)
{
	if (__efrm_resource_release(&rxq->rs)) {
		efct_nic_rxq_free(rxq->rs.rs_client->nic, rxq->qid,
		                  rxq->n_hugepages);
		efrm_pd_release(rxq->pd);
		efrm_client_put(rxq->rs.rs_client);
		kfree(rxq);
	}
}
EXPORT_SYMBOL(efrm_rxq_release);


static void efrm_rxq_rm_dtor(struct efrm_resource_manager *rm)
{
	/* NOP */
}


int efrm_create_rxq_resource_manager(struct efrm_resource_manager **rm_out)
{
	struct efrm_resource_manager *rm;
	int rc;

	rm = kzalloc(sizeof(*rm), GFP_KERNEL);
	if (rm == NULL)
		return -ENOMEM;

	rc = efrm_resource_manager_ctor(rm, efrm_rxq_rm_dtor, "RXQ",
	                                EFRM_RESOURCE_EFCT_RXQ);
	if (rc < 0)
		goto fail;

	*rm_out = rm;
	return 0;

fail:
	kfree(rm);
	return rc;
}