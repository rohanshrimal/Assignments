//Header Files
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>

#define ALL_ONE 4294967295
#define FILE_NAME_LENGTH 1000
#define MAX_STACK_SIZE 10

int MAX_SIZE_OF_BUFFER=0;
int UPPER=0;
int LOWER=0;
int SESSION_TIME_OUT=0;
int CLEAN_UP_TIME=0;
int DISPLAY_TIME=0;
int INPUT_GENERATION_TIME=0;
int MAX_INPUTS=0;
int INPUTS_PER_BATCH=0;
long start_time=0;
int thread_exit=1;

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


typedef struct 
{
   	char key[100];
    	char value[100];
}config_parameters;


//Fixed Size Buffer
host* buffer;

//Free Memory Storage
int* free_store;

//Array of Mutex Locks
pthread_mutex_t* lock_array;

//Function Prototypes
unsigned int calc_power(int num,int exp);
void display_buffer();
void traverse_subnet_index_array(FILE* fp,host** subnet_index_array ,int size, int bits_of_subnet,int i);
void *print_fun(void *vargp);
void decimal_to_ip(FILE* fp,unsigned int number,int bits_of_subnet);
void delete_particular_subnet(host** subnet_index_array ,int size, int bits_of_subnet,int* free_index_rear,int* free_index_front,int i);
void *delete_host(void* var_arg);
host* insert_in_buffer(unsigned int ip,int* free_ptr,int* free_index_rear,int* free_index_front,int* insert_index);
unsigned int generate_IP();
unsigned generate_seq_IP(unsigned int i);
int push(char* stack,char c,int* tos);
int read_config();
char pop(char* stack,int* tos);


//Main Function
int main(int argc, char** argv)
{
	if(read_config()==-1)
	{
		printf("Error\n");
	}
	getc(stdin);
	int test=0;

        if(argc>=2)
        {
                if(argv[1][0]=='1')
                test=1;

                else if(argv[1][0]=='2')
                test=2;

                else if(argv[1][0]=='3')
                test=3;

                else if(argv[1][0]=='4')
                test=4;

                else
                {
                        printf("INVALID COMMAND LINE ARGUMENTS\n");
                        return 1;
                }
        }

	start_time=time(NULL);
	printf("Program Started At: %ld\n",time(NULL) - start_time);
   	
	printf("\nEnter number of bits for subnet.\n");
        int bits_of_subnet;
	scanf("%d",&bits_of_subnet);
	
	printf("Enter Buffer Size: ");
	scanf("%d",&MAX_SIZE_OF_BUFFER);
	
	printf("Enter Upper Value: ");
	scanf("%d",&UPPER);

	printf("Enter Lower Value: ");
	scanf("%d",&LOWER);

	printf("Enter Session Time Out of Packets: ");
	scanf("%d",&SESSION_TIME_OUT);

	printf("Enter Clean Up Time: "); 
	scanf("%d",&CLEAN_UP_TIME);

	printf("Enter Display TIme: ");
	scanf("%d",&DISPLAY_TIME);

	printf("Enter Input Generation Time: ");
	scanf("%d",&INPUT_GENERATION_TIME);
	
	printf("Enter Number of Iterations (FOR INFINITE ITERATIONS ENTER -1): ");
	scanf("%d",&MAX_INPUTS);
	
	if(MAX_INPUTS == -1)
	{
		test=0;
		MAX_INPUTS=1;
	}
	else
	{
		test=3;
	}

	printf("Enter Number of Inputs Per Sleep: "); 
	scanf("%d",&INPUTS_PER_BATCH);


        if(bits_of_subnet<0 || bits_of_subnet>32)
        {
        	printf("Invalid Bits For Subnet\n"); 
	 	return 1;
        }
	
	if(MAX_SIZE_OF_BUFFER<=0 || UPPER<=0 || LOWER<=0 || SESSION_TIME_OUT<=0 || CLEAN_UP_TIME<=0 || DISPLAY_TIME<=0 || INPUT_GENERATION_TIME<=0 || MAX_INPUTS<=0 || INPUTS_PER_BATCH<=0 || LOWER>UPPER)
	{
		printf("Invalid Configuration Values\n");
		return 1;
	}
		
        unsigned int size_of_index_array = calc_power(2,bits_of_subnet);
    	host** subnet_index_array=(host**)malloc(size_of_index_array * sizeof(host*));

	host host_buffer[MAX_SIZE_OF_BUFFER];
	buffer=host_buffer;

	int free_buffer_store[MAX_SIZE_OF_BUFFER];
	free_store=free_buffer_store;

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
	        	printf("Mutex init has failed\n");
	        	return 1;
	    	}
	}


        unsigned int ip=0;
        unsigned int mask=ALL_ONE;
        unsigned int subnet_index=0;

    	int free_ptr=0,flag=0,free_index_rear=-1,free_index_front=-1,insert_index=-1,unhandled_ip=0,dropped_ip=0;

    	srand(time(0));

        subnet_details subnet;
        subnet.bits_of_subnet=bits_of_subnet;
        subnet.subnet_index_array=subnet_index_array;
	subnet.size_of_index_array=size_of_index_array;
	subnet.free_index_rear=&free_index_rear;
	subnet.free_index_front=&free_index_front;

	pthread_t print_thread_id;
  	pthread_create(&print_thread_id, NULL, print_fun, (void*)&subnet);

	pthread_t clean_up_thread_id;
	int error=pthread_create(&clean_up_thread_id,NULL,delete_host,(void*)&subnet);

	if(error!=0)
	{
		printf("Thread can't be created.\n");
		return 1;			
	}

        for(unsigned int i=1;;)
        {
             	flag=0;
                insert_index=-1;
                
		switch(test)
		{
			case 0:
				ip=generate_IP();
				break;

			case 1:
				ip=generate_seq_IP(i);
				break;

			case 2:
				ip=generate_seq_IP(i);
				ip=ip + 65536 * ip;
				break;

			case 3:
				ip=generate_IP();
				break;

			case 4:
				ip=generate_IP();
				break;
		
			default:
				printf("INVALID CHOICE\n");
				return 0;
                }

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
					unhandled_ip++;
		                     	printf("IP: %u NOT PROCESSED. UNHANDLED IP COUNT: %d\n",ip,unhandled_ip);
		           	}
		   	}
		        else
		        {
		            	host* node=subnet_index_array[subnet_index];

		              	while(node->next_index != -1)
		           	{
		                     	if(node->host_no==ip)
		                      	{
		                          	printf("COUNT OF IP: %u Increased.\n",ip);
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
						printf("COUNT OF IP: %u Increased.\n",ip);
		                            	node->count = node->count + 1;
		                         	node->active_time=time(NULL);
		                    	}
		                   	else
		                    	{
		                            if(insert_in_buffer(ip,&free_ptr,&free_index_rear,&free_index_front,&insert_index)!=NULL)
		                       		node->next_index = insert_index;

		                            else
		                            {
						unhandled_ip++;
		                                printf("IP: %u NOT PROCESSED. UNHANDLED IP COUNT: %d\n",ip,unhandled_ip);
		                            }
		                    	}
		              	}
		    	  }
					
			pthread_mutex_unlock(&lock_array[subnet_index]);	
		}
		else
		{
			dropped_ip++;
			printf("IP: %u Dropped Because of Locks. Dropped IP Count: %d \n",ip,dropped_ip);
		
		}

		if(i%INPUTS_PER_BATCH==0)
		{
			printf("%u Packets Encountered Till %ld Seconds.\n",i,time(NULL)-start_time);
			sleep(INPUT_GENERATION_TIME);
			
			if(test==0)
			{
				i=0;
			}
		}

		if(i==MAX_INPUTS && test!=0)
		{
			printf("TOTAL %u INPUTS GENERATED IN %ld SECONDS.\n",i,time(NULL)-start_time);
			printf("TOTAL COUNT OF DROPPED IP's : %d\n",dropped_ip);
			printf("TOTAL COUNT OF UNHANDLED IP's : %d\n",unhandled_ip);			
			break;
		}

		
		i++;
		
        }
	
	sleep(CLEAN_UP_TIME);
    	thread_exit=0;
	
	pthread_join(print_thread_id, NULL);
	pthread_join(clean_up_thread_id, NULL);
	free(subnet_index_array);

	for(int i=0;i<size_of_index_array;i++)		
	pthread_mutex_destroy(&lock_array[i]);        
		
	printf("Program Terminated In: %ld Seconds.",time(NULL)-start_time);
        return 0;
}


int read_config()
{
	char stack[MAX_STACK_SIZE];
	config_parameters cp[10]; 
	char str[1000];
	int tos=-1,i=0,ptr=-1,j=0,flag=0,is_new_line=1;

	FILE* fp=fopen("config.json","r");

	if(fp==NULL)
	{
		printf("ERROR IN OPENING FILE\n");
		return -1;
	}

	while(1)
	{
		if(is_new_line==1)
		{
			if(fgets(str,1000,fp)==NULL)
			{
				break;
			}
			
			i=0;
		}

		while(tos==-1 && str[i]!='{' && str[i]!='\0')
		{
			if(str[i]==' ' || str[i]=='\n')
			{
				i++;
			}
			else
			{	
				flag=1;
				break;
			}
		}

		if(tos==-1 && str[i]=='{')
		{
			push(stack,'{',&tos);
			i++;
		}

		while(tos==0 && str[i]!='"' && str[i]!='\0')
		{
			if(str[i]==' ' || str[i]=='\n')
			{
				i++;
			}
			else
			{
				flag=1;
				break;
			} 
		} 
	
		if(tos==0 && str[i]=='"')
		{
			i++;
			j=0;
			
			push(stack,'"',&tos);
			ptr=ptr+1;
			
			while(str[i]!='"' && str[i]!='\0')
			{
				if(str[i]!='\n')
				{
					cp[ptr].key[j++]=str[i++];
				}
				else if(str[i]=='\n')
				{
					flag=1;
					break;
				}		
			}
			
			if(str[i]=='"')
			{
				cp[ptr].key[j]='\0';
				i++;
			}
		}

		while(tos==1 && str[i]!=':' && str[i]!='\0')
		{
			if(str[i]==' ' || str[i]=='\n')
			{
				i++;
			}
			else
			{
				flag=1;
				break;
			}
		}

		if(tos==1 && str[i]==':')
		{
			push(stack,':',&tos);
			i++;
		}

		while(tos==2 && str[i]!='"' && str[i]!='\0')
		{
			if(str[i]==' ' || str[i]=='\n')
			{
				i++;
			}
			else
			{	
				flag=1;
				break;
			}
		}

		if(tos==2 && str[i]=='"')
		{
			i++;
			j=0;
			push(stack,'"',&tos);

			while(str[i]!='"' && str[i]!='\0')
			{
				if(str[i]!='\n')
				{
					cp[ptr].value[j++]=str[i++];
				}
				else if(str[i]=='\n')
				{	
					flag=1;
					break;
				}
			}

			if(str[i]=='"')
			{
				cp[ptr].value[j]='\0';
				i++;
			}
		}

		while(tos==3 && str[i]!=',' && str[i]!='}' && str[i]!='\0')
		{
			if(str[i]==' ' || str[i]=='\n')
			{
				i++;
			}
			else
			{	
				flag=1;
				break;
			}
		}

		if(tos==3 && (str[i]==',' || str[i]=='}'))
		{
			if(pop(stack,&tos)=='"' && pop(stack,&tos)==':' && pop(stack,&tos)=='"')
			{
				if(str[i]=='}')
				{
					pop(stack,&tos);					
				}

				is_new_line=0;
				i++;
			}
			else
			{	
				flag=1;
			}
		}

		if(str[i]=='\0')
		{
			is_new_line=1;
		}

		if(flag==1)
		{
			break;
		}
	}

	if(flag==1)
	return -1;

	else
	{
		for(int i=0;i<10;i++)
		{
			printf("KEY: %s and VALUE: %s\n",cp[i].key,cp[i].value);
		}
		return 0;
	}
}

int push(char* stack,char c,int* tos)
{
	if(*tos == MAX_STACK_SIZE-1)
	{
		printf("Stack Is Full\n");
		return 0;
	}
	else
	{	
		*tos = *tos + 1;
		stack[*tos] = c;
		return 1;
	}
	
}

char pop(char* stack,int* tos)
{
	if(*tos==-1)
	{
		printf("Stack Is Empty\n");
		return '\0';
	}
	else
	{
		char c = stack[*tos];
		*tos = *tos -1;
		return c;
	}
}

unsigned int generate_IP()
{
	unsigned int IP=(rand() % (UPPER - LOWER + 1)) + LOWER;
	return IP;
}


unsigned generate_seq_IP(unsigned int i)
{
	if(i>=LOWER && i<=UPPER)
	{
		return i;
	}
	else 
	{
		unsigned ip=(i % (UPPER - LOWER + 1)) + LOWER-1;
		
		if (ip>=LOWER)
		{
			return ip;		
		}	
		else
		{
			return UPPER;
		}
	}
}


																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																				

void *delete_host(void* var_arg)
{
	subnet_details* subnet=(subnet_details*)var_arg;
	host** subnet_index_array=subnet->subnet_index_array;
	int size=subnet->size_of_index_array;
	int bits_of_subnet=subnet->bits_of_subnet;
	int* free_index_rear=subnet->free_index_rear;
	int* free_index_front=subnet->free_index_front;	
	int count_clean_up=1;
	sleep(CLEAN_UP_TIME);	
	
	while(thread_exit)
    	{
		printf("Clean Up Thread Executing %d Times At %ld seconds.\n",count_clean_up++,time(NULL)-start_time);
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
                         
				printf(".c= %d==SUBNET INDEX=%d. SESSION TIME OUT OF IP: %u last active at %ld seconds is deleted at %ld seconds.\n",c++,i,node->host_no,node->active_time-start_time,time(NULL)-start_time);

                       		if(previous_node==NULL)
                               	{
				   	if((*free_index_front == 0 && *free_index_rear == MAX_SIZE_OF_BUFFER-1) || (*free_index_rear == (*free_index_front - 1)%(MAX_SIZE_OF_BUFFER-1)))
				        {
				        	printf("Free Store Is Full At %ld seconds\n",time(NULL)-start_time);
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
                                    	if((*free_index_front == 0 && *free_index_rear == MAX_SIZE_OF_BUFFER-1) || (*free_index_rear == (*free_index_front - 1)%(MAX_SIZE_OF_BUFFER-1)))
                                       	{
                                        	printf("Free Store Is Full at %ld seconds.\n",time(NULL)-start_time);
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
                                           	node=&buffer[previous_node->next_index];
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

          		printf("c=%d==SUBNET INDEX=%d. SESSION TIME OUT OF IP: %u last active at %ld seconds is deleted at %ld seconds\n",c++,i,node->host_no,node->active_time-start_time,time(NULL)-start_time);

                   	if(previous_node==NULL)
                  	{
                         	if((*free_index_front == 0 && *free_index_rear == MAX_SIZE_OF_BUFFER-1) || (*free_index_rear == (*free_index_front-1)%(MAX_SIZE_OF_BUFFER-1)))
                             	{
                               		printf("FREE STORE IS FULL AT %ld SECONDS.\n",time(NULL)-start_time);
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
                                    	printf("FREE STORE IS FULL AT %ld SECONDS.\n",time(NULL)-start_time);
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
	FILE* fp=NULL;
	
	while(thread_exit)
    	{	
		char* file_start_string="IP_DUMP_AT_";
		char* file_format=".csv";
		char file_name[FILE_NAME_LENGTH] = {0};
		
		snprintf(file_name, sizeof(file_name), "%s%ld%s",file_start_string,time(NULL)-start_time,file_format);
		fp=fopen(file_name,"a");
		fprintf(fp,"HOST_NO,COUNT,LAST_ACTIVE_TIME,IP ADDRESS\n");

		for(int i=0; i < subnet->size_of_index_array; i++)
		{		
			pthread_mutex_lock(&lock_array[i]);
			traverse_subnet_index_array(fp,subnet->subnet_index_array ,subnet->size_of_index_array, subnet->bits_of_subnet,i);
			pthread_mutex_unlock(&lock_array[i]);
		}

		fclose(fp);
		sleep(DISPLAY_TIME);
    	}
    	
	return NULL;
}

void traverse_subnet_index_array(FILE* fp, host** subnet_index_array ,int size, int bits_of_subnet,int i)
{
    	int count=0;
    	host* node=NULL;
        

        if(subnet_index_array[i] !=NULL)
        {
		node=subnet_index_array[i];

              	while(node->next_index!=-1)
               	{
                  	fprintf(fp,"%u,%d,%ld,",node->host_no,node->count,node->active_time);
                      	decimal_to_ip(fp,node->host_no,bits_of_subnet);
                       	node=&buffer[node->next_index];
            	}

            	fprintf(fp,"%u,%d,%ld,",node->host_no,node->count,node->active_time);
           	decimal_to_ip(fp,node->host_no,bits_of_subnet);

       	}
}


void decimal_to_ip(FILE* fp,unsigned int number,int bits_of_subnet)
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

   fprintf(fp,"%u.%u.%u.%u\n",first_octet,second_octet,third_octet,fourth_octet);
}

host* insert_in_buffer(unsigned int ip,int* free_ptr,int* free_index_rear,int* free_index_front,int* insert_index)
{
        if(*free_ptr < MAX_SIZE_OF_BUFFER)
        {
                buffer[*free_ptr].host_no = ip;
                buffer[*free_ptr].count = 1;
                buffer[*free_ptr].next_index = -1;
                buffer[*free_ptr].active_time = time(NULL);
                
		printf("Sequential insertion done. IP: %u at %d index on %ld seconds.\n",ip,*free_ptr,buffer[*free_ptr].active_time-start_time);
                
		host* new_host = &buffer[*free_ptr];
                *insert_index = *free_ptr;
                (*free_ptr) = (*free_ptr)+1;

                return new_host;
        }
        else 
        {
		if (*free_index_front == -1)
		{
			printf("Free Store is Empty At %ld Seconds.",time(NULL)-start_time);
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
                
		printf("Insertion from free store. IP: %u at %d index on %ld seconds.\n",ip,free_index,buffer[free_index].active_time-start_time);
                
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

