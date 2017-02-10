/*
 * Copyright (C) 2014 - 2015 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Use of the Software is limited solely to applications:
 * (a) running on a Xilinx device, or
 * (b) that interact with a Xilinx device through a bus or interconnect.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 */

/*********************************************************************
 * Contains system-level PM functions and state
 ********************************************************************/

#include "pm_system.h"
#include "pm_common.h"
#include "crl_apb.h"
#include "pm_callbacks.h"
#include "xpfw_resets.h"
#include "pm_requirement.h"
#include "pm_sram.h"
#include "pm_periph.h"
#include "pm_ddr.h"

/*********************************************************************
 * Structure definitions
 ********************************************************************/
/**
 * PmSystem - System level information
 * @psRestartPerms	ORed IPI masks of masters allowed to restart PS
 * @systemRestartPerms	ORed IPI masks of masters allowed to restart system
 */
typedef struct {
	u32 psRestartPerms;
	u32 systemRestartPerms;
} PmSystem;

/**
 * PmSystemRequirement - System level requirements (not assigned to any master)
 * @slave	Slave for which the requirements are set
 * @caps	Capabilities of the slave that are required by the system
 */
typedef struct PmSystemRequirement {
	PmSlave* const slave;
	u32 caps;
} PmSystemRequirement;

/*********************************************************************
 * Data initialization
 ********************************************************************/

PmSystem pmSystem;

/*
 * These requirements are needed for the system to operate:
 * - OCM bank(s) store FSBL which is needed to restart APU, and should never be
 *   powered down unless the whole system goes down.
 */
PmSystemRequirement pmSystemReqs[] = {
	{
		.slave = &pmSlaveOcm0_g.slv,
		.caps = PM_CAP_CONTEXT,
	}, {
		.slave = &pmSlaveOcm1_g.slv,
		.caps = PM_CAP_CONTEXT,
	}, {
		.slave = &pmSlaveOcm2_g.slv,
		.caps = PM_CAP_CONTEXT,
	},{
		.slave = &pmSlaveOcm3_g.slv,
		.caps = PM_CAP_CONTEXT,
	},{
		.slave = &pmSlaveDdr_g,
		.caps = PM_CAP_ACCESS,
	},{
		.slave = &pmSlaveSD1_g,
		.caps = PM_CAP_ACCESS,
	},
	{
		.slave = &pmSlaveQSpi_g,
		.caps = PM_CAP_ACCESS,
	},
};

/*********************************************************************
 * Function definitions
 ********************************************************************/

/**
 * PmSystemRequirementAdd() - Add requirements of the system
 * @return	XST_SUCCESS if requirements are added, XST_FAILURE otherwise
 */
int PmSystemRequirementAdd(void)
{
	int status;
	u32 i;

	for (i = 0U; i < ARRAY_SIZE(pmSystemReqs); i++) {
		PmRequirement* req;

		status = PmRequirementAdd(NULL, pmSystemReqs[i].slave);
		if (XST_SUCCESS != status) {
			goto done;
		}

		req = PmRequirementGetNoMaster(pmSystemReqs[i].slave);
		if (NULL == req) {
			status = XST_FAILURE;
			goto done;
		}

		status = PmCheckCapabilities(req->slave, pmSystemReqs[i].caps);
		if (XST_SUCCESS != status) {
			status = XST_FAILURE;
			goto done;
		}

		req->info |= PM_MASTER_USING_SLAVE_MASK;
		req->preReq = pmSystemReqs[i].caps;
		req->currReq = pmSystemReqs[i].caps;
		req->nextReq = pmSystemReqs[i].caps;
		req->defaultReq = pmSystemReqs[i].caps;
		req->latencyReq = MAX_LATENCY;
	}

done:
	return status;
}
