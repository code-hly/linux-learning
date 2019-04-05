#ifndef _NAMESPACE_H_
#define _NAMESPACE_H_
#ifdef __KERNEL__

#include <linux/mount.h>
#include <linux/sched.h>
#include <linux/nsproxy.h>

//装载的文件系统所属的命名空间
struct mnt_namespace {
	atomic_t		count;	//计数器，指定使用该命名空间的进程数
	struct vfsmount *	root;	//指向根目录的vfsmount实例
	struct list_head	list;	//双链表表头，保存了vfs命名空间中所有文件系统的VFSmount实例，链表元素时vfsmount的成员mnt_list
	wait_queue_head_t poll;
	int event;
};

extern struct mnt_namespace *copy_mnt_ns(unsigned long, struct mnt_namespace *,
		struct fs_struct *);
extern void __put_mnt_ns(struct mnt_namespace *ns);

static inline void put_mnt_ns(struct mnt_namespace *ns)
{
	if (atomic_dec_and_lock(&ns->count, &vfsmount_lock))
		/* releases vfsmount_lock */
		__put_mnt_ns(ns);
}

static inline void exit_mnt_ns(struct task_struct *p)
{
	struct mnt_namespace *ns = p->nsproxy->mnt_ns;
	if (ns)
		put_mnt_ns(ns);
}

static inline void get_mnt_ns(struct mnt_namespace *ns)
{
	atomic_inc(&ns->count);
}

#endif
#endif
