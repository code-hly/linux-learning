/*
 *
 * Definitions for mount interface. This describes the in the kernel build 
 * linkedlist with mounted filesystems.
 *
 * Author:  Marco van Wieringen <mvw@planets.elm.net>
 *
 * Version: $Id: mount.h,v 2.0 1996/11/17 16:48:14 mvw Exp mvw $
 *
 */
#ifndef _LINUX_MOUNT_H
#define _LINUX_MOUNT_H
#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <asm/atomic.h>

struct super_block;
struct vfsmount;
struct dentry;
struct mnt_namespace;

#define MNT_NOSUID	0x01
#define MNT_NODEV	0x02	//装载文件系统是虚拟的，没有物理后端设备
#define MNT_NOEXEC	0x04
#define MNT_NOATIME	0x08
#define MNT_NODIRATIME	0x10
#define MNT_RELATIME	0x20

#define MNT_SHRINKABLE	0x100	//专门用于NFS和AFS的，用来标记子装载

#define MNT_SHARED	0x1000	/* if the vfsmount is a shared mount */
#define MNT_UNBINDABLE	0x2000	/* if the vfsmount is a unbindable mount */
#define MNT_PNODE_MASK	0x3000	/* propagation flag mask */

struct vfsmount {
	struct list_head mnt_hash;	//散列表，该参数是链表元素
	struct vfsmount *mnt_parent;	/* fs we are mounted on 指向父文件系统的vfsmount结构 */
	struct dentry *mnt_mountpoint;	/* dentry of mountpoint 保存当前文件系统的装载点在其父目录中的dentry结构 */
	struct dentry *mnt_root;	/* root of the mounted tree 保存文件系统本身的相对根目录所对应的dentry结构 */
	struct super_block *mnt_sb;	/* pointer to superblock 该指针指向相对应的超级块 */
	struct list_head mnt_mounts;	/* list of children, anchored here 文件系统的父子关系用链表实现，该参数是子文件系统的起点 */
	struct list_head mnt_child;	/* and going through their mnt_child 该参数和mnt_mounts一起作用。该参数用作链表的链表元素 */
	int mnt_flags;	//设置各种独立于文件系统的标志
	/* 4 bytes hole on 64bits arches */
	char *mnt_devname;		/* Name of device e.g. /dev/dsk/hda1 */
	struct list_head mnt_list;
	struct list_head mnt_expire;	/* link in fs-specific expiry list 作为链表元素，用作把所有可能自动过期的装载放置在一个循环链表上 */
	struct list_head mnt_share;	/* circular list of shared mounts 共享装载的循环链表元素 */
	struct list_head mnt_slave_list;/* list of slave mounts 从属装载的链表表头 */
	struct list_head mnt_slave;	/* slave list entry 从属装载的链表元素 */
	struct vfsmount *mnt_master;	/* slave is on master->mnt_slave_list 所有从属装载都通过该参数指向主装载 */
	struct mnt_namespace *mnt_ns;	/* containing namespace 指向该装载所属的命名空间 */
	/*
	 * We put mnt_count & mnt_expiry_mark at the end of struct vfsmount
	 * to let these frequently modified fields in a separate cache line
	 * (so that reads of mnt_flags wont ping-pong on SMP machines)
	 */
	atomic_t mnt_count;	//使用计数器，每当一个vfsmount实例不在需要时，都必须用mntput把计数器减1
	int mnt_expiry_mark;		/* true if marked for expiry 表示装载的文件系统是否不在使用 */
	int mnt_pinned;
};

static inline struct vfsmount *mntget(struct vfsmount *mnt)
{
	if (mnt)
		atomic_inc(&mnt->mnt_count);
	return mnt;
}

extern void mntput_no_expire(struct vfsmount *mnt);
extern void mnt_pin(struct vfsmount *mnt);
extern void mnt_unpin(struct vfsmount *mnt);

static inline void mntput(struct vfsmount *mnt)
{
	if (mnt) {
		mnt->mnt_expiry_mark = 0;
		mntput_no_expire(mnt);
	}
}

extern void free_vfsmnt(struct vfsmount *mnt);
extern struct vfsmount *alloc_vfsmnt(const char *name);
extern struct vfsmount *do_kern_mount(const char *fstype, int flags,
				      const char *name, void *data);

struct file_system_type;
extern struct vfsmount *vfs_kern_mount(struct file_system_type *type,
				      int flags, const char *name,
				      void *data);

struct nameidata;

extern int do_add_mount(struct vfsmount *newmnt, struct nameidata *nd,
			int mnt_flags, struct list_head *fslist);

extern void mark_mounts_for_expiry(struct list_head *mounts);
extern void shrink_submounts(struct vfsmount *mountpoint, struct list_head *mounts);

extern spinlock_t vfsmount_lock;
extern dev_t name_to_dev_t(char *name);

#endif
#endif /* _LINUX_MOUNT_H */
