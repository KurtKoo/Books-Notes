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

![linux device model](https://github.com/KurtKoo/Books-Notes/tree/master/Linux%20Driver%20Development%20with%20Raspberry%20Pi/img/chap2)

