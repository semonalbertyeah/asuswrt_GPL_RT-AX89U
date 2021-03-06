/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include<linux/module.h>
#include<linux/version.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include "commonmhitest.h"

static bool rddm_r;
module_param(rddm_r, bool, 0);
MODULE_PARM_DESC(rddm_r, "Do need to go for recovery after rddm?");

static int d_instance;
static struct mhitest_platform *mplat_g[MHI_MAX_DEVICE];
static struct platform_device *m_plat_dev;

struct platform_device *get_plat_device(void)
{
	return m_plat_dev;
}

int mhitest_store_mplat(struct mhitest_platform *temp)
{

	if (d_instance < MHI_MAX_DEVICE) {
		mplat_g[d_instance] = temp;
		mplat_g[d_instance]->d_instance = d_instance;
		pr_mhitest2("mplat_g[%d]:%p temp:%p same ? d_instance:%d\n",
			d_instance, mplat_g[d_instance], temp, d_instance);
		d_instance++;
		return 0;
	}
	pr_mhitest2("Error Max device support count exceeds ...\n");
	return 1;
}
void mhitest_free_mplat(struct mhitest_platform *temp)
{
	pr_mhitest2("##\n");
	devm_kfree(&temp->plat_dev->dev, temp);
}

struct mhitest_platform *get_mhitest_mplat(int id)
{
	return mplat_g[id];
}

char *mhitest_recov_reason_to_str(enum mhitest_recovery_reason reason)
{
	switch (reason) {
	case MHI_DEFAULT:
		return "MHI_DEFAULT";
	case MHI_LINK_DOWN:
		return "MHI_LINK_DOWN";
	case MHI_RDDM:
		return "MHI_RDDM";
	case MHI_TIMEOUT:
		return "MHI_TIMEOUT";
	default:
		return "UNKNOWN";
	}
}

void mhitest_recovery_post_rddm(struct mhitest_platform *mplat)
{
	int ret;
	u32 in_reset = -1, temp = -1;

	pr_mhitest2("##Start...\n");
	msleep(10000); /*Let's wait for some time !*/

	mhitest_pci_set_mhi_state(mplat, MHI_POWER_OFF);
//	msleep(1000);
	mhitest_pci_set_mhi_state(mplat, MHI_DEINIT);

	pr_mhitest2("in_reset:%d - before sleep\n", in_reset);
	mhitest_pci_remove_all(mplat);
	temp = readl_relaxed(mplat->mhi_ctrl->regs  + 0x38);
	in_reset = (temp & 0x2) >> 0x1;
	pr_mhitest2("in_reset0?:%d- after sleep\n", in_reset);
	if (in_reset) {
		pr_mhitest2("Device failed to exit RESET state\n");
		return;
	}
	pr_mhitest2("MHI Reset good !!!\n");
	ret = mhitest_ss_powerup(&mplat->mhitest_ss_desc);
	if (ret) {
		pr_mhitest2("ERRORRRR..ret:%d\n", ret);
		return;
	}

	pr_mhitest2("##End...\n");
}

int mhitest_recovery_event_handler(struct mhitest_platform *mplat, void *data)
{
	struct mhitest_driver_event *event = data;
	struct mhitest_recovery_data *rdata = event->data;
	struct subsys_device *mhitest_ss_device = mplat->mhitest_ss_device;

	pr_mhitest2("Recovery triggred with reason:(%s)-(%d)\n",
		mhitest_recov_reason_to_str(rdata->reason), rdata->reason);

	switch (rdata->reason) {
	case MHI_DEFAULT:
	case MHI_LINK_DOWN:
	case MHI_TIMEOUT:
		break;
	case MHI_RDDM:
		mhitest_dump_info(mplat, false);
		if (rddm_r) { /*using mod param for now*/
			mhitest_recovery_post_rddm(mplat);
			return 0; /*for now*/
		} else
			return 0;
		break;
	default:
		pr_mhitest2("Incorect reason...\n");
		break;
	}
	/*TODO: no subsystem restart for now. check the use case!*/
	subsystem_restart_dev(mhitest_ss_device);
kfree(data);
return 0;
}

static void mhitest_event_work(struct work_struct *work)
{
	struct mhitest_platform *mplat =
		container_of(work, struct mhitest_platform, event_work);
	struct mhitest_driver_event *event;
	unsigned long flags;
	int ret = 0;

	if (!mplat) {
		pr_mhitest2("NULL mplat\n");
		return;
	}
	spin_lock_irqsave(&mplat->event_lock, flags);
	while (!list_empty(&mplat->event_list)) {
		event = list_first_entry(&mplat->event_list,
				struct mhitest_driver_event, list);
		list_del(&event->list);
		spin_unlock_irqrestore(&mplat->event_lock, flags);

		switch (event->type) {
		/*only support recovery event so far*/
		case MHITEST_RECOVERY_EVENT:
			pr_mhitest2("MHITEST_RECOVERY_EVENT event..\n");
			ret = mhitest_recovery_event_handler(mplat, event);
			break;
		default:
			pr_mhitest2("Invalid event recived ..\n");
			kfree(event);
			continue;
		}

		spin_lock_irqsave(&mplat->event_lock, flags);
		event->ret = ret;
		if (event->sync) {
			pr_mhitest2("sending event completion event..\n");
			complete(&event->complete);
			continue;
		}
		spin_unlock_irqrestore(&mplat->event_lock, flags);
		kfree(event);
		spin_lock_irqsave(&mplat->event_lock, flags);
	}
	spin_unlock_irqrestore(&mplat->event_lock, flags);

}
int mhitest_register_driver(void)
{
	int ret = 1;

	pr_mhitest2("Going for register pci and subsystem..\n");
	ret = mhitest_pci_register();
	if (ret) {
		pr_mhitest("Error pci register ret:%d\n", ret);
		goto error_pci_reg;
	}
	return 0;

error_pci_reg:
	mhitest_pci_unregister();
	return ret;
}

void mhitest_unregister_driver(void)
{
	pr_mhitest2("Unregistering...\n");

	/* add driver related unregister stuffs here */
	mhitest_pci_unregister();

}

int mhitest_event_work_init(struct mhitest_platform *mplat)
{
	spin_lock_init(&mplat->event_lock);
	mplat->event_wq = alloc_workqueue("mhitest_mod_event",
						      WQ_UNBOUND, 1);
	if (!mplat->event_wq) {
		pr_mhitest2("Failed to create event workqueue!\n");
		return -EFAULT;
	}
	INIT_WORK(&mplat->event_work, mhitest_event_work);
	INIT_LIST_HEAD(&mplat->event_list);

	return 0;
}
void mhitest_event_work_deinit(struct mhitest_platform *mplat)
{
	pr_mhitest2("##\n");
	if (mplat->event_wq)
		destroy_workqueue(mplat->event_wq);
}
static int mhitest_probe(struct platform_device *plat_dev)
{
	int ret;

	pr_mhitest2("## Start\n");

	m_plat_dev = plat_dev;

	ret = mhitest_register_driver();
	if (ret) {
		pr_mhitest2("Error..\n");
		goto fail_probe;
	}
	pr_mhitest2("## End\n");
	return 0;

fail_probe:
	return ret;
}
static int mhitest_remove(struct platform_device *plat_dev)
{
	pr_mhitest2("##\n");
	mhitest_unregister_driver();
	m_plat_dev = NULL;
	return 0;
}

void mhitest_pci_disable_msi(struct mhitest_platform *mplat)
{
	pci_free_irq_vectors(mplat->pci_dev);
	/*pci_disable_msi(mplat->pci_dev);*/
}

void mhitest_pci_unregister_mhi(struct mhitest_platform *mplat)
{
	struct mhi_controller *mhi_ctrl = mplat->mhi_ctrl;

	mhi_unregister_mhi_controller(mhi_ctrl);
	kfree(mhi_ctrl->irq);
}

int mhitest_pci_remove_all(struct mhitest_platform *mplat)
{
	pr_mhitest2("start.\n");

	mhitest_pci_unregister_mhi(mplat);
	mhitest_pci_disable_msi(mplat);

	mhitest_pci_disable_bus(mplat);

	mhitest_unregister_ramdump(mplat);

	pr_mhitest2("End\n");
return 0;
}

static const struct platform_device_id test_platform_id_table[] = {
	{ .name = "qcn90xx", .driver_data = QCN90xx_DEVICE_ID, },
	{ .name = "qca6390", .driver_data = QCA6390_DEVICE_ID, },
};

static const struct of_device_id test_of_match_table[] = {
	{
	.compatible = "qcom,testmhi",
	.data = (void *)&test_platform_id_table[0]},
	{ },
};

MODULE_DEVICE_TABLE(of, cnss_of_match_table);

struct platform_driver mhitest_platform_driver = {
	.probe = mhitest_probe,
	.remove = mhitest_remove,
	.driver = {
		.name = "mhitest",
		.owner = THIS_MODULE,
		.of_match_table = test_of_match_table,
	},
};

int __init mhitest_init(void)
{
	int ret;
	pr_mhitest2("Inserting--->...\n");
	ret = platform_driver_register(&mhitest_platform_driver);
	if (ret)
		pr_mhitest("Error: Platform driver reg.ret:%d\n", ret);
	pr_mhitest2("...<---done\n");
	return ret;
}
void __exit mhitest_exit(void)
{
	pr_mhitest2("##\n");
	platform_driver_unregister(&mhitest_platform_driver);
}


module_init(mhitest_init);
module_exit(mhitest_exit);

MODULE_DESCRIPTION("MHITEST");
MODULE_LICENSE("GPL v2");
