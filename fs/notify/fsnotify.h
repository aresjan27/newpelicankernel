#ifndef __FS_NOTIFY_FSNOTIFY_H_
#define __FS_NOTIFY_FSNOTIFY_H_

//#ifdef	//ASUSTOR_PATCH_FSNOTIFY
#ifndef	ASUSTOR_FSNOTIFY_ONLY
//#endif	//ASUSTOR_PATCH_FSNOTIFY

#include <linux/list.h>
#include <linux/fsnotify.h>
#include <linux/srcu.h>
#include <linux/types.h>

#include "../mount.h"

/* destroy all events sitting in this groups notification queue */
extern void fsnotify_flush_notify(struct fsnotify_group *group);

/* protects reads of inode and vfsmount marks list */
extern struct srcu_struct fsnotify_mark_srcu;

/* Calculate mask of events for a list of marks */
extern u32 fsnotify_recalc_mask(struct hlist_head *head);

/* compare two groups for sorting of marks lists */
extern int fsnotify_compare_groups(struct fsnotify_group *a,
				   struct fsnotify_group *b);

extern void fsnotify_set_inode_mark_mask_locked(struct fsnotify_mark *fsn_mark,
						__u32 mask);
/* Add mark to a proper place in mark list */
extern int fsnotify_add_mark_list(struct hlist_head *head,
				  struct fsnotify_mark *mark,
				  int allow_dups);
/* add a mark to an inode */
extern int fsnotify_add_inode_mark(struct fsnotify_mark *mark,
				   struct fsnotify_group *group, struct inode *inode,
				   int allow_dups);
/* add a mark to a vfsmount */
extern int fsnotify_add_vfsmount_mark(struct fsnotify_mark *mark,
				      struct fsnotify_group *group, struct vfsmount *mnt,
				      int allow_dups);

/* vfsmount specific destruction of a mark */
extern void fsnotify_destroy_vfsmount_mark(struct fsnotify_mark *mark);
/* inode specific destruction of a mark */
extern void fsnotify_destroy_inode_mark(struct fsnotify_mark *mark);
/* Find mark belonging to given group in the list of marks */
extern struct fsnotify_mark *fsnotify_find_mark(struct hlist_head *head,
						struct fsnotify_group *group);
/* Destroy all marks in the given list protected by 'lock' */
extern void fsnotify_destroy_marks(struct hlist_head *head, spinlock_t *lock);
/* run the list of all marks associated with inode and destroy them */
static inline void fsnotify_clear_marks_by_inode(struct inode *inode)
{
	fsnotify_destroy_marks(&inode->i_fsnotify_marks, &inode->i_lock);
}
/* run the list of all marks associated with vfsmount and destroy them */
static inline void fsnotify_clear_marks_by_mount(struct vfsmount *mnt)
{
	fsnotify_destroy_marks(&real_mount(mnt)->mnt_fsnotify_marks,
			       &mnt->mnt_root->d_lock);
}
/* prepare for freeing all marks associated with given group */
extern void fsnotify_detach_group_marks(struct fsnotify_group *group);
/*
 * wait for fsnotify_mark_srcu period to end and free all marks in destroy_list
 */
extern void fsnotify_mark_destroy_list(void);

/*
 * update the dentry->d_flags of all of inode's children to indicate if inode cares
 * about events that happen to its children.
 */
extern void __fsnotify_update_child_dentry_flags(struct inode *inode);

/* allocate and destroy and event holder to attach events to notification/access queues */
extern struct fsnotify_event_holder *fsnotify_alloc_event_holder(void);
extern void fsnotify_destroy_event_holder(struct fsnotify_event_holder *holder);

//#ifdef	//ASUSTOR_PATCH_FSNOTIFY
#endif	//ASUSTOR_FSNOTIFY_ONLY
//#endif	//ASUSTOR_PATCH_FSNOTIFY


#ifdef	ASUSTOR_PATCH_FSNOTIFY

#define	FS_AS_MASK						0x00F00000			/* ASUSTOR fsnotify event mask */

#define	FS_AS_CREATE					0x00100000			/* File was accessed */
#define	FS_AS_UNLINK					0x00200000			/* File was unlinked */
#define	FS_AS_MOVE_FROM					0x00300000			/* File was moved from X */
#define	FS_AS_MOVE_TO					0x00400000			/* File was moved to Y */
#define	FS_AS_LINK_FROM					0x00500000			/* File was linked from X */
#define	FS_AS_LINK_TO					0x00600000			/* File was linked to Y */

#define	FS_AS_TRUNCATE					0x00A00000			/* File size was truncated */
#define	FS_AS_TIME_MODIFY				0x00B00000			/* File time was modified */
#define	FS_AS_MODE_MODIFY				0x00C00000			/* File mode was modified */
#define	FS_AS_OWNER_MODIFY				0x00D00000			/* File owner was modified */

#define	FS_AS_XATTR_ACCESS				0x00E00000			/* File xattr was accessed */
#define	FS_AS_XATTR_MODIFY				0x00F00000			/* File time was modified */

// redefine constant and function if fsnotify_backend.h is not included (for fs/utimes.c)
#ifndef	FS_ISDIR
#define FS_ISDIR						0x40000000			/* event occurred against dir */
#define FSNOTIFY_EVENT_PATH				1
extern int fsnotify(struct inode *to_tell, __u32 mask, void *data, int data_is, const unsigned char *name, u32 cookie);
#endif	

#endif	//ASUSTOR_PATCH_FSNOTIFY

#endif	/* __FS_NOTIFY_FSNOTIFY_H_ */
