#reading-and-annotate-mongodb-4.4
  
  
### mongodb-4.4最新版本内核源码中文模块化注释详细分析      
  [mongodb-4.4版本内核源码中文注释分析](https://github.com/y123456yz/reading-and-annotate-mongodb-4.4)    
  
### mongodb-3.6版本内核源码中文模块化注释详细分析      
  [mongodb-3.6版本内核源码中文注释分析](https://github.com/y123456yz/reading-and-annotate-mongodb-3.6)    
        
### 对外演讲   
|#|对外演讲|演讲内容|
|:-|:-|:-|
|1|Qcon全球软件开发大会分享|[OPPO万亿级文档数据库MongoDB集群性能优化实践](https://qcon.infoq.cn/2020/shenzhen/track/916)|
|2|2021年度Gdevops全球敏捷运维峰会|[PB级万亿数据库性能优化及最佳实践](https://gdevops.com/index.php?m=content&c=index&a=lists&catid=87)|
|3|2019年mongodb年终盛会|[OPPO百万级高并发MongoDB集群性能数十倍提升优化实践](https://www.shangyexinzhi.com/article/428874.html)|
|4|2020年mongodb年终盛会|[万亿级文档数据库集群性能优化实践](https://mongoing.com/archives/76151)|
|5|2021年dbaplus分享|[万亿级文档数据库集群性能优化实践](http://dbaplus.cn/news-162-3666-1.html)|

  
### 专栏  
|#|专栏名|专栏内容|
|:-|:-|:-|
|1|infoq专栏|[《MongoDB内核源码设计、性能优化、最佳运维实践》](https://www.infoq.cn/profile/8D2D4D588D3D8A/publish)|
|2|oschina专栏|[《mongodb内核源码中文注释详细分析及性能优化实践系列》](https://my.oschina.net/u/4087916)|
|3|知乎专栏|[《MongoDB内核源码设计、性能优化、最佳运维实践》](https://www.zhihu.com/people/yang-ya-zhou-42/columns)|
|4|itpub专栏|[《mongodb内核源码设计实现、性能优化、最佳运维实践》](http://blog.itpub.net/column/150)|

### <<千万级峰值tps/十万亿级数据量文档数据库内核研发及运维之路>>   
|#|文章内容|
|:-|:-|
|1|[盘点 2020 - 我要为分布式数据库 mongodb 在国内影响力提升及推广做点事](https://xie.infoq.cn/article/372320c6bb93ddc5b7ecd0b6b)|
|2|[万亿级数据库 MongoDB 集群性能数十倍提升及机房多活容灾实践](https://xie.infoq.cn/article/304a748ad3dead035a449bd51)|
|3|[Qcon现代数据架构 -《万亿级数据库 MongoDB 集群性能数十倍提升优化实践》核心 17 问详细解答](https://xie.infoq.cn/article/0c51f3951f3f10671d7d7123e)|
|4|[数百万级代码量mongodb内核源码阅读经验分享](https://xie.infoq.cn/article/7b2c1dc67de82972faac2812c)|
|5|[话题讨论 - mongodb 相比 mysql 拥有十大核心优势，为何国内知名度不高？](https://xie.infoq.cn/article/180d98535bfa0c3e71aff1662)|
|6|[万亿级数据库 MongoDB 集群性能数十倍提升及机房多活容灾实践](https://xie.infoq.cn/article/304a748ad3dead035a449bd51)|
|7|[百万级高并发mongodb集群性能数十倍提升优化实践(上篇)](https://my.oschina.net/u/4087916/blog/3141909)|
|8|[百万级高并发mongodb集群性能数十倍提升优化实践(下篇)](https://my.oschina.net/u/4087916/blog/3155205)|
|9|[Mongodb网络传输处理源码实现及性能调优-体验内核性能极致设计](https://my.oschina.net/u/4087916/blog/4295038)|
|10|[常用高并发网络线程模型设计及mongodb线程模型优化实践(最全高并发网络IO线程模型设计及优化)](https://my.oschina.net/u/4087916/blog/4431422) |
|11|[Mongodb集群搭建一篇就够了-复制集模式、分片模式、带认证、不带认证等(带详细步骤说明)](https://my.oschina.net/u/4087916/blog/4661542)|
|12|[Mongodb特定场景性能数十倍提升优化实践(记一次mongodb核心集群雪崩故障)](https://blog.51cto.com/14951246)|
|13|[mongodb内核源码设计实现、性能优化、最佳运维系列-mongodb网络传输层模块源码实现二](https://zhuanlan.zhihu.com/p/265701877)|
|14|[为何需要对开源mongodb社区版本做二次开发，需要做哪些必备二次开发](https://github.com/y123456yz/reading-and-annotate-mongodb-3.6.1/blob/master/development_mongodb.md)|
|15|[对开源mongodb社区版本做二次开发收益列表](https://my.oschina.net/u/4087916/blog/3063529)|
|16|[盘点 2020 - 我要为分布式数据库 mongodb 在国内影响力提升及推广做点事](https://xie.infoq.cn/article/372320c6bb93ddc5b7ecd0b6b)|
|17|[300 条数据变更引发的血案 - 记某十亿级核心 mongodb 集群部分请求不可用故障踩坑记](https://xie.infoq.cn/article/5932858d57db13d43a8b8d62a)|  
|18|[记十亿级Es数据迁移mongodb成本节省及性能优化实践](https://zhuanlan.zhihu.com/p/373351625)|  
|19|[千亿级数据迁移mongodb成本节省及性能优化实践](https://zhuanlan.zhihu.com/p/376679225)|  
|20|[千亿级数据迁移 mongodb 成本节省及性能优化实践 (附性能对比质疑解答)](https://xie.infoq.cn/article/2bc78d36adef6832ada8ea7c5)|  
|21|[记某百亿级 mongodb 集群数据过期性能优化实践](https://xie.infoq.cn/article/98daf7330a3107fa0bf1edc9c)|  
|27|[mongodb内核源码实现、性能调优、最佳运维实践系列-数百万行mongodb内核源码阅读经验分享](https://my.oschina.net/u/4087916/blog/4696104)|  
|28|[mongodb内核源码实现、性能调优、最佳运维实践系列-mongodb网络传输层模块源码实现一](https://my.oschina.net/u/4087916/blog/4295038)|
|29|[mongodb内核源码实现、性能调优、最佳运维实践系列-mongodb网络传输层模块源码实现二](https://my.oschina.net/u/4087916/blog/4674521)|
|30|[mongodb内核源码实现、性能调优、最佳运维实践系列-mongodb网络传输层模块源码实现三](https://my.oschina.net/u/4087916/blog/4678616)|
|31|[mongodb内核源码实现、性能调优、最佳运维实践系列-mongodb网络传输层模块源码实现四](https://my.oschina.net/u/4087916/blog/4685419)|
|32|[mongodb内核源码实现、性能调优、最佳运维实践系列-command命令处理模块源码实现一](https://my.oschina.net/u/4087916/blog/4709503)|
|33|[mongodb内核源码实现、性能调优、最佳运维实践系列-command命令处理模块源码实现二](https://my.oschina.net/u/4087916/blog/4748286)|
|34|[mongodb内核源码实现、性能调优、最佳运维实践系列-command命令处理模块源码实现三](https://my.oschina.net/u/4087916/blog/4782741)|
|35|[mongodb内核源码实现、性能调优、最佳运维实践系列-记mongodb详细表级操作及详细时延统计实现原理(教你如何快速进行表级时延问题分析)](https://xie.infoq.cn/article/3184cdc42c26c86e2749c3e5c)|
|36|[mongodb内核源码实现、性能调优、最佳运维实践系列-Mongodb write写(增、删、改)模块设计与实现](https://my.oschina.net/u/4087916/blog/4974132)|
  

### 《mongodb-3.6内核源码设计与实现》源码模块化分析  
#### 第一阶段：单机内核源码分析  
![单机模块化架构图](/image/单机模块化架构图.png)  
|#|单机模块名|核心代码中文注释|说明|模块文档输出|
|:-|:-|:-|:-|:-|
|1|[网络收发处理(含工作线程模型)](https://github.com/y123456yz/reading-and-annotate-mongodb-3.6/blob/master/mongo/README.md#L8)|网络处理模块核心代码实现(100%注释分析)|完成ASIO库、网络数据收发、同步线程模型、动态线程池模型等功能|[详见infoq专栏：《MongoDB内核源码设计、性能优化、最佳运维实践》](https://www.infoq.cn/profile/8D2D4D588D3D8A/publish)|
|2|[command命令处理模块](https://github.com/y123456yz/reading-and-annotate-mongodb-3.6/blob/master/mongo/README.md#L85)|命令处理相关模块源码分析(100%注释分析)|完成命令注册、命令执行、命令分析、命令统计等功能|[详见oschina专栏:《mongodb内核源码中文注释详细分析及性能优化实践系列》](https://www.infoq.cn/profile/8D2D4D588D3D8A/publish)|
|3|[write写(增删改操作)模块](https://github.com/y123456yz/reading-and-annotate-mongodb-3.6/blob/master/mongo/README.md#L115))|增删改写模块(100%注释分析)|完成增删改对应命令解析回调处理、事务封装、storage存储模块对接等功能|[详见知乎专栏：《MongoDB内核源码设计、性能优化、最佳运维实践》](https://www.zhihu.com/people/yang-ya-zhou-42/columns)|
|4|[query查询引擎模块](https://github.com/y123456yz/reading-and-annotate-mongodb-3.6/blob/master/mongo/README.md#L131))|query查询引擎模块(核心代码注释)|完成expression tree解析优化处理、querySolution生成、最优索引选择等功能|[详见知乎专栏：《MongoDB内核源码设计、性能优化、最佳运维实践》](https://www.zhihu.com/people/yang-ya-zhou-42/columns)|
|5|[concurrency并发控制模块](https://github.com/y123456yz/reading-and-annotate-mongodb-3.6/tree/master/mongo/src/mongo/db/concurrency)|并发控制模块(核心代码注释)|完成信号量、读写锁、读写意向锁相关实现及封装|[详见infoq专栏：《MongoDB内核源码设计、性能优化、最佳运维实践》](https://www.infoq.cn/profile/8D2D4D588D3D8A/publish)|
|6|[index索引模块](https://github.com/y123456yz/reading-and-annotate-mongodb-3.6/blob/master/mongo/README.md#L240)|index索引模块(100%注释分析)|完成索引解析、索引管理、索引创建、文件排序等功能|[详见oschina专栏:《mongodb内核源码中文注释详细分析及性能优化实践系列》](https://www.infoq.cn/profile/8D2D4D588D3D8A/publish)|
|7|[storage存储模块](https://github.com/y123456yz/reading-and-annotate-mongodb-3.6/blob/master/mongo/README.md#L115))|storage存储模块(100%注释分析)|完成存储引擎注册、引擎选择、中间层实现、KV实现、wiredtiger接口实现等功能|[详见知乎专栏：《MongoDB内核源码设计、性能优化、最佳运维实践》](https://www.zhihu.com/people/yang-ya-zhou-42/columns)|
|8|[wiredtiger存储引擎](https://github.com/y123456yz/reading-and-annotate-wiredtiger-3.0.0)) |wiredtiger存储引擎设计与实现专栏分析(已分析部分)|完成KV读写、存储结构、checkpoint择等主功能，待完善|[详见知乎专栏：《MongoDB内核源码设计、性能优化、最佳运维实践》](https://github.com/y123456yz/reading-and-annotate-wiredtiger-3.0.0)|
  
  
#### 第二阶段：复制集内核源码分析(已分析部分源码，待整理,持续分析)  
    
    
#### 第三阶段：sharding分片内核源码分析(已分析部分源码，待整理，持续分析)   
      
#### 第四阶段：wiredtiger存储引擎源码分析(已分析部分源码，待整理，持续分析)  
  
#### 第五阶段：重新回顾分析mongodb内核主模块以外细节(已分析部分源码，待整理，持续分析) 
   

     
### 其他  
#### nginx高并发设计优秀思想应用于其他高并发代理中间件:   
  * [高性能 -Nginx 多进程高并发、低时延、高可靠机制在百万级缓存 (redis、memcache) 代理中间件中的应用](https://xie.infoq.cn/article/2ee961483c66a146709e7e861)  

#### redis、nginx、memcache、twemproxy、mongodb等更多中间件，分布式系统，高性能服务端核心思想实现博客:    
  * [中间件、高性能服务器、分布式存储等(redis、memcache、pika、rocksdb、mongodb、wiredtiger、高性能代理中间件)二次开发、性能优化，逐步整理文档说明并配合demo指导](https://github.com/y123456yz/middleware_development_learning)    
      
