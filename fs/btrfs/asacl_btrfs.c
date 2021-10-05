#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/asacl.h>

#include "ctree.h"
#include "btrfs_inode.h"
#include "xattr.h"
#include "asacl_btrfs.h"

PT_ASACL Btrfs_Get_Asacl(struct inode *ptInode)
{
	return NULL;
}

int Btrfs_Set_Asacl(struct inode *ptInode, PT_ASACL ptasAcl, int bUpdateMode)
{
	return 0;
}

int Btrfs_Init_Asacl(struct btrfs_trans_handle *ptHandle, struct inode *ptInode, struct inode *ptParentInode)
{
	return 0;
}

void Btrfs_Init_Admin_Priv_Procfs(void)
{
	return;
}
