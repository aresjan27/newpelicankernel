/* Copyright (c) 2011-2020  Asustor Systems, Inc.  All Rights Reserved. */
/** \file asacl_btrfs.h
 * \brief The header file for the ASACL management functions for btrfs.
 *
 * - History
 *  - 2018/07/09		clko	File created.
 *  - 2019/01/27		clko	Updated to adapt to kernel 4.14.x.
 */
#ifndef _ASACL_BTRFS_INC_
#define _ASACL_BTRFS_INC_

#include <linux/asacl.h>

#ifdef CONFIG_BTRFS_FS_ASACL

#define BTRFS_IS_ASACL(inode) IS_ASACL(inode)

/** \brief	The inode operation for "get_asacl".
 * 			Reference function: Original "btrfs_get_acl()" function at fs/btrfs/acl.c.
 *
 * \param	ptInode	The specified inode to get ASACL.
 * \return	It returns the ASACL obj of the inode if successful, returns NULL if the inode doesn't have ASACL,
 * 			or returns a negative address (use IS_ERR() to check) if encountering an error.
 */
extern PT_ASACL Btrfs_Get_Asacl(struct inode *ptInode);

/** \brief	To set the ASACL of an inode.
 * 			Reference function: Original "btrfs_set_acl()" function at fs/btrfs/acl.c.
 *
 * \param	ptasAcl	The ASACL to be set. You could also assign NULL to clear out the ASACL of this file.
 * \param	bUpdateMode	Flag to indicate if needing to update the file mode after the asacl is set successfully.
 * \return	It returns 0 if successful, or returns a negative value if encountering an error.
 */
extern int Btrfs_Set_Asacl(struct inode *ptInode, PT_ASACL ptasAcl, int bUpdateMode);

/** \brief	Initialize the ACLs of a new inode. Called from btrfs_new_inode.
 * 			Reference function: Original "btrfs_init_acl()" function at fs/btrfs/acl.c.
 *
 * \param	ptInode			The inode of the new file.
 * \param	ptParentInode	The inode of the parent dir of the new file.
 * \return	It returns 0 if successful, or returns a negative value if encountering an error.
 */
extern int Btrfs_Init_Asacl(struct btrfs_trans_handle *ptHandle, struct inode *ptInode, struct inode *ptParentInode);

/** \brief To initialize the admin-priv proc entry file.
 *
 * \return None.
 */
extern void Btrfs_Init_Admin_Priv_Procfs(void);

#else /* CONFIG_BTRFS_FS_ASACL */

#define BTRFS_IS_ASACL(inode) (0)
#define Btrfs_Get_Asacl   NULL
#define Btrfs_Set_Asacl   NULL

static inline int Btrfs_Init_Asacl(struct btrfs_trans_handle *ptHandle, struct inode *ptInode, struct inode *ptParentInode)
{
	return 0;
}

static inline void Btrfs_Init_Admin_Priv_Procfs(void)
{
	return;
}

#endif /* CONFIG_BTRFS_FS_ASACL */

#endif /* _ASACL_BTRFS_INC_ */
