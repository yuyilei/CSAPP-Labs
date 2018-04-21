//#include "csapp.c"
#include "csapp.c"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define SBUFSIZE 16
#define NTHREADS 8 

typedef struct cache_block cache_block ; 
struct cache_block{
  char request[MAXLINE] ;
  char *content ;
  size_t size ;
  cache_block *next ;
  cache_block *prev ;
} ;

size_t cache_size = 0 ;
cache_block *head = NULL ;
cache_block *tail = NULL ;
pthread_mutex_t mutex_lock ;

typedef struct {
    int *buf ; 
    int n ; 
    int front ;
    int rear ; 
    sem_t mutex ; 
    sem_t slots ; 
    sem_t items ;  
} sbuf_t ; 


void *thread(void *vargp) ; 
void doit(int fd) ; 
void clienterror(int fd , char  *cause , char *errnum , char *shortmsg , char *longmsg ) ;
void parse_url(char *uri , char *hostname, char *filename, char*port) ; 
void connect_server(int fd, char *hostname, char *port, char *filename, rio_t *rio) ; 
void sbuf_init(sbuf_t *sp, int n) ; 
void sbuf_deinit(sbuf_t *sp) ; 
void sbuf_insert(sbuf_t *sp, int item) ; 
int  sbuf_remove(sbuf_t *sp ) ;
void getout_block(cache_block *block) ; 
int  find_and_insert_cache(char *request, char *response, size_t *size) ; 
void save_to_cache(char *request, char *response, size_t size) ; 
void add_block(cache_block* block) ; 
void delete_block(cache_block *block) ; 

sbuf_t sbuf ; 

int main(int argc, char **argv){ 
    if ( argc != 2 ){
        fprintf(stderr,"usage: %s <port>\n", argv[0]) ; 
        exit(1) ; 
    }
    pthread_mutex_init(&mutex_lock, NULL) ;
    // Signal(SIGPIPE, SIG_IGN) ;  
    int listenfd, *connfd ; 
    char hostname[MAXLINE], port[MAXLINE] ; 
    socklen_t clientlen ; 
    struct sockaddr_storage clientaddr ; 
    listenfd = Open_listenfd(argv[1]) ; 
    pthread_t tid ; 
    sbuf_init(&sbuf,SBUFSIZE) ; 
    int i ; 
    for ( i = 0 ; i < NTHREADS ; i++ )
        Pthread_create(&tid,NULL,thread,NULL) ; 
    while (1){
        clientlen = sizeof(clientaddr) ; 
        connfd = Malloc(sizeof(int)) ; 
        *connfd = Accept(listenfd,(SA *)&clientaddr,&clientlen) ; 
        Getnameinfo((SA*) &clientaddr,clientlen,hostname,MAXLINE,port,MAXLINE,0 ) ;
        sbuf_insert(&sbuf,*connfd) ; 
    }
    Free(connfd) ; 
    sbuf_deinit(&sbuf) ; 
    return 0;
}

void *thread(void *vargp){
    pthread_detach(pthread_self()) ; 
    while (1){
        int connfd = sbuf_remove(&sbuf) ; 
        doit(connfd) ; 
        Close(connfd) ; 
    }
}

void doit(int fd) {
    struct stat sbuf ;
    char buf[MAXLINE] , method[MAXLINE] , uri[MAXLINE] , version[MAXLINE] ;
    char filename[MAXLINE] , hostname[MAXLINE] , port[MAXLINE] , path[MAXLINE] ;
    char proxy_buf[MAXBUF] ; 
    char all_buf[MAX_OBJECT_SIZE] ;
    rio_t rio ;
    size_t num  = 0 ; 
    int filelen = 0, all_num = 0  ; 
    Rio_readinitb(&rio,fd) ;
    Rio_readlineb(&rio,buf,MAXLINE ) ;
    sscanf(buf,"%s %s %s",method,uri,version) ;
    if ( find_and_insert_cache(buf, proxy_buf,&num) != 0 ){
        Rio_writen(fd,proxy_buf,num) ;   
        return ;  
    }  
    if (strcasecmp(method,"GET")) {
        clienterror(fd, method , "501" , "Not implemented","Tiny does not implement this method") ;
        return ;
    }
    parse_url(uri,hostname,filename,port) ; 
    rio_t proxy_rio ; 
    int proxy_fd = open_clientfd(hostname,port) ; 
    if ( proxy_fd < 0 ){
        Close(proxy_fd) ;  
        unix_error("Can not connect to the final server!\n") ; 
    }
    connect_server(proxy_fd,hostname,port,filename,&rio) ; 
    rio_readinitb(&proxy_rio,proxy_fd) ; 
    while ( (num = rio_readlineb(&proxy_rio,proxy_buf,MAXLINE)) != 0  ){
        if(strstr(proxy_buf,"Content-length")!=NULL)
            sscanf(proxy_buf,"Content-length: %d\r\n",&filelen);
        memcpy(all_buf+all_num,proxy_buf, num * sizeof(char));
        Rio_writen(fd,proxy_buf,num) ; 
        all_num += num ; 
    } 
    
    if ( all_num <= MAX_OBJECT_SIZE ) 
        save_to_cache(buf, all_buf, all_num) ;    
    Close(proxy_fd) ; 
}

void clienterror( int fd, char  *cause , char *errnum , char *shortmsg , char *longmsg ) {
    char buf[MAXLINE] , body[MAXLINE] ;

    // build the http response ' s body
    sprintf(body,"<html><title>Proxy Error</title>") ;
    sprintf(body,"%s<body bgcolor=""ffffff"">/r/n",body) ;
    sprintf(body,"%s%s: %s\r\n",body,errnum,shortmsg) ;
    sprintf(body,"%s<p>%s: %s\r\n",body,longmsg,cause) ;
    sprintf(body,"%s<hr><em>The Tiny Web server</em>\r\n",body) ;

    // print the http response
    sprintf(buf,"HTTP/1.0 %s %s\r\n",errnum,shortmsg) ;
    Rio_writen(fd,buf,strlen(buf)) ;
    sprintf(buf,"Content-type: text/html\r\n") ;
    Rio_writen(fd,buf,strlen(buf)) ;
    sprintf(buf,"Content-length: %d\r\n\r\n",(int)strlen(body)) ;
    Rio_writen(fd,buf,strlen(buf)) ;
    Rio_writen(fd,body,strlen(body))  ;
}

void parse_url(char *url , char *hostname, char *filename, char *port) {
    char uri[MAXLINE] ;
    strcat(uri,url) ;
    char *newurl = strstr(uri,"//") ;   // http:// 和 https:// 
    if ( newurl ) 
        newurl += 2 ; 
    else 
        newurl = uri ; 
    *hostname = *filename = *port = '\0' ; 
    char *t1 = strstr(newurl,":") ;      // :<port> 
    int tmp ; 
    if ( t1 ) {
        t1[0] = '\0' ; 
        sscanf(newurl,"%s",hostname) ; 
        sscanf(t1+1,"%d%s",&tmp,filename) ; 
        sprintf(port,"%d",tmp) ; 
    }
    else {
        if  ( strstr(url,"https") != NULL )
            strcat(port,"443") ; 
        else 
            strcat(port,"80") ; 
        char *t2 = strstr(newurl,"/") ; 
        if ( t2 ){
            *t2 = '\0' ; 
            sscanf(newurl,"%s",hostname) ;
            *t2 = '/' ; 
            sscanf(t2,"%s",filename) ; 
        }
        else {
            sscanf(newurl,"%s",hostname) ;
        }
    }
    if ( strlen(filename) <= 1 ) 
        strcat(filename,"/index.html") ;      
  
}

// connect to the finally server by  proxy server
void connect_server(int proxy_fd, char *hostname, char *port, char *filename , rio_t * rio){
    static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n" ;
    static const char *connection = "Connection: close\r\n" ; 
    static const char *proxy_connection = "Proxy-Connection: close\r\n" ; 
    static const char *requestlint_hdr_format = "GET %s HTTP/1.0\r\n" ;
    static const char *endof_hdr = "\r\n" ;
    static const char *host_format = "Host: %s\r\n" ; 
    char host_headers[MAXLINE] ; 
    char request_hdr[MAXLINE] ; 
    char client_buf[MAXBUF], other_hd[MAXLINE] ; 
    while ( rio_readlineb(rio,client_buf,MAXLINE-1) > 0 ){
        if ( strcmp(client_buf,endof_hdr) == 0 )
            break ; 
        if ( strstr(client_buf,"Host") != NULL ) 
            strcat(host_headers,client_buf) ; 
        else if ( !strstr(client_buf,"User-Agent") && !strstr(client_buf,"Connection") && !strstr(client_buf,"Proxy-Connection")){
            strcat(other_hd,client_buf) ; 
        }
    }
    if ( strlen(host_headers) == 0 ) 
        sprintf(host_headers,host_format,hostname);
    if ( proxy_fd < 0 ) {
        Close(proxy_fd) ;  
        unix_error("Can not connect to the final server!\n") ; 
    }
    char buf[MAXBUF] ; 
    sprintf(request_hdr,requestlint_hdr_format,filename);
    sprintf(buf,"%s%s%s%s%s%s%s",request_hdr,host_headers,connection,user_agent_hdr,proxy_connection,other_hd,endof_hdr) ; 
    Rio_writen(proxy_fd,buf,strlen(buf)) ;
}


void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n,sizeof(int)) ; 
    sp->n = n ; 
    sp->front = sp->rear = 0 ; 
    Sem_init(&sp->mutex,0,1) ; 
    Sem_init(&sp->slots,0,n) ; 
    Sem_init(&sp->items,0,0) ; 
}

void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf) ; 
}

void sbuf_insert(sbuf_t *sp, int item){
    P(&sp->slots) ; 
    P(&sp->mutex) ; 
    sp->buf[(++sp->rear)%(sp->n)] = item ;
    V(&sp->mutex) ; 
    V(&sp->items) ; 
}

int sbuf_remove(sbuf_t *sp){
    P(&sp->items) ; 
    P(&sp->mutex) ; 
    int item = sp->buf[(++sp->front)%(sp->n)] ; 
    V(&sp->mutex) ; 
    V(&sp->slots) ; 
    return item ;  
}


int find_and_insert_cache(char *request, char *response, size_t * size){
    pthread_mutex_lock(&mutex_lock) ;
    cache_block* tmp = head ; 
    while ( tmp != NULL ){
        if  ( strcmp(request,tmp->request) == 0 )
            break ; 
        tmp = tmp->next ; 
    }
    if ( tmp == NULL ){
        pthread_mutex_unlock(&mutex_lock) ; 
        return 0 ; 
    } 
    cache_size -= tmp->size ; 
    getout_block(tmp) ; 
    strcat(response,tmp->content) ; 
    *size = tmp->size ; 
    add_block(tmp) ; 
    pthread_mutex_unlock(&mutex_lock) ;
    return 1 ;
}

void getout_block(cache_block *block){
    cache_block* prev_one = block->prev ; 
    cache_block* next_one = block->next ; 
    if ( prev_one != NULL ) 
        prev_one->next = next_one ; 
    else if ( head == block ) {
        head = next_one ; 
        if ( next_one != NULL )
            head->prev = NULL ; 
    } 

    if ( next_one != NULL ) 
        next_one->prev = prev_one ;     
    else if ( next_one == block ) {
        tail = prev_one ; 
        if ( prev_one != NULL ) 
            tail->next = NULL ; 
    }
    block->prev = NULL ; 
    block->next = NULL ; 
}

void add_block(cache_block* block){
    if ( !head ) {
        head = block ; 
        tail = block ; 
    } 
    else if ( !head->prev ) {
        head->prev = block ; 
        block->next = head ;
        block->prev = NULL ;
        head = block ;
    } 
     cache_size += block->size ;
    if (cache_size > MAX_CACHE_SIZE) {
       fprintf(stderr, "cache size exceded\n") ;
       abort() ; 
     }
}

void delete_block(cache_block *block){
    getout_block(block) ;
    free(block->content) ; 
    free(block) ;
    cache_size -= block->size ;
}

void save_to_cache(char *request, char *response, size_t size) {
    pthread_mutex_lock(&mutex_lock) ;
    cache_block *block = (cache_block*)Malloc(sizeof(cache_block)) ;
    strcat(block->request,request) ; 
    block->content = response ; 
    block->size = size ; 
    block->next = NULL ; 
    block->prev = NULL ; 
    while( cache_size + block->size > MAX_CACHE_SIZE) 
        delete_block(tail) ;
    add_block(block) ;
    pthread_mutex_unlock(&mutex_lock) ;
}
