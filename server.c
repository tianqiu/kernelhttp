#include <linux/in.h>  
#include <linux/inet.h>  
#include <linux/socket.h>  
#include <net/sock.h>  
#include <linux/string.h>
#include <linux/init.h>  
#include <linux/module.h>  
#include <linux/posix_types.h>
#include <uapi/linux/eventpoll.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/workqueue.h> 
#include <linux/slab.h> 
#include <linux/sched.h>
#include <linux/kmod.h>


struct socket *sock;
static struct workqueue_struct *my_wq;
struct work_struct_data  
{  
    struct work_struct my_work;         //表示一个工作  
    struct socket * client;              //传给处理函数的数据(client socket)  
};


static void work_handler(struct work_struct *work)  
{
        struct work_struct_data *wsdata = (struct work_struct_data *)work;  
        char *recvbuf=NULL;  
        recvbuf=kmalloc(1024,GFP_KERNEL);  
        if(recvbuf==NULL)
        {  
            printk("server: recvbuf kmalloc error!\n");  
            return ;  
        }  
        memset(recvbuf, 0, sizeof(recvbuf));  
          
        //receive message from client  
        struct kvec vec;  
        struct msghdr msg;  
        memset(&vec,0,sizeof(vec));  
        memset(&msg,0,sizeof(msg));  
        vec.iov_base=recvbuf;  
        vec.iov_len=1024;  
        int ret=0;
        ret=kernel_recvmsg(wsdata->client,&msg,&vec,1,1024,0);  
        //printk("receive message:\n%s\n",recvbuf); 
        //printk("receive size=%d\n",ret);  
      
        //char *buf2;
        //buf2=dealrequest(recvbuf,buf2);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        struct socket *urlsock;
        struct sockaddr_in s_addr2;  
        unsigned short portnum2=9999;        
        memset(&s_addr2,0,sizeof(s_addr2));
        s_addr2.sin_family=AF_INET;  
        s_addr2.sin_port=htons(portnum2);  
        s_addr2.sin_addr.s_addr=in_aton("127.0.0.1");  
        urlsock=(struct socket *)kmalloc(sizeof(struct socket),GFP_KERNEL);  
      
    /*create a socket*/  
        ret=sock_create_kern(AF_INET, SOCK_STREAM,0,&urlsock);  
        if(ret<0){  
            printk("client:socket create error!\n");  
            return ;  
        }  
        ret=urlsock->ops->connect(urlsock,(struct sockaddr *)&s_addr2, sizeof(s_addr2),0);  
        if(ret!=0){  
            printk("client:connect error!\n");  
            return ;  
        }  
        //send to url.py

        int len=strlen(recvbuf)+1;    
        struct kvec vecsend;  
        struct msghdr msgsend;  
      
        vecsend.iov_base=recvbuf;  
        vecsend.iov_len=len;  
        printk("\nrecvbuf22==%slen==%d\n",recvbuf,len);
        memset(&msgsend,0,sizeof(msgsend));  
      
        ret= kernel_sendmsg(urlsock,&msgsend,&vecsend,1,len);  
        if(ret<0){  
            printk("client: kernel_sendmsg error!\n");  
            return ;   
        }
        else if(ret!=len){  
            printk("client: ret!=len\n");  
        }  
        kfree(recvbuf); 
        //recv from url.py

        char *urlpy=NULL;  
        urlpy=kmalloc(1024000,GFP_KERNEL);  
        if(urlpy==NULL)
        {  
            printk("server: recvbuf kmalloc error!\n");  
            return ;  
        }  
        memset(urlpy, 0, sizeof(urlpy));  

        struct kvec vec3;  
        struct msghdr msg3;  
        memset(&vec3,0,sizeof(vec3));  
        memset(&msg3,0,sizeof(msg3));  
        vec3.iov_base=urlpy;  
        vec3.iov_len=1024000;  
        ret=kernel_recvmsg(urlsock,&msg3,&vec3,1,1024000,0);  
        sock_release(urlsock);  


        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
        //send message to client ///////////////////////////////
        printk("urlpy==%s",urlpy);
        len=strlen(urlpy)*sizeof(char);
        struct kvec vec2;  
        struct msghdr msg2;  
        vec2.iov_base=urlpy; 
        vec2.iov_len=len;  
        memset(&msg2,0,sizeof(msg2));
        ret= kernel_sendmsg(wsdata->client,&msg2,&vec2,1,len);
        kfree(urlpy);
        urlpy=NULL;
        //release client socket
        sock_release(wsdata->client);  
}  


int myserver(void)
{      
    struct socket *client_sock;  
    struct sockaddr_in s_addr;  
    unsigned short portnum=8888;  
    int ret=0;  

    memset(&s_addr,0,sizeof(s_addr));  
    s_addr.sin_family=AF_INET;  
    s_addr.sin_port=htons(portnum);  
    s_addr.sin_addr.s_addr=htonl(INADDR_ANY);  
    
    sock=(struct socket *)kmalloc(sizeof(struct socket),GFP_KERNEL);
    client_sock=(struct socket *)kmalloc(sizeof(struct socket),GFP_KERNEL);   
    /*create a socket*/  
    ret=sock_create_kern(AF_INET, SOCK_STREAM,0,&sock);  
    if(ret)
    {  
        printk("server:socket_create error!\n");  
    }  
    printk("server:socket_create ok!\n");  
  
    /*set the socket can be reused*/  
    int val=1;  
    ret= kernel_setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val));  
    if(ret)
    {  
        printk("kernel_setsockopt error!!!!!!!!!!!\n");  
    }  
  
    /*bind the socket*/  
    ret=sock->ops->bind(sock,(struct sockaddr *)&s_addr,sizeof(struct sockaddr_in));
    if(ret<0)
    {  
        printk("server: bind error\n");  
        return ret;  
    }  
    printk("server:bind ok!\n");  
    
    /*listen*/  
    ret=sock->ops->listen(sock,10);  
    if(ret<0)
    {  
        printk("server: listen error\n");  
        return ret;  
    }  
    printk("server:listen ok!\n");  
    

    my_wq = create_workqueue("my_queue");
    while(1)
    {
        ret=1;
        struct work_struct_data * wsdata;
        ret = kernel_accept(sock,&client_sock,100);  
        printk("server:accept ing!,ret=%d\n",ret); 
        if(ret<0)
        {  
            printk("server:accept error!,ret=%d\n",ret);  
            //return ret; 
            break; 
        }  
        if (my_wq) 
        {
            wsdata = (struct work_struct_data *) kmalloc(sizeof(struct work_struct_data), GFP_KERNEL);
                  //  设置要传递的数据  
            wsdata->client = client_sock;  
            if (wsdata)  
            {
                //初始化work_struct类型的变量（主要是指定处理函数）  
                INIT_WORK(&wsdata->my_work, work_handler);  
                //将work添加到刚创建的工作队列中  
                ret = queue_work(my_wq, &wsdata->my_work);  
            }  
        }  
        //printk("server: accept ok, Connection Established,ret=%d\n",ret);    
    }
    
    sock_release(sock);  
    return ret;  
}  
  




static int server_init(void)
{
    printk("server init:\n");  
    myserver();  
    return 0;  
}         
  
static void server_exit(void){  
    printk("good bye\n");  
    flush_workqueue(my_wq);  
    //销毁工作队列  
    destroy_workqueue(my_wq);  
}  

module_init(server_init);  
module_exit(server_exit);    
MODULE_LICENSE("GPL");
