#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/quotaops.h>
#include <linux/asacl.h>

#include "ext4.h"
#include "ext4_jbd2.h"
#include "xattr.h"
#include "acl.h"
#include "asacl_ext4.h"

PT_ASACL Ext4_Get_Asacl(struct inode *ptInode)
{
	return NULL;
}

int Ext4_Set_Asacl(struct inode *ptInode, PT_ASACL ptasAcl, int bUpdateMode)
{
	return 0;
}

int Ext4_Init_Asacl(handle_t *ptHandle, struct inode *ptInode, struct inode *ptParentInode)
{
	return 0;
}

void Ext4_Init_Admin_Priv_Procfs(void)
{
	return;
}
