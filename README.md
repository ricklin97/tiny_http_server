**简介：** 这是一个简单的基于Linux的多线程Web服务器，使用了C语言的socket函数，以及标准I/O函数。目前该服务器只支持了GET请求，后续可能会添加支持其他http请求，以及其他功能。



**目录：**

* htdocs文件夹：html代码放在该文件夹中

* tiny_hserv.c   ：可运行在Linux平台上的C语言代码
* content_type_form.txt：保存了content type的对照表，用于查询



**运行命令：**

```
$ gcc tiny_hserv.c -D_REENTRANT -o hserv -lpthread
$ ./hserv 9190
```



**运行示例：**

![示例图片1](https://github.com/ricklin97/tiny_http_server/blob/master/示例图片1.png)

![示例图片2](https://github.com/ricklin97/tiny_http_server/blob/master/示例图片2.png)

