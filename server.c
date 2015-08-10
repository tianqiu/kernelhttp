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
char * dealrequest(char *recvbuf,char *buf2);



struct socket *sock;
static struct workqueue_struct *my_wq;
struct work_struct_data  
{  
    struct work_struct my_work;         //表示一个工作  
    struct socket * client;              //传给处理函数的数据(client socket)  
};



char * dealrequest(char *recvbuf,char *buf2)
{
    char *response=NULL;
    char *method=NULL;
    char *url=NULL;
    char error[]={"HTTP/1.1 200 OK \r\nContent-Type: text/html\r\n\r\n<html><body><p>hello</p><p>There is some errors</p></body><html>"};
    //char *path=NULL;
    method=strsep(&recvbuf," ");
    url=strsep(&recvbuf," ");
    printk("\nmethod==%s\n",method);
    printk("\nurl==%s\n",url);
    if(url==NULL)
    {
        printk("\nnullnullnull\n");
        response=(char *)kmalloc(strlen(error)+1,GFP_KERNEL);
        strcpy(response,error);
        return response;
    }
    //path=strsep(&url,"?");
    if(strcmp(url,"/")==0)
    {
        struct file *fp;
        mm_segment_t fs;
        int ret=0;
        int iFileLen = 0;
        loff_t pos;
        pos = 0;
        //printk("hello enter\n");
        fp = filp_open("/home/qiutian/c/www/index3.html", O_RDWR | O_CREAT, 0644);
        if (IS_ERR(fp)) 
        {
       //printk("create file error\n");
            response=(char *)kmalloc(strlen(error)+1,GFP_KERNEL);
            strcpy(response,error);
            return response;
        }
        iFileLen = vfs_llseek(fp, 0, SEEK_END);
       // printk("lenshi:%d", iFileLen);
        char buf1[iFileLen+1];
        memset(buf1,0,iFileLen+1);    
        fs = get_fs();
        set_fs(KERNEL_DS);
        ret=vfs_read(fp, buf1, iFileLen, &pos);
        filp_close(fp, NULL);
        set_fs(fs);   
        response=(char *)kmalloc(strlen(buf1)+1,GFP_KERNEL);
        strcpy(response,buf1);
        return response;    
    }
    /*else if(strcmp(method,"GET")==0 || strcmp(method,"HEAD")==0)
    {
        
    }*/
    response=(char *)kmalloc(strlen(error)+1,GFP_KERNEL);
    strcpy(response,error);
    printk("\nout\n");
    return response; 
}

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
      
        char *buf2;
        buf2=dealrequest(recvbuf,buf2);
        kfree(recvbuf); 
         //printk("\nbuf2 de dihzhi:%d\n",buf2);
        //printk("\n\n%s\n\n",buf2);
    
        //send message to client ///////////////////////////////
        int len;
        //iFileLen=sizeof(buf2);
        len=strlen(buf2)*sizeof(char);
        //printk("\n33==%s\nlen=%d\n",buf2,len);
        struct kvec vec2;  
        struct msghdr msg2;  
        vec2.iov_base=buf2; 
        vec2.iov_len=len;  
        memset(&msg2,0,sizeof(msg2));
        ret= kernel_sendmsg(wsdata->client,&msg2,&vec2,1,len);
        kfree(buf2);
        buf2=NULL;
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
    //printk("server:socket_create ok!\n");  
  
    /*set the socket can be reused*/  
    int val=1;  
    ret= kernel_setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val));  
    if(ret)
    {  
       // printk("kernel_setsockopt error!!!!!!!!!!!\n");  
    }  
  
    /*bind the socket*/  
    ret=sock->ops->bind(sock,(struct sockaddr *)&s_addr,sizeof(struct sockaddr_in));
    if(ret<0)
    {  
       // printk("server: bind error\n");  
        return ret;  
    }  
    //printk("server:bind ok!\n");  
    
    /*listen*/  
    ret=sock->ops->listen(sock,10);  
    if(ret<0)
    {  
       // printk("server: listen error\n");  
        return ret;  
    }  
    //printk("server:listen ok!\n");  
    

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
        int ret = 0;  
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