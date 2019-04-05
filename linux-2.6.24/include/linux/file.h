/*
 * Wrapper functions for accessing the file_struct fd array.
 */

#ifndef __LINUX_FILE_H
#define __LINUX_FILE_H

#include <asm/atomic.h>
#include <linux/posix_types.h>
#include <linux/compiler.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/types.h>

/*
 * The default fd array needs to be at least BITS_PER_LONG,
 * as this is the granularity returned by copy_fdset().
 * 内核允许每个进程打开NR_OPEN_DEFAULT个文件，默认是BITS_PER_LONG
 * 在32位操作系统上，初始为32；64位操作系统是64
 */
#define NR_OPEN_DEFAULT BITS_PER_LONG

/*
 * The embedded_fd_set is a small fd_set,
 * suitable for most tasks (which open <= BITS_PER_LONG files)
 * 简单的整数，封装在一个特殊的结构中
 */
struct embedded_fd_set {
	unsigned long fds_bits[1];
};

struct fdtable {
	unsigned int max_fds;	//指定进程当前可以处理的文件对象和文件描述符的最大数目
	struct file ** fd;      /* current fd array 指针数组，每个数组项指向一个file结构的实例，管理一个打开文件的所有信息
							 * 用户空间进程的文件描述符充当数组索引，该数组当前长度由max_fds定义	
							 */
	fd_set *close_on_exec;	//指向位域的指针，该位域保存了所有在exec系统调用时将要关闭的文件描述符的信息
	
	fd_set *open_fds;	//指向位域的指针，该位域管理当前所有打开文件的描述符。每个可能的文件描述符都对应一个比特位
						//如果该比特位置位，则对应的文件描述符处于使用中；否则未使用
	struct rcu_head rcu;
	struct fdtable *next;
};

/*
 * Open file table structure
 */
struct files_struct {
  /*
   * read mostly part
   */
	atomic_t count;
	struct fdtable *fdt;
	struct fdtable fdtab;	//文件描述符信息集合
  /*
   * written part on a separate cache line in SMP
   */
	spinlock_t file_lock ____cacheline_aligned_in_smp;
	int next_fd;	//下一次打开新文件时使用的文件描述符
	struct embedded_fd_set close_on_exec_init;	//位图，对执行exec时将关闭的所有文件描述符，在close_on_exec中对应的比特位都将置位
	struct embedded_fd_set open_fds_init;		//位图，最初的文件描述符集合
	struct file * fd_array[NR_OPEN_DEFAULT];	//每个数组项都是一个指针，指向每个打开文件的struct file实例
};

#define files_fdtable(files) (rcu_dereference((files)->fdt))

extern struct kmem_cache *filp_cachep;

extern void FASTCALL(__fput(struct file *));
extern void FASTCALL(fput(struct file *));

struct file_operations;
struct vfsmount;
struct dentry;
extern int init_file(struct file *, struct vfsmount *mnt,
		struct dentry *dentry, mode_t mode,
		const struct file_operations *fop);
extern struct file *alloc_file(struct vfsmount *, struct dentry *dentry,
		mode_t mode, const struct file_operations *fop);

static inline void fput_light(struct file *file, int fput_needed)
{
	if (unlikely(fput_needed))
		fput(file);
}

extern struct file * FASTCALL(fget(unsigned int fd));
extern struct file * FASTCALL(fget_light(unsigned int fd, int *fput_needed));
extern void FASTCALL(set_close_on_exec(unsigned int fd, int flag));
extern void put_filp(struct file *);
extern int get_unused_fd(void);
extern int get_unused_fd_flags(int flags);
extern void FASTCALL(put_unused_fd(unsigned int fd));
struct kmem_cache;

extern int expand_files(struct files_struct *, int nr);
extern void free_fdtable_rcu(struct rcu_head *rcu);
extern void __init files_defer_init(void);

static inline void free_fdtable(struct fdtable *fdt)
{
	call_rcu(&fdt->rcu, free_fdtable_rcu);
}

static inline struct file * fcheck_files(struct files_struct *files, unsigned int fd)
{
	struct file * file = NULL;
	struct fdtable *fdt = files_fdtable(files);

	if (fd < fdt->max_fds)
		file = rcu_dereference(fdt->fd[fd]);
	return file;
}

/*
 * Check whether the specified fd has an open file.
 */
#define fcheck(fd)	fcheck_files(current->files, fd)

extern void FASTCALL(fd_install(unsigned int fd, struct file * file));

struct task_struct;

struct files_struct *get_files_struct(struct task_struct *);
void FASTCALL(put_files_struct(struct files_struct *fs));
void reset_files_struct(struct task_struct *, struct files_struct *);

extern struct kmem_cache *files_cachep;

#endif /* __LINUX_FILE_H */
