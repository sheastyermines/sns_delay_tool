/*
* Author: Shea Styer
* Name: sns_delay_forward
*/

#include <time.h>
#include <getopt.h>
#include <sns.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sns/event.h>
#include <ach/experimental.h>

int fd[2]; //File descriptor for the pipeline
struct timespec tim;
static int verbose_flag;

typedef struct Message {
  void* msg_pointer;
  size_t msg_size;
} msg_container;

static enum ach_status
handle_state( void *cx_, void *msg_, size_t msg_size )
{
  msg_container msg;
  msg.msg_size = msg_size;
  fprintf(stderr, "Raw Message Size: %zu\n", msg.msg_size);
  int r; 
  void* msg_mem = malloc(msg_size);
  memcpy(msg_mem, msg_, msg_size);
  msg.msg_pointer = msg_mem;
  fprintf(stderr, "Pointer to message size:  %zu\n", sizeof(msg.msg_pointer));
  do { 
    fprintf(stderr, "WRITING TO PIPE IN OBJECT: %p\n", &msg.msg_pointer);
    r = write(fd[1],&msg,sizeof(msg_container));
    //Check if r is -1 too
    if(r == -1){
      fprintf(stderr, "write: %s\n", strerror(errno));     
      exit(EXIT_FAILURE); 
    } else if(r != sizeof(msg)){
      fprintf(stderr, "write: %i bytes written, was supposed to write %li bytes\n", r, sizeof(void*));
       exit(EXIT_FAILURE); 
    } else {
      //Good write, we can break out
      break;
    }
  } while(1);
  fprintf(stderr, "Clean message written to the pipe\n");
  return ACH_OK;
}
//Advanced Programming Unix Environment Richard Stevens 

void *reader(struct ach_channel *channel) 
{
  /* Setup Event Handler */
  struct sns_evhandler handlers[1];
  handlers[0].channel = channel;
  handlers[0].context = NULL;
  handlers[0].handler = handle_state;
  handlers[0].ach_options = ACH_O_LAST;
  /* Run event loop */
  enum ach_status r =
    sns_evhandle( handlers, 1,
                  NULL, NULL, NULL,
                  sns_sig_term_default,
                  ACH_EV_O_PERIODIC_TIMEOUT );
    SNS_REQUIRE( sns_cx.shutdown || (ACH_OK == r),
                 "Could not handle events: %s, %s\n",
                 ach_result_to_string(r),
                 strerror(errno) );	
return 0;
}

void *writer(struct ach_channel *channel)
{ 
        fprintf(stderr, "Started monitoring for message on pipe\n");
	while(1) {
		/* Read from the pipe if something is there */
                msg_container msg;
                int r;
                fprintf(stderr, "attempting to read from pipe\n");
		r = read(fd[0],&msg,sizeof(msg));
                fprintf(stderr, "read finished with exit code of %i\n",r); 
		//Error handle read
                fprintf(stderr, "Read bytes from pipe: %i\n ", r);
                if(r == -1){
                  fprintf(stderr,"read: %s\n", strerror(errno)); 
                }
                fprintf(stderr, "Pointer read from pipe: %p\n", msg.msg_pointer); 
		char* deref = (char*)msg.msg_pointer;
                //fprintf(stderr, "Dereferenced Pointer: %s, Size: %lu\n", deref, msg.msg_size);

		fprintf(stderr, "Sleeping for %li seconds and %li nanoseconds\n\n", tim.tv_sec, tim.tv_nsec);
                struct timespec t, t2;
		t.tv_sec = 3;
		t.tv_nsec = 0L;
                //int rettime = clock_gettime(CLOCK_MONOTONIC, &t);
		//if(rettime < 0){
                  //fprintf(stderr, "clock_gettime returned value of %i: %s\n",rettime, strerror(errno));
                //}
		//int ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		int ret = nanosleep(&t, &t2);
		fprintf(stderr, "clock_nanosleep return %i\n", ret);
                if(ret != 0){
                  fprintf(stderr, "clock failed");
                }

		r = ach_put(channel,deref, msg.msg_size);
                fprintf(stderr, "ach_put returned code %i\n", r);
                if(r == -1){
                  fprintf(stderr,"ach_put: Failed to put\n");
                } 
	}

}

void * start_reading(void * restrict channel){
 return  reader(channel);
}

void * start_writing(void * restrict channel){
  return writer(channel);
}

int main(int argc, char** argv) 
{
  tim.tv_sec = 0;
  tim.tv_nsec = 0L;
  int c;
  char * outgoing_channel;
  char * receiving_channel;
  while (1)
    {
      static struct option long_options[] =
        {
          /* These options set a flag. */
          {"verbose", no_argument,       &verbose_flag, 1},
          {"brief",   no_argument,       &verbose_flag, 0},
          /* These options don’t set a flag.
             We distinguish them by their indices. */
          {"milliseconds",  required_argument, 0, 'm'},
          {"nanoseconds",    required_argument, 0, 'n'},
          {"out_channel", required_argument, 0, 'o'},
          {"in_channel", required_argument, 0, 'i'},
          {"seconds", required_argument, 0, 's'},
          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "m:n:o:i:s:",
                       long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 0:
          /* If this option set a flag, do nothing else now. */
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;

        case 'm':
          printf ("millisecond delay: `%s'\n", optarg);
          tim.tv_nsec += atol(optarg) * 1000000;
	  
          break;

        case 'n':
          printf ("nanosecond delay: `%s'\n", optarg);
	  tim.tv_nsec += atol(optarg);
          break;

	case 's':
	  tim.tv_sec += atoi(optarg);
	  break;

	case 'o':
	  outgoing_channel = optarg;
	  break;
    
        case 'i':
	  receiving_channel = optarg;
          break;

        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
          abort ();
        }
    }	
	sns_init();
		
	/* Open Channel */ 
	struct ach_channel rec_channel;
	sns_chan_open(&rec_channel, receiving_channel, NULL);
	
	struct ach_channel out_channel;
	sns_chan_open(&out_channel, outgoing_channel, NULL);

	pthread_t tid1, tid2;

	int result;

	result = pipe(fd);

	if(result < 0) {
		perror("pipe ");
		exit(1);
	}
        result = pthread_create(&tid1,NULL,&start_reading,&rec_channel);
        fprintf(stderr, "pthread_create returned exit code of %i\n", result);
        if (result != 0){
           fprintf(stderr, "pthread_create returned value of %i: %s\n",result, strerror(errno));
	}
        
        fprintf(stderr, "\n\nCreating the polling thread\n");
        result = pthread_create(&tid1,NULL,&start_writing,&out_channel);
        if (result != 0){
          fprintf(stderr, "p_thread_create returned value of %i: %s\n", result, strerror(errno));
        }

        fprintf(stderr, "back in main thread");

	pthread_join(tid1,NULL);
	pthread_join(tid2,NULL);
}
//getopt - command line arguments flags

//malloc put memory on the heap

