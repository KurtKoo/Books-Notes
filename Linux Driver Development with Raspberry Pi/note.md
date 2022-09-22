# Chapter 1 Building the System
## Bootloader
* Configuration of the memory system.
* Loading of the kernel image and the Device Tree at the correct addresses.
* Optional loading of an initial RAM disk at the correct memory address.
* Setting of the kernel command-line and other parameters (e.g., Device Tree, machine type).

## Root Filesystem
* **/bin**: Commands needed during bootup that might be used by normal users (probably after bootup).
* **/sbin:**: Commands for root users.
* **/etc**:  Configuration files specific to the machine.
* **/home**: 家目录。
* **/root**: root用户的家目录。
* **/lib**: Essential shared libraries and kernel modules.
* **/etc**: Device files. These are special virtual files that help the user interface with the various devices on the system. 
* **/tmp**: Temporary files. Programs running often store temporary files in here.
* **/boot**: Files used by the bootstrap loader. Kernel images are often kept here instead of in the root directory.
* **/mnt**: 文件系统临时挂载点。
* **/opt**: Add-on application software packages.
* **/usr**: Secondary hierarchy.
* **/var**: Variable data.
* **/sys**: Exports information about devices and drivers from the kernel device model to user space, and it is also used for configuration.
* **/proc**: Represent the current state of the kernel.

## Linux Boot Process
1. 开始于POR(Power on Reset)，执行在片上ROM的bootloader的第一阶段。DRAM memory controller此时关闭。
2. bootloader的第一部分读取第二部分内容。bootloader第二阶段配置DRAM memory controller，读取并运行固件。固件会读取系统配置，在DRAM memory加载kernel image和设备树，最后在CPU上释放reset信号。
3. kernel image随后运行在CPU上。
4. 底层初始化，如使能MMU、创建页表和建立cache。随后进入start_kernel()函数。
5. start_kernel()函数会进行：
    - 初始化kernel核心组件，如memory、scheduling和interrupts等。
    - 初始化静态编译的驱动。
    - 根据在U-Boot传给内核的bootargs，挂在root filesystem。
    - 执行第一个用户进程，init。

# Chapter 2 The Linux Device and Driver Model
## Bus Core Driver
* 分配`bus_type`结构，并用`bus_register()`函数注册各种类型总线（USB、PCI和I2C等）。`/sys/bus/platform`目录下有`device`和`driver`两个分支。
* bus controller driver检测设备和分配资源，`device_register()`函数。
* 匹配设备和驱动，进行绑定，`driver_register()`函数。

## Device
* 在Linux中，最底层表示设备的数据结构为`struct device`。在`struct device`中，`struct kboject`表示设备并用于与系统交互。Linux中一般不用`struct device`，多为用`struct platform_device`进行封装。

![linux device model](https://github.com/KurtKoo/Books-Notes/blob/master/Linux%20Driver%20Development%20with%20Raspberry%20Pi/img/chap2/linux_device_model.png)

# Chapter 3 The Simplest Drivers
* module_init()和module_exit()分别导出驱动init()和exit()函数。
* module_param(变量名，变量类型，变量权限)用于导出某一变量。

# Chapter 4 Character Drivers
* Linux支持character device（c）、block device（b）和network device。character device，无缓冲区设备类型。block device，通常以块大小访问，512或1024字节。
* copy_from_user()和copy_to_user()用于进行内核态与用户态之间的数据交换。
* Linux中每个设备都有其major number（主设备号）和minor number（从设备号）。
* `mknod /dev/test_c_dev c 202 108`，创建一个主设备号为202和从设备号为108的character device。在kernel中，character device用`struct cdev`表示。

## Registration and Unregistration
* register_chrdev_region(dev_t first, unsigned int count, char* name)和unregister_chrdev_region(dev_t first, unsigned int count)用于注册和注销character device。__推荐用alloc_chrdev_region(dev_t* dev, unsigned baseminor, unsigned count, const char* name)函数，因为该函数可动态获取主设备号。__
* dev_t类型数据可以用宏MKDEV获取。
* cdev_init()用于初始化character device。cdev_add()用于添加character device于kernel中。

## Class Character Device
* class_create()和class_destroy()用于创建和销毁类。
* device_create()和device_destroy()用于创建和销毁设备节点。
* 可创建一个class，基于这个class创建一个device，且这个device是与某个已创建的character device绑定，以达到一个class可以管理多个device(character device)的目的。

# Chapter 5 Platform Drivers
待写

# Chapter 6 I2C Client Drivers
待写

# Chapter 7 Handling Interrupts in Device Drivers
* `struct irq_chip` - hardware interrupt chip descriptor
* `struct irq_desc`，表示一个IRQ。`struct irq_data`，包含底层中与中断管理相关的信息。

## Linux Kernel IRQ Domain for GPIO Controllers
待写

## Device Tree Interrupt Handling
* 节点property
    1. `interrupt-controller`，表示该节点接收中断信号。
    2. `interrupt-cells`，indicates the number of cells in the interrupts property for the child device nodes。
    3. `interrupt-parent`， a phandle to the interrupt controller that it is attached to。Nodes that do not have an interrupt-parentproperty can also inherit the property of their parent node.
    4. `interrupt`,containing a list of interrupt specifiers, one for each interrupt output signal on the device.

## Requesting Interrupts in Linux Device Drivers
* interrupt context（中断上下文）中不可访问用户空间和进入睡眠。中断服务函数，不可以进行任何可能睡眠的操作。
* 推荐用devm_request_irq()函数获取中断线，可自动管理中断资源。

## Deferred Work
* Linux kernel在两种上下文中运行。
    1. 进程上下文（process context），**可被阻塞**。（1）作为用户进程的操作运行在此环境，如system call kernel service routine；（2）**workqueue**和**threaded interrupt**都运行在进程上下文。
    2. 中断上下文（interrupt context），**不可被阻塞、调度（原子化）和睡眠**。**softirq**、**tasklet**和**timer**都运行在中断上下文。
* workqueue和（threaded irq的bottom half）是在kernel thread基础上实现的，因此可阻塞，运行在进程上下文中。
* tasklet和timer是在softirq基础上实现的，因此不可阻塞，运行在中断上下文中。

* Linux中断分为两部分：
    * top half（上半部）
        1. 要求占用时间少。
        2. 要求处理速度快。
        3. 期间所有中断都关闭，并调度bottom half作进一步处理。
        4. top half即handler本身。
    * bottom half（下半部）
        1. 时间要求不高。
        2. 期间中断使能。

### Softirq（软中断）
* 属于bottom half
* 运行在中断上下文。
* 为kernel subsystem准备，不可被设备驱动使用。
* 在所有IRQ handler运行结束后，才运行softirq，且可被top half抢占。
* HI_SOFTIRQ与TASKLET_SOFTIRQ用于运行tasklet。HI_SOFTIRQ优先级最高。

### Tasklet
* 属于bottom half。
* 在softirq基础上实现。
* 运行在中断上下文。
* 可被设备驱动使用，用`struct tasklet`表示。
* 运行期间所有中断可用，但一个tasklet一次只可在一个CPU上运行。
* 使用
    1. 静态方式
       >void handler(unsigned long data);  
        DECLARE_TASKLET(tasklet, handler, data);  
        DECLARE_TASKLET_DISABLED(tasklet, handler, data);
    2. 动态方式
       >void handler(unsigned long data);  
        struct tasklet_struct tasklet;  
        tasklet_init(&tasklet, handler, data);
    3. 中断服务函数中调度tasklet的执行
       >void tasklet_schedule(struct tasklet_struct *tasklet); //用TASKLET_SOFTIRQ softirq
        >
       >void tasklet_hi_schedule(struct tasklet_struct *tasklet); //用HI_SOFTIRQ softirq

### Timer
* 在softirq基础上实现。
* 运行在中断上下文。
* 可被设备驱动调用，以`struct timer_list`表示。
* 时间单位由`jiffie`表示。
    > jiffie与秒的换算  
    jiffies_value = seconds_value * HZ; //HZ为宏，代表一秒内多少个jiffie  
    seconds_value = jiffies_value / HZ;
* 使用timer
    1. 初始化timer。
    > void setup_timer(struct timer_list * timer, void (*function)(unsigned long),unsigned long data);
    2. 运行timer。
    > int mod_timer(struct timer_list *timer, unsigned long expires); //在`expires`秒后运行handler
    3. 停止timer，用del_timer()和del_timer_sync()。

### Threaded Interrupt
* 可运行在进程上下文和中断上下文，handler运行在中断上下文（属于top half），thread_handler运行在进程上下文（属于bottom half）。
* 推荐用 devm_request_threaded_irq()申请。
* devm_request_threaded_irq()参数：
    * `handler`可设NULL，会调用默认handler返回 IRQ_WAKE_THREAD，以调用对应的thread_handler。thread_handler运行期间，所有中断使能。
    * `flag`
        * IRQF_DISABLED，关闭所有中断。其他CPU也不能接收中断。
        * IRQF_SHARED，当前interrupt line可被其他handler共用。
        * IRQF_ONESHOT，当前interrupt line会在thread_handler运行过程中被激活（process context）。没有设置的话，当前interrupt line会在handler结束运行后重新激活（interrupt context）。

### Workqueue
* 运行在process context，可睡眠。
* 使用workqueue
    1. 声明与初始化一项`work`
    > #include <linux/workqueue.h>  
    >   
    >  //声明并初始化一项`work`  
    void my_work_handler(struct work_struct *work);  
    DECLARE_WORK(my_work, my_work_handler);  
    >  
    >//分开声明和初始化`work`  
    void my_work_handler(struct work_struct *work);  
    struct work_struct my_work;  
    INIT_WORK(&my_work, my_work_handler);

    2. 使`work`进入workqueue，参与调度
    > //运行在任意可行的CPU上  
    schedule_work(struct work_struct *work);  
    schedule_delayed_work(struct delayed_work *work, unsigned long delay); //基于timer使`work`过后再参与调度  
    >   
    > //运行在指定CPU或所有CPU上
    int schedule_delayed_work_on(int cpu, struct delayed_work *work, unsigned long delay);  
    int schedule_on_each_cpu(void(*function)(struct work_struct *));

    3. 使用flush_scheduled_work()去等待某个`work`完成。

    4. 额外创建`workqueue`情况
    > struct workqueue_struct *create_workqueue(const char *name); //每个CPU创建一个`workqueue`  
    struct workqueue_struct *create_singlethread_workqueue(const char *name); //仅创建一个`workqueue`  
    >  
    >  //为`workqueue`添加`work`  
    int queue_work(struct workqueue_struct *queue, struct work_struct *work);  
    int queue_delayed_work(struct workqueue_struct *queue, struct delayed_work *work, unsigned long delay);  
    >  
    > //等待所有`work`完成  
    void flush_workqueue(struct worksqueue_struct *queue);  
    >  
    > //销毁`workqueue`  
    void destroy_workqueue(structure workqueque_struct *queue);  

## Locking in the Kernel
* spinlock
    * 不断地获取。
    * 推荐用spin_lock_irqsave()，会关闭当前core的中断，但不影响其他core索求spinlock。因此，当获取了spinlock在进程上下文执行时，其他核处于中断上下文请求spinlock，实现spinlock在进程上下文和中断上下文的共享。

* mutex
    * 无法获取，就阻塞。
    * 如果数据一直处于用户态，应该使用mutex。
    * 推荐使用mutex_lock_interruptible()。

## Sleeping in the Kernel
* wait queue
> 静态声明和初始化  
DECLARE_WAIT_QUEUE_HEAD(name);  
>  
> 动态声明和初始化  
wait_queue_head_t my_queue;  
init_waitqueue_head(&my_queue);  
>   
>  设置等待事件，推荐使用可中断版本  
> wait_event(queue, condition);  
wait_event_interruptible(queue, condition);  
wait_event_timeout(queue, condition, timeout);  
wait_event_interruptible_timeout(queue, condition, timeout);  
>  
> 唤醒  
void wake_up(wait_queue_head_t *queue); /* wake_up wakes up all processes waiting on the given queue */  
void wake_up_interruptible(wait_queue_head_t *queue); /* restricts itself to processes performing an interruptible sleep */

## Kernel Thread
* 在进程上下文中，运行内核代码，无任何用户地址空间。
* kernel thread, workqueue的实现基础。

* 使用
> 创建kernel thread  
#include <linux/kthread.h>  
structure task_struct *kthread_create(int (*threadfn)(void *data),
 void *data, const char namefmt[], ...);  
 >  
 > 例子  
 kthread_create(f, NULL, "%skthread%d", "my", 0);  
 >  
 > 运行kernel thread  
 #include <linux/sched.h>  
int wake_up_process(struct task_struct *p);  
>  
> 创建并运行kernel thread  
struct task_struct *kthread_run(int (*threadfn)(void *data),
 void *data, const char namefmt[], ...);  
>  
> 停止kernel thread  
kthread_stop()

* kthread_stop()是会向对应kernel thread发送信号以终止。kernel thread在运行过程中，无法被中断，因此需要检查信号。


# Chapter 8 Allocating Kernel Memory
## Linux Address Types
* User Virtual Address，32位机器下分配1G高地址作为内核空间的话，0xC0000000为内核地址空间基地址。
![user virtual address](上传github，再分配地址)
* Physical Address，处理器与内存之间的地址。
* Bus Address，在外设地址与内存地址之间使用。可用IOMMU把bus address映射至内存，在配置DMA时必须要配置IOMMU。
* Kernel Logical Address，内核空间地址，`kmalloc()`返回的就是指向kernel logical address，实际是从**连续物理地址**映射至内存的。__pa(addr)可把kernel logical address转换为实际物理地址，__va(addr)可把实际物理地址转换为kernel logical address。
* Kernel Virtual Address，内核空间地址但**物理地址不连续**（vmalloc()返回）。`ioremap()`返回kernel virtual address。通过`iotable_init()`的机器特定的静态映射也是保存在kernel virtual address。

## Kernel Virtual to Physical Memory Mapping
* kernel physical memory可分为四个区域
    * ZONE_DMA，地址空间为kernel virtual address，利用`dma_alloc_xxx()`取得。
    * ZONE_NORMAL，地址空间为kernel logical address，利用`kmalloc()`取得。
    * ZONE_HIGHMEM，地址空间为kernel virtual address，利用`vmalloc()`取得。
    * Memory-Mapped I/O，地址空间为kernel virtual address，利用`ioremap()`取得。
![kernel memory layout](地址)

## Kernel Memory Allocators
![kernel memory allocator](地址)
* 主要为**page allocator**和**slab allocator**。
* slab allocator是基于page allocator实现的。
* kernel allocated memory不可被换出，也没有fault handler。

### Page Allocator
* 管理整个kernel的page分配。管理物理上连续的page，并映射至MMU page table中。
* page分配算法使用的是**Binary Buddy Allocator**(<https://www.kernel.org/doc/gorman/html/understand/understand009.html>)

* API
>unsigned long get_zeroed_page(int flags); /* Returns the virtual address of a free page, initialized to zero */  
unsigned long __get_free_page(int flags); /* Same, but doesn't initialize the contents */  
unsigned long __get_free_pages(int flags, unsigned int order); /* Returns the starting virtual address of an area of several contiguous pages in physical RAM, with the order being log2(number_of_pages. Can be computed from the size with the get_order() function */  
>  
> 常用flags  
GFP_KERNEL，日常使用，可能因空间不足而阻塞。  
GFP_ATOMIC，不允许阻塞，但空间不足会分配失败。  
GFP_DMA，用于DMA传输。





