
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#ifndef NGX_CYCLE_POOL_SIZE
#define NGX_CYCLE_POOL_SIZE     NGX_DEFAULT_POOL_SIZE
#endif


#define NGX_DEBUG_POINTS_STOP   1
#define NGX_DEBUG_POINTS_ABORT  2


typedef struct ngx_shm_zone_s  ngx_shm_zone_t;

typedef ngx_int_t (*ngx_shm_zone_init_pt) (ngx_shm_zone_t *zone, void *data);

struct ngx_shm_zone_s {
    void                     *data;
    ngx_shm_t                 shm;
    ngx_shm_zone_init_pt      init;
    void                     *tag;
    ngx_uint_t                noreuse;  /* unsigned  noreuse:1; */
};


struct ngx_cycle_s {
    /* ？？
    * 每个进程中都有一个唯一的ngx_cycle_t核心结构体，它有一个成员conf_ctx维护着所有模块的配置结构体
    * 保存着所有模块存储配置项的结构体指针，它首先是一个数组，数组大小为ngx_max_module，正好与Nginx的module个数一样；     
    * 每个数组成员又是一个指针，指向另一个存储着指针的数组，因此会看到void **** 
    */
    void                  ****conf_ctx;
    // 内存池
    ngx_pool_t               *pool;

    ngx_log_t                *log;
    /* 
    * 由nginx.conf配置文件读取到日志文件路径后，将开始初始化error_log日志文件，由于log对象还在用于输出日志到屏幕，    
    * 这时会用new_log对象暂时性地替代log日志，待初始化成功后，会用new_log的地址覆盖上面的log指针
    */
    ngx_log_t                 new_log;

    ngx_uint_t                log_use_stderr;  /* unsigned  log_use_stderr:1; */
    /*  
    * 对于poll，rtsig这样的事件模块，会以有效文件句柄数来预先建立这些ngx_connection t结构
    * 体，以加速事件的收集、分发。这时files就会保存所有ngx_connection_t的指针组成的数组，files_n就是指
    * 针的总数，而文件句柄的值用来访问files数组成员 
    */
    ngx_connection_t        **files;
    // 可用连接池
    ngx_connection_t         *free_connections;
    // 可用连接池连接总数
    ngx_uint_t                free_connection_n;
    // 模块数组
    ngx_module_t            **modules;
    ngx_uint_t                modules_n;
    ngx_uint_t                modules_used;    /* unsigned  modules_used:1; */
    // 可重复使用连接队列  双向链表
    ngx_queue_t               reusable_connections_queue;
    // 动态数组  每个元素为ngx_listening_t，用于监听端口及相关参数
    ngx_array_t               listening;
    // 动态数组  保存着Nginx所有要操作的目录
    ngx_array_t               paths;
    ngx_array_t               config_dump;
    /* 
    * 单链表容器，元素类型是ngx_open_file_t结构体，它表示nginx已经打开的所有文件。事实上，nginx框架不会向open_files链表中添加文件。    
    * 而是由对此感兴趣的模块向其中添加文件路径名，nginx框架会在ngx_init_cycle方法中打开这些文件  
    * 该链表中所包含的文件的打开在ngx_init_cycle中打开  
    */
    ngx_list_t                open_files;
    // 单链表容器，元素类型是ngx_shm_zone_t结构体，每个元素表示一块共享内存
    ngx_list_t                shared_memory;
    // 当前进程中所有连接对象总数
    ngx_uint_t                connection_n;
    // 每个进程能够打开的最多文件数  赋值见ngx_event_process_init
    ngx_uint_t                files_n;
    // 指向当前进程中的所有连接对象
    ngx_connection_t         *connections;
    // 指向当前进程中的所有读事件对象
    ngx_event_t              *read_events;
    // 指向当前进程中的所有写事件对象
    ngx_event_t              *write_events;
    /*   
    * 旧的ngx_cycle_t 对象用于引用上一个ngx_cycle_t 对象中的成员。例如ngx_init_cycle 方法，在启动初期，    
    * 需要建立一个临时的ngx_cycle_t对象保存一些变量， 
    * 再调用ngx_init_cycle 方法时就可以把旧的ngx_cycle_t 对象传进去，而这时old_cycle对象就会保存这个前期的ngx_cycle_t对象
    */
    ngx_cycle_t              *old_cycle;
    // 配置文件相对于安装目录的路径名称 默认为安装路径下的NGX_CONF_PATH,见ngx_process_options
    ngx_str_t                 conf_file;
    // nginx处理配置文件时需要特殊处理的在命令行携带的参数，一般是-g 选项携带的参数
    ngx_str_t                 conf_param;
    // nginx配置文件所在目录的路径  ngx_prefix见ngx_process_options
    ngx_str_t                 conf_prefix;
    // nginx安装目录的路径 ngx_prefix 见ngx_process_options
    ngx_str_t                 prefix;
    // 用于进程间同步的文件锁名称
    ngx_str_t                 lock_file;
    ngx_str_t                 hostname;
};


typedef struct {
    ngx_flag_t                daemon;
    ngx_flag_t                master;

    ngx_msec_t                timer_resolution;

    ngx_int_t                 worker_processes;
    ngx_int_t                 debug_points;

    ngx_int_t                 rlimit_nofile;
    off_t                     rlimit_core;

    int                       priority;

    ngx_uint_t                cpu_affinity_auto;
    ngx_uint_t                cpu_affinity_n;
    ngx_cpuset_t             *cpu_affinity;

    char                     *username;
    ngx_uid_t                 user;
    ngx_gid_t                 group;

    ngx_str_t                 working_directory;
    ngx_str_t                 lock_file;

    ngx_str_t                 pid;
    ngx_str_t                 oldpid;

    ngx_array_t               env;
    char                    **environment;
} ngx_core_conf_t;


#define ngx_is_init_cycle(cycle)  (cycle->conf_ctx == NULL)


ngx_cycle_t *ngx_init_cycle(ngx_cycle_t *old_cycle);
ngx_int_t ngx_create_pidfile(ngx_str_t *name, ngx_log_t *log);
void ngx_delete_pidfile(ngx_cycle_t *cycle);
ngx_int_t ngx_signal_process(ngx_cycle_t *cycle, char *sig);
void ngx_reopen_files(ngx_cycle_t *cycle, ngx_uid_t user);
char **ngx_set_environment(ngx_cycle_t *cycle, ngx_uint_t *last);
ngx_pid_t ngx_exec_new_binary(ngx_cycle_t *cycle, char *const *argv);
ngx_cpuset_t *ngx_get_cpu_affinity(ngx_uint_t n);
ngx_shm_zone_t *ngx_shared_memory_add(ngx_conf_t *cf, ngx_str_t *name,
    size_t size, void *tag);


extern volatile ngx_cycle_t  *ngx_cycle;
extern ngx_array_t            ngx_old_cycles;
extern ngx_module_t           ngx_core_module;
extern ngx_uint_t             ngx_test_config;
extern ngx_uint_t             ngx_dump_config;
extern ngx_uint_t             ngx_quiet_mode;


#endif /* _NGX_CYCLE_H_INCLUDED_ */
