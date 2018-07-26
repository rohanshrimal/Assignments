
//Header Files
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>
#include<pthread.h>

#define MAX_SIZE_OF_BUFFER 10000
#define ALL_ONE 4294967295
#define UPPER 10000000
#define LOWER 1
#define SESSION_TIME_OUT 1
#define CLEAN_UP_TIME 5

//Structure for storing host details
typedef struct
{
        unsigned int host_no;
        int count;
        int next_index;
        time_t active_time;
}host;


//Structure for storing subnet related information
typedef struct
{
        host** subnet_index_array;
        unsigned int size_of_index_array;
        int bits_of_subnet;
		int* free_index_rear;
		int* free_index_front;
}subnet_details;


//Fixed Size Buffer
host buffer[MAX_SIZE_OF_BUFFER];

//Free Memory Storage
int free_store[MAX_SIZE_OF_BUFFER];

//Array of Mutex Locks
pthread_mutex_t* lock_array;

//Function Prototypes
unsigned int calc_power(int num,int exp);
void display_buffer();
void traverse_subnet_index_array(host** subnet_index_array ,int size, int bits_of_subnet);
void *print_fun(void *vargp);
void decimal_to_ip(unsigned int number,int bits_of_subnet);
void delete_particular_subnet(host** subnet_index_array ,int size, int bits_of_subnet,int* free_index_rear,int* free_index_front,int i);
void *delete_host(void* var_arg);
host* insert_in_buffer(unsigned int ip,int* free_ptr,int* free_index_rear,int* free_index_front,int* insert_index);


//Main Function
int main()
{
	printf("Program Started At: %ld",time(NULL));
   	printf("\nEnter number of bits for subnet.\n");
        int bits_of_subnet;
	scanf("%d",&bits_of_subnet);

        if(bits_of_subnet<0 || bits_of_subnet>32)
        {
        	printf("Invalid Bits For Subnet\n"); 
	 	return 1;
        }

        unsigned int size_of_index_array = calc_power(2,bits_of_subnet);
    	host** subnet_index_array=(host**)malloc(size_of_index_array * sizeof(host*));

        if(subnet_index_array==NULL)
        {
                printf("Memory issues");
                return 1;
        }

	pthread_mutex_t lock[size_of_index_array];
	lock_array=lock;		

	for(int i=0;i<size_of_index_array;i++)
	{
		if (pthread_mutex_init(&lock_array[i], NULL) != 0)
	    	{
	        	printf("mutex init has failed\n");
	        	return 1;
	    	}
	}

        unsigned int ip=0;
        unsigned int mask=ALL_ONE;
        unsigned int subnet_index=0;

    	int free_ptr=0,flag=0,free_index_rear=-1,free_index_front=-1,insert_index=-1;
    	srand(time(0));

        subnet_details subnet;
        subnet.bits_of_subnet=bits_of_subnet;
        subnet.subnet_index_array=subnet_index_array;
	subnet.size_of_index_array=size_of_index_array;
	subnet.free_index_rear=&free_index_rear;
	subnet.free_index_front=&free_index_front;

	//pthread_t thread_id;
  	//pthread_create(&thread_id, NULL, print_fun, (void*)&subnet);

	pthread_t clean_up_thread_id;
	int error=pthread_create(&clean_up_thread_id,NULL,delete_host,(void*)&subnet);

	if(error!=0)
	{
		printf("Thread can't be created.\n");
		return 1;			
	}

        for(int i=0;i<100000;)
        {
             	flag=0;
                insert_index=-1;
                ip=(rand() % (UPPER - LOWER + 1)) + LOWER;
                
                mask=ALL_ONE;
                mask=mask<<(32-bits_of_subnet);

                subnet_index=mask & ip;
                subnet_index=subnet_index>>(32-bits_of_subnet);
                
		if(pthread_mutex_trylock(&lock_array[subnet_index])==0)
                {
			if(subnet_index_array[subnet_index]==NULL)
		     	{
		          	subnet_index_array[subnet_index]=insert_in_buffer(ip,&free_ptr,&free_index_rear,&free_index_front,&insert_index);

		            	if(subnet_index_array[subnet_index]==NULL)
		                {
		                     	printf("IP: %u NOT PROCESSED.\n",ip);
		           	}
		   	}
		        else
		        {
		            	host* node=subnet_index_array[subnet_index];

		              	while(node->next_index != -1)
		           	{
		                     	if(node->host_no==ip)
		                      	{
		                          	node->count = node->count + 1;
		                          	node->active_time=time(NULL);
		                                flag=1;
		                               	break;
					}
					node=&buffer[node->next_index];
		            	}

		             	if(flag==0)
		             	{
		                   	if(node->host_no == ip)
		                    	{
		                            	node->count = node->count + 1;
		                         	node->active_time=time(NULL);
		                    	}
		                   	else
		                    	{
		                            if(insert_in_buffer(ip,&free_ptr,&free_index_rear,&free_index_front,&insert_index)!=NULL)
		                       		node->next_index = insert_index;

		                            else
		                            {
		                                printf("IP: %u NOT PROCESSED.\n",ip);
		                            }
		                    	}
		              	}
		    	  }
					
			pthread_mutex_unlock(&lock_array[subnet_index]);	
		}
		else
		{
			printf("IP: %u Dropped Because of Locks.\n",ip);
		}
        }

    	free(subnet_index_array);

	for(int i=0;i<size_of_index_array;i++)		
	pthread_mutex_destroy(&lock_array[i]);        
		
	printf("Program Terminated At: %ld\n",time(NULL));
        return 0;
}

void *delete_host(void* var_arg)//host** subnet_index_array ,int size, int bits_of_subnet,int* free_index
{
	subnet_details* subnet=(subnet_details*)var_arg;
	host** subnet_index_array=subnet->subnet_index_array;
	int size=subnet->size_of_index_array;
	int bits_of_subnet=subnet->bits_of_subnet;
	int* free_index_rear=subnet->free_index_rear;
	int* free_index_front=subnet->free_index_front;	
	sleep(CLEAN_UP_TIME);
	
	while(1)
    	{
	   	for(int i=0;i<size;i++)
        	{
			if(pthread_mutex_trylock(&lock_array[i])==0)
                	{
				delete_particular_subnet(subnet_index_array ,size,bits_of_subnet,free_index_rear,free_index_front,i);
				pthread_mutex_unlock(&lock_array[i]);
			}
			else
			{
				printf("SUBNET INDEX: %d IS LOCKED.\n",i);
			}
        	}
		sleep(CLEAN_UP_TIME);
	}	
}


void delete_particular_subnet(host** subnet_index_array ,int size, int bits_of_subnet,int* free_index_rear,int* free_index_front,int i)
{
	int count=0,c=0;
    	host* node=NULL;
    	host* previous_node=NULL;

	if(subnet_index_array[i] !=NULL)
	{
        	node=subnet_index_array[i];
                previous_node=NULL;

        	while(node!=NULL && node->next_index!=-1)
               	{
                	if((time(NULL) - node->active_time) >= SESSION_TIME_OUT )
                        {
                         
				printf("c= %d==SUBNET INDEX=%d. SESSION TIME OUT OF IP: %u last active at %ld is deleted at %ld.\n",c++,i,node->host_no,node->active_time,time(NULL));

                       		if(previous_node==NULL)
                               	{
				   	if((*free_index_front == 0 && *free_index_rear == MAX_SIZE_OF_BUFFER-1) || (*free_index_rear == (*free_index_front-1)%(MAX_SIZE_OF_BUFFER-1)))
				        {
				        	printf("Free Store Is Full\n");
						return;
				        }

					else if (*free_index_front == -1) /* Insert First Element */
					{
						*free_index_front = *free_index_rear = 0;			            		
					}
													 
					else if (*free_index_rear == MAX_SIZE_OF_BUFFER-1 && *free_index_front != 0)
					{
						*free_index_rear = 0;									
					}
													 
					else
					{
						*free_index_rear = *free_index_rear + 1;
					}

		                        long int index=((void*)node-(void*)buffer)/sizeof(host);
		                        free_store[*free_index_rear]=index;
		                      	
		                        if(node->next_index != -1)
		                        subnet_index_array[i]=&buffer[node->next_index];

		                        else
		                        subnet_index_array[i]=NULL;

												
					node->host_no=0;
		                        node->count=0;
		                        node->next_index=-1;
		                        node->active_time=0;

		                        node=subnet_index_array[i];
		                        continue;
													
                           	}
                                else
                               	{
                                    	if((*free_index_front == 0 && *free_index_rear == MAX_SIZE_OF_BUFFER-1) || (*free_index_rear == (*free_index_front-1)%(MAX_SIZE_OF_BUFFER-1)))
                                       	{
                                        	printf("Free Store Is Full.\n");
						return;
                                        }
					else if (*free_index_front == -1) 
					{
						*free_index_front = *free_index_rear = 0;			    		
					}
					else if (*free_index_rear == MAX_SIZE_OF_BUFFER-1 && *free_index_front != 0)
					{
						*free_index_rear = 0;
					}
					else
					{
						*free_index_rear = *free_index_rear + 1;
					}
                                                
                                        free_store[*free_index_rear] = previous_node->next_index;
                                        previous_node->next_index = node->next_index;

                                        node->host_no=0;
                                        node->count=0;
                                        node->next_index=-1;
                                        node->active_time=0;


                                        if(previous_node->next_index!=-1)
                                        {
                                           	node=&buffer[node->next_index];
                                        }
                                        else
                                        {
                                            	node=NULL;
                                        }
                                }
      			}
                                else
                                {
                                   	previous_node=node;
                                        node=&buffer[node->next_index];
 				}
              	}

		if(node!=NULL && ((time(NULL) - node->active_time) >= SESSION_TIME_OUT) )
            	{

          		printf("\nc=%dSUBNET INDEX=%d*******SESSION TIME OUT OF IP: %u last active at %ld is deleted at %ld*******\n",c++,i,node->host_no,node->active_time,time(NULL));

                   	if(previous_node==NULL)
                  	{
                         	if((*free_index_front == 0 && *free_index_rear == MAX_SIZE_OF_BUFFER-1) || (*free_index_rear == (*free_index_front-1)%(MAX_SIZE_OF_BUFFER-1)))
                             	{
                               		printf("FREE STORE IS FULL.\n");
					return;
                                }
				else if (*free_index_front == -1) /* Insert First Element */
				{
					*free_index_front = *free_index_rear = 0;			
                                    		
				}
				else if (*free_index_rear == MAX_SIZE_OF_BUFFER-1 && *free_index_front != 0)
				{
					*free_index_rear = 0;
				}
				else
				{
					*free_index_rear = *free_index_rear + 1;
				}
                                        
                                long int index=((void*)node-(void*)buffer)/sizeof(host);
                                free_store[*free_index_rear]=index;
                              	subnet_index_array[i]=NULL;

                              	node->host_no=0;
                              	node->count=0;
                             	node->next_index=-1;
                             	node->active_time=0;
                       	}
                        else
                       	{
                        	if((*free_index_front == 0 && *free_index_rear == MAX_SIZE_OF_BUFFER-1) || (*free_index_rear == (*free_index_front-1)%(MAX_SIZE_OF_BUFFER-1)))
                              	{
                                    	printf("FREE STORE IS FULL.\n");
					return;
                               	}
				else if (*free_index_front == -1) 
				{
					*free_index_front = *free_index_rear = 0;			
                                }
				else if (*free_index_rear == MAX_SIZE_OF_BUFFER-1 && *free_index_front != 0)
				{
					*free_index_rear = 0;
				}
				else
				{
					*free_index_rear = *free_index_rear + 1;
				}
                                        
                                free_store[*free_index_rear] = previous_node->next_index;
                                previous_node->next_index = node->next_index;

                                node->host_no=0;
                              	node->count=0;
                             	node->next_index=-1;
                               	node->active_time=0;
                       	}
               	}
	}
      	else
       	{
		return;
      	}
}


void *print_fun(void *var_arg)
{
        subnet_details* subnet=(subnet_details*)var_arg;
	int count=1;
   	
	while(1)
    	{
		traverse_subnet_index_array(subnet->subnet_index_array ,subnet->size_of_index_array, subnet->bits_of_subnet);
		sleep(2);
    	}
    	
	return NULL;
}

void traverse_subnet_index_array(host** subnet_index_array ,int size, int bits_of_subnet)
{
    	int count=0;
    	host* node=NULL;
        
	for(int i=0;i<size;i++)
        {
        	if(subnet_index_array[i] !=NULL)
                {
			node=subnet_index_array[i];

                        while(node->next_index!=-1)
                        {
                            printf("{host = %u, count = %d, active time = %ld, ",node->host_no,node->count,node->active_time);
                         
			    decimal_to_ip(node->host_no,bits_of_subnet);
                            node=&buffer[node->next_index];
                        }

                        printf("{host = %u, count = %d, active time = %ld, ",node->host_no,node->count,node->active_time);
                        decimal_to_ip(node->host_no,bits_of_subnet);

                }
        }
}

void decimal_to_ip(unsigned int number,int bits_of_subnet)
{
    unsigned int mask=ALL_ONE;

    mask=mask<<24;
    unsigned int first_octet  = number & mask;

    mask=mask>>8;
    unsigned int second_octet = number & mask;

    mask=mask>>8;
    unsigned int third_octet  = number & mask;

    mask=mask>>8;
    unsigned int fourth_octet = number & mask;
	
    first_octet  = first_octet  >> 24;
    second_octet = second_octet >> 16;
    third_octet  = third_octet  >> 8;

    printf("IP = %u.%u.%u.%u }\n",first_octet,second_octet,third_octet,fourth_octet);
}

host* insert_in_buffer(unsigned int ip,int* free_ptr,int* free_index_rear,int* free_index_front,int* insert_index)
{
        if(*free_ptr < MAX_SIZE_OF_BUFFER)
        {
                buffer[*free_ptr].host_no = ip;
                buffer[*free_ptr].count = 1;
                buffer[*free_ptr].next_index = -1;
                buffer[*free_ptr].active_time = time(NULL);
                
		printf("Sequential insertion done. IP: %u at %d index on %ld time\n",ip,*free_ptr,buffer[*free_ptr].active_time);
                
		host* new_host = &buffer[*free_ptr];
                *insert_index = *free_ptr;
                (*free_ptr) = (*free_ptr)+1;

                return new_host;
        }
        else 
        {
		if (*free_index_front == -1)
		{
			printf("Free Store is Empty.");
			return NULL;
		}				
				
		int free_index = free_store[*free_index_front];
				
		if (*free_index_front == *free_index_rear)
		{
			*free_index_front = -1;
			*free_index_rear = -1;
		}
		else if (*free_index_front == MAX_SIZE_OF_BUFFER-1)
		{	
			*free_index_front = 0;
		}
		else
		{	
			*free_index_front = *free_index_front + 1;
		}

                buffer[free_index].host_no=ip;
                buffer[free_index].count=1;
                buffer[free_index].next_index=-1;
                buffer[free_index].active_time=time(NULL);
                
		printf("Insertion from free store. IP: %u\n at %d index on %ld time.\n",ip,free_index,buffer[free_index].active_time);
                
		host* new_host=&buffer[free_index];
                *insert_index=free_index;
                
                return new_host;
        }
}

void display_buffer()
{
	printf("\n------------------------------DISPLAY BUFFER-------------------------------\n");
        for(int i=0;i<MAX_SIZE_OF_BUFFER;i++)
        {
                printf("index=%d Host No.=%d, Count=%d, Next Index=%d\n",i+1,buffer[i].host_no, buffer[i].count, buffer[i].next_index);
        }
        printf("\n---------------------------------------------------------------------------\n");
}


unsigned int calc_power(int num,int exp)
{
        unsigned int ans=1;

        for(int i=0;i<exp;i++)
        {
                ans=num*ans;
        }

        return ans;
}



