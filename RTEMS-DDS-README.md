# RTEMS项目（ARM/realview_pbx_a9_qemu）
文档中的所有图片，在顶层目录下的images中提供
## 一、RTEMS的quick-start搭建
### 1、环境介绍
当前使用的是Ubuntu-22.04，RTEMS的版本是6.1
### 2、编译过程
线上只有源码或者各种配置文件，暂时不会有各种中间生成文件，参照的是官网的documentation
#### 1）step1源码下载
进入终端之后使用如下命令进行创建以及拉取源码
```
mkdir -p $HOME/RTEMS/quick-start/src
```
```
cd $HOME/RTEMS/quick-start/src
```
```
curl https://ftp.rtems.org/pub/rtems/releases/6/6.1/sources/rtems-source-builder-6.1.tar.xz | tar xJf -
```
```
mv rtems-source-builder-6.1 rsb
```
注：可以在任何地方创建这个多级目录结构（quick-start/src），但是原则上一般不这样干，如果随意定义位置的话，后面的某些命令中需要更改。（本文构建编译是修改了目录的）
#### 2）step2构建工具安装
```
cd $HOME/RTEMS/quick-start/src/rsb/rtems
```
安装bison、flex
```
sudo apt install -y bison flex build-essential
```
```
../source-builder/sb-set-builder --prefix=$HOME/RTEMS/quick-start/rtems/6 6/rtems-arm
```
注：如果上一步中自定义了目录结构位置，那么本步操作中--prefix指定的路径也要相应的修改。这是编译成arm架构，如果要换成其他架构，就在./config/6/下面去找

##### 工具安装成功检验
```
$HOME/RTEMS/quick-start/rtems/6/bin/arm-rtems6-gcc --version
```
输出截图
![工具安装成功](images/1.png "success")
#### 3）构建BSP
```
../source-builder/sb-set-builder --prefix=$HOME/RTEMS/quick-start/rtems/6 --target=arm-rtems6 --with-rtems-bsp=arm/realview_pbx_a9_qemu --with-rtems-tests=yes 6/rtems-kernel
```
###### 成功截图
![成功构建BSP](images/2.png "success")
### 3、构建测试程序（hello world）
#### 1）创建测试工程目录
```
mkdir -p $HOME/RTEMS/quick-start/app/hello
```
```
cd $HOME/RTEMS/quick-start/app/hello
```
#### 2）联网获取构建程序
```
curl https://waf.io/waf-2.0.19 > waf
```
```
chmod +x waf
```
#### 3）初始化工程并添加子模块
```
git init
```
```
git submodule add https://gitlab.rtems.org/rtems/tools/rtems_waf.git rtems_waf
```
#### 4）创建配置文件（init.c）
填入如下内容
```
/*
 * Simple RTEMS configuration
 */

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
```
#### 5）创建源文件（hello.c）
填入如下内容
```
/*
 * Hello world example
 */
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>

rtems_task Init(
  rtems_task_argument ignored
)
{
  printf( "\nHello World\n" );
  exit( 0 );
}
```
#### 6）创建构建脚本文件（wscript）
填入如下内容
```
#
# Hello world Waf script
#
from __future__ import print_function

rtems_version = "6"

try:
    import rtems_waf.rtems as rtems
except:
    print('error: no rtems_waf git submodule')
    import sys
    sys.exit(1)

def init(ctx):
    rtems.init(ctx, version = rtems_version, long_commands = True)

def bsp_configure(conf, arch_bsp):
    # Add BSP specific configuration checks
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure = bsp_configure)

def build(bld):
    rtems.build(bld)

    bld(features = 'c cprogram',
        target = 'hello.exe',
        cflags = '-g -O2',
        source = ['hello.c',
                  'init.c'])
```
#### 7）构建命令
```
./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 --rtems-bsp=arm/realview_pbx_a9_qemu
```
#### 8）构建成功截图
![demo构建成功](images/3.png "success")
#### 9）编译命令
```
./waf build
```
#### 10）编译成功截图
![demo编译成功](images/4.png "success")
#### 11）运行命令
使用qemu模拟运行程序
```
qemu-system-arm -no-reboot -nographic -M realview-pbx-a9 -m 256M -kernel ./build/arm-rtems6-realview_pbx_a9_qemu/hello.exe
```
#### 12）运行成功截图
![demo运行成功](images/5.png "success")
#### 13）提交代码（可选）
```
git add init.c hello.c wscript
git commit -m "My first RTEMS application."
```
## 二、ROS2移植适配（humble）
在quick-start/app目录下创建workspace工作空间，所有关于ROS的代码都在此目录下
### 1、DDS通信组件移植
在该目录下存在三个目录一个是code（源代码目录），一个config（编译构建脚本存放的目录），在config中存在两个脚本，一个是gen_run.sh是用于第一次构建以及拉取组件的环境代码，主要用于生成wscript脚本，run.sh是根据生成的wscript脚本，编译构建组件的静态库，所有的这些中间过程的代码都会在同级目录下的tmp目录中，并不会上传到github中，此后的组件编译构建的目录架构基本都是如此，后面不在赘述，还有一个是测试代码，用于测试组件的编译运行是否成功
#### 1）bsd构建
在$HOME/quick-start/src/bsd目录下
##### 1）bsd压缩包解压
```
tar -xJf rtems-libbsd-6.1.tar.xz
```
##### 2）进入目录
```
cd $HOME/RTEMS/quick-start/src/bsd
```
##### 3）运行构建命令
```
./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 --prefix=$HOME/RTEMS/quick-start/rtems/6 --rtems-bsp=arm/realview_pbx_a9_qemu
```
##### 4）编译构建
```
./waf build
```
##### 5）安装
```
./waf install
```
安装成功截图，安装成功之后会把libbsd.a这个库安装到$HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/lib下面去
![demo运行成功](images/6.png "success")
#### 2）tinyxml2的编译构建
进入到config目录下运行脚本
```
bash ./gen_run.sh
```
![demo运行成功](images/tinyxml2/1.png "success")
```
bash ./run.sh
```
![demo运行成功](images/tinyxml2/2.png "success")

静态库生成位置
相对目录：./tmp/build/arm-rtems5-realview_pbx_a9_qemu
#### 测试
在test测试目录下运行gen_run.sh去测试这个库
```
./gen_run.sh
```
![demo运行成功](images/tinyxml2/3.png "success")

在test测试目录下运行run.sh去测试这个库
```
./run.sh
```
![demo运行成功](images/tinyxml2/4.png "success")
#### 3）memory-main的编译构建
进入到config目录下运行脚本
```
./gen_run.sh
```
![demo运行成功](images/memory-main/1.png "success")
```
./run.sh
```
![demo运行成功](images/memory-main/2.png "success")

静态库生成位置
相对目录：/tmp/build/arm-rtems5-realview_pbx_a9_qemu
#### 测试
在test测试目录下运行gen_run.sh去测试这个库
```
./gen_run.sh
```
![demo运行成功](images/memory-main/3.png "success")

在test测试目录下运行run.sh去测试这个库
```
./run.sh
```
![demo运行成功](images/memory-main/4.png "success")
#### 4）Fast-CDR的编译构建
进入到config目录下运行脚本
```
./gen_run.sh
```
![demo运行成功](images/Fast_CDR/1.png "success")
```
./run.sh
```
![demo运行成功](images/Fast_CDR/2.png "success")

静态库生成位置
相对目录：/tmp/build/arm-rtems5-realview_pbx_a9_qemu
#### 测试
在test测试目录下运行gen_run.sh去测试这个库
```
./gen_run.sh
```
![demo运行成功](images/Fast_CDR/3.png "success")

在test测试目录下运行run.sh去测试这个库
```
./run.sh
```
![demo运行成功](images/Fast_CDR/4.png "success")
#### 5）Fast-DDS的编译构建
进入到config目录下运行脚本
```
./gen_run.sh
```
![demo运行成功](images/Fast_DDS/1.png "success")

运行run.sh的时候可以在后面添加参数inc，表示增量编译，不添加任何参数表示全量编译
```
./run.sh
```
![demo运行成功](images/Fast_DDS/2.png "success")

静态库生成位置
相对目录：/tmp/build/arm-rtems5-realview_pbx_a9_qemu
#### 测试
在test测试目录下运行gen_run.sh去测试这个库，这个测试用例是DDS通信组件自带的测试用例
```
./gen_run.sh
```
![demo运行成功](images/Fast_DDS/3.png "success")

在test测试目录下运行run.sh去测试这个库
```
./run.sh
```
![demo运行成功](images/Fast_DDS/4.png "success")

静态库生成位置
相对目录：/tmp/build/arm-rtems5-realview_pbx_a9_qemu

### 2、其余组件的移植适配
#### 组件依赖表
| 组件 | 依赖组件 |
|---------|---------|
| DDS   | 无   |
| yaml   | 无   |
| spdlog   | 无   |
| rcutils   | 无   |
| ament_index_cpp   | 无   |
| rcpputils   | rcutils   |
| logging   | rcutils rcpputils spdlog   |
| class_loader   | rcutils rcpputils   |
| rosidl   | rcutils rcpputils   |
| rmw   | rcutils rcpputils rosidl   |
| rmw_dds_common   | rcutils rcpputils rosidl rmw   |
| rmw_fastrtps   | DDS rcutils rcpputils rmw rosidl rmw_dds_common   |
| rmw_implementation   | rcutils rcpputils rmw ament_index_cpp   |
| builtin_interfaces   | rcutils DDS rosidl   |
| rcl_interfaces   | DDS rcutils rmw rosidl builtin_interfaces   |
| composition_interfaces   | rcl_interfaces rcutils rosidl DDS rmw   |
| unique_identifier_msgs   | rosidl rcutils DDS   |
| action_msgs   | rosidl unique_identifier_msgs builtin_interfaces rcutils DDS rmw   |
| example_interfaces   | rosidl rcutils unique_identifier_msgs builtin_interfaces action_msgs rmw DDS   |
| lifecycle_msgs   | rosidl rcutils DDS rmw   |
| rosgraph_msgs   | rosidl rcutils builtin_interfaces DDS   |
| rcl   | rmw rcutils rosidl rmw_implementation rcl_interfaces logging   |
| statistics_msgs   | rosidl rcutils builtin_interfaces DDS   |
| statistics_collector   | rcpputils rosidl builtin_interfaces rcl rmw statistics_msgs   |
| std_msgs   | rosidl rcutils DDS builtin_interfaces   |
| rclcpp   | rmw rcpputils rcutils rcl builtin_interfaces rcl_interfaces rosgraph_msgs statistics_msgs statistics_collector ament_index_cpp rosidl   |
#### 组件编译工具RTcolcon
运行workspace目录下的RTcolon脚本，也可以把脚本放到/usr/local/bin目录下，然后把RTcolcon当做工具使用
使用介绍：运行-h或者--help会有详细介绍

# 遇到的问题以及修复方案
## 一、构建工具安装报错
### 1、版本此次步骤的代码修复
#### 问题显示
![-l参数缺失报错](images/1.png "no library (-l) provided报错")
#### 错误解析
这个错误是配置脚本解析的时候本应该解析出-l后面链接的库，但是解析出来是空的
#### 定位问题
因为整个编译构建的过程是会在quick-start/src/rsb/rtems目录下面产生一个日志文件，我是通过打印自定义的一些输出到日志文件中（rtems使用%log进行输出的，例如：%log "=======test1====== %{变量名}"），去定位问题是出错在哪个配置文件的哪一行。最终定位在：quick-start/src/rsb/source-builder/config/gdb-common-1.cfg配置文件中的118行：%define gdb-python-lib-check %(%{_sbdir}/sb/rtems-build-dep -c %{__cc} %{gdb-host-libs} -l %{gdb-python-config-libs})。
#### 解决思路
编译更新的版本，看看官方是否已经进行了问题的修复，编译6.1版本的时候发现完全是可以编译成功的，因此可以找到对应的配置文件，看看配置文件是怎么解决的
#### 解决办法
发现较新的版本配置文件中对于python版本的判定更加的细致，所以可以借鉴新版本的方案以及代码，应用到5.1版本中去
```
将112行到116行的代码删去，换成6.1版本中对应的代码
%if %{gdb-python-ver-major} < 3 || \
    %{gdb-python-ver-major} == 3 && %{gdb-python-ver-minor} < 8
    %define gdb-python-config-lib-check-flags --ldflags
%else
    %define gdb-python-config-lib-check-flags --ldflags --embed
%endif

在83行代码下面添加如下两行代码
%define gdb-python-ver-major %(echo "%{gdb-python-ver-mm}" | sed -e 's/\..*//')
%define gdb-python-ver-minor %(echo "%{gdb-python-ver-mm}" | sed -e 's/.*\.//')
```
#### 成功截图
编译过程有点长，可能会持续20分钟左右
![成功](images/2.png "success")
## 二、DDS中的一些问题
1、在构建编译Fast-DDS组件的时候，自定义了一些宏用于屏蔽一些不支持的代码，例如屏蔽底层对于ipv6的支持以及TLS安全等，这些宏包括如下
```
ASIO_HAS_IPV6
ASIO_HAS_GETHOSTNAME
ASIO_HAS_STRERROR
ASIO_HAS_IP_MREQ_v6
ASIO_HAS_SIGFILLSET
BOOST_HAS_CLOCK
DDS_HAS_SHAERMEMORY
RTEMS_HAS_TLS
```
当这些宏全部定义之后这些代码才会编译进来

2、在asio中创建kqueue_reactor的时候使用的是pipe管道通信，虽然rtems支持管道，也支持kevent，但是不支持往kevent中注册pipe管道事件

3、代码底层获取mac地址的代码也被屏蔽掉了

4、$HOME/RTEMS/quick-start/app/workspace/DDS/Fast-DDS/code/thirdparty/boost/include/stub，在这个路径下是关于一些打桩的代码
## 三、ROS2其余组件的问题
整个过程采用静态库去构建，所以原生的ROS2在运行过程中动态加载动态库的特性就被破坏，所以搭建了一层框架用于在静态库在这方面的应用