#include <linux/kernel.h>
#include <linux/cpumask.h>
#include <linux/kernel_stat.h>
#include <linux/sysstat.h>
#include <linux/swap.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/tick.h>
#include <linux/export.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

extern int Asustor_Netlink(void*);

T_ASUSTOR_SYSTEM_STAT Asustor_Sys_Stat;
EXPORT_SYMBOL(Asustor_Sys_Stat);

spinlock_t Pers_lock = __SPIN_LOCK_UNLOCKED(Pers_lock);
EXPORT_SYMBOL(Pers_lock);

void Init_Asustor_Sata_Port(PT_ASUSTOR_SATA_STAT ptNew){
	return;
}

int Add_Asustor_Sata_Port(int cnAtaNumber){
	return 0;
}

int Remove_Asustor_Sata_Port(int cnAtaNumber){
	return 0;
}

int Link_Asustor_Sata_Port(int cnAtaNumber){
	return 0;
}

int Unlink_Asustor_Sata_Port(int cnAtaNumber){
	return 0;
}

int Add_Asustor_Sata_Disk(int cnAtaNumber, const char *pszdiskname){
	return 0;
}

int Add_Asustor_Sata_Retry_Count(int cnAtaNumber){
	return 0;
}

int Add_Asustor_Sata_Retry_Failed(int cnAtaNumber){
	return 0;
}

int Add_Asustor_Sata_Scsi_Error(int cnAtaNumber){
	return 0;
}

int Add_Asustor_Sata_Error_Log(int cnAtaNumber, uint8_t ata_command, uint32_t err_mask, uint8_t ata_status, uint8_t ata_error_flag, uint8_t sata_error_flag, uint64_t ata_lba){
	return 0;
}

extern int Add_Asustor_Netif_Stat(const char *szIfname){
	return 0;
}

extern int Remove_Asustor_Netif_Stat(const char *szIfname){
	return 0;
}

static void Init_Asustor_Mddev_Stat(PT_ASUSTOR_MDEV_LOAD ptNew, const char *pszDevName){
	return;
}

int Add_Asustor_Diskdev_Stat(const char *pszDevName){
	return 0;
}

int Remove_Asustor_Diskdev_Stat(const char *pszDevName){
	return 0;
}

void Init_Sys_Stat(void){
	return;
}
//EXPORT_SYMBOL(init_sys_stat);
