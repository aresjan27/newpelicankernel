/* Copyright (c) 2011-2020  Asustor Systems, Inc.  All Rights Reserved. */
/** \file asacl_ext4.h
 * \brief The header file for the functions of ASACL management.
 *
 * - History
 *  - 2014/05/08		clko	File created.
 *  - 2019/01/27		clko	Updated to adapt to kernel 4.14.x.
 */
#ifndef _ASACL_EXT4_INC_
#define _ASACL_EXT4_INC_

#include <linux/asacl.h>

#ifdef CONFIG_EXT4_FS_ASACL

#define EXT4_IS_ASACL(inode) IS_ASACL(inode)

/** \brief	The inode operation for "get_asacl".
 * 			Reference function: Original "ext4_get_acl()" function at fs/ext4/acl.c.
 *
 * \param	ptInode	The specified inode to get ASACL.
 * \return	It returns the ASACL obj of the inode if successful, returns NULL if the inode doesn't have ASACL,
 * 			or returns a negative address (use IS_ERR() to check) if encountering an error.
 */
extern PT_ASACL Ext4_Get_Asacl(struct inode *ptInode);

/** \brief	To set the ASACL of an inode. (internal function.)
 * 			Reference API: ext4_set_acl() of fs/ext4/acl.c.
 *
 * \param	ptasAcl		The ASACL to be set. You could also assign NULL to clear out the ASACL of this file.
 * \param	bUpdateMode	Flag to indicate if needing to update the file mode after the asacl is set successfully.
 * \return	It returns 0 if successful, or returns a negative value if encountering an error.
 */
extern int Ext4_Set_Asacl(struct inode *ptInode, PT_ASACL ptasAcl, int bUpdateMode);

/** \brief	Initialize the ACLs of a new inode. Called from ext4_new_inode.
 * 			Reference function: Original "ext4_init_acl()" function at fs/ext4/acl.c.
 *
 * \param	ptInode			The inode of the new file.
 * \param	ptParentInode	The inode of the parent dir of the new file.
 * \return	It returns 0 if successful, or returns a negative value if encountering an error.
 */
extern int Ext4_Init_Asacl(handle_t *ptHandle, struct inode *ptInode, struct inode *ptParentInode);

/** \brief To initialize the admin-priv proc entry file.
 *
 * \return None.
 */
extern void Ext4_Init_Admin_Priv_Procfs(void);

#else /* CONFIG_EXT4_FS_ASACL */

#define EXT4_IS_ASACL(inode) (0)
#define Ext4_Get_Asacl		NULL
#define Ext4_Set_Asacl		NULL

static inline int Ext4_Init_Asacl(handle_t *ptHandle, struct inode *ptInode, struct inode *ptParentInode)
{
	return 0;
}

#if 0
static inline int Ext4_Estimate_New_File_Asacl_Xattr_Size(umode_t mMode, struct inode *ptParentInode)
{
	return 0;
}
#endif

static inline void Ext4_Init_Admin_Priv_Procfs(void)
{
	return;
}

#endif /* CONFIG_EXT4_FS_ASACL */

#endif /* _ASACL_EXT4_INC_ */
