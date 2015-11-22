#include "sender.h"

void init_sender(Sender * sender, int id)
{
    //TODO: You should fill in this function as necessary
    sender->send_id = id;
    sender->input_cmdlist_head = NULL;
    sender->input_framelist_head = NULL;
    pthread_cond_init(&sender->buffer_cv, NULL);
    pthread_mutex_init(&sender->buffer_mutex, NULL);
    sender->seqNum = 0;
    sender->LAR = -1;
    sender->sendQSize = 0;
    int i = 0;
    for (; i < SWS; i++) {
        sender->sendQ[i].inuse = 0;
    }
}
void setTimeOutTime(struct timeval *timeout) {
    gettimeofday(timeout, NULL);
    timeout->tv_usec += 100000;
    if (timeout->tv_usec >= 1000000) {
        timeout->tv_usec -= 1000000;
        timeout->tv_sec += 1;
    }
}
struct timeval * sender_get_next_expiring_timeval(Sender * sender)
{
    //TODO: You should fill in this function so that it returns the next timeout that should occur
    return NULL;
}


void handle_incoming_acks(Sender * sender,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming ACKs
    //    1) Dequeue the ACK from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this sender
    //    5) Do sliding window protocol for sender/receiver pair 
    int incoming_ack_length = ll_get_length(sender->input_framelist_head);
    while (incoming_ack_length > 0) {
        LLnode * ll_ack_node = ll_pop_node(&sender->input_framelist_head);
        incoming_ack_length = ll_get_length(sender->input_framelist_head);
        char * raw_char_buf = (char *) ll_ack_node->value;
        int flag = is_corrupted(raw_char_buf, MAX_FRAME_SIZE);
        //fprintf(stderr, "SFLAG = %d\n", flag);
        if (flag == 0) {
           // fprintf(stderr, "CORRUPTED\n");
            break;
        }
        Frame * ackFrame = convert_char_to_frame(raw_char_buf);
        free (raw_char_buf);

        if (sender->send_id == ackFrame->src_id) {
            if (!((sender->LAR <= sender->seqNum &&
                   ackFrame->seqNum > sender->LAR &&
                   ackFrame->seqNum <= sender->seqNum) ||
                  (sender->LAR > sender->seqNum &&
                  (ackFrame->seqNum > sender->LAR ||
                   ackFrame->seqNum <= sender->seqNum))))
                break;
      //      fprintf(stderr, "Receiver[%d] Ack #%d\n",ackFrame->dst_id, 
        //                                           ackFrame->seqNum);
           // fprintf(stderr, "OLD LAR %d and OLD sendQSize %d\n", sender->LAR, sender->sendQSize);
            do {
                int i = 0;
                for (;i < SWS; i++) {
                    if (sender->sendQ[i].inuse == 1) {
                        Frame * qFrame = (Frame *)sender->sendQ[i].msg;
                        if (qFrame->seqNum == sender->LAR){
                            sender->sendQ[i].inuse = 0;
                            sender->sendQSize--;
                   //         fprintf(stderr, "REMOVING FRAME %d\n", qFrame->seqNum);
                        }
                    }
                }
                sender->LAR++;
            } while (sender->LAR != ackFrame->seqNum);
            //fprintf(stderr, "LAR = %d and SENDQSIZE %d\n", sender->LAR, sender->sendQSize);
        } 
        free (ackFrame);
        free (ll_ack_node);
    }
}


void handle_input_cmds(Sender * sender,
                       LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling input cmd
    //    1) Dequeue the Cmd from sender->input_cmdlist_head
    //    2) Convert to Frame
    //    3) Set up the frame according to the sliding window protocol
    //    4) Compute CRC and add CRC to Frame

    int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    
        
    //Recheck the command queue length to see if stdin_thread dumped a command on us
    input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    ll_split_head(&sender->input_cmdlist_head, FRAME_PAYLOAD_SIZE - 1);
    while (input_cmd_length > 0 && sender->sendQSize <= SWS)
    {
    //    fprintf(stderr, "%d %d\n", input_cmd_length, ll_get_length(sender->input_cmdlist_head));
    
        unsigned char upper = sender->LAR + SWS - 1;
        //printf("%d %d %d ", sender->LAR, upper, sender->seqNum);
        if (!((sender->LAR <= upper && 
               sender->seqNum >= sender->LAR &&
               sender->seqNum <= upper) ||
              (sender->LAR > upper &&
              (sender->seqNum >= sender->LAR ||
               sender->seqNum <= upper))))
            break;
        //Pop a node off and update the input_cmd_length
        LLnode * ll_input_cmd_node = ll_pop_node(&sender->input_cmdlist_head);
        input_cmd_length = ll_get_length(sender->input_cmdlist_head);

        //Cast to Cmd type and free up the memory for the node
        Cmd * outgoing_cmd = (Cmd *) ll_input_cmd_node->value;
        free(ll_input_cmd_node);
            

        //DUMMY CODE: Add the raw char buf to the outgoing_frames list
        //NOTE: You should not blindly send this message out!
        //      Ask yourself: Is this message actually going to the right receiver (recall that default behavior of send is to broadcast to all receivers)?
        //                    Does the receiver have enough space in in it's input queue to handle this message?
        //                    Were the previous messages sent to this receiver ACTUALLY delivered to the receiver?
        int msg_length = strlen(outgoing_cmd->message);
        if (msg_length > MAX_FRAME_SIZE)
        {
            //Do something about messages that exceed the frame size
            printf("<SEND_%d>: sending messages of length greater than %d is not implemented\n", sender->send_id, MAX_FRAME_SIZE);
        }
        else
        {
            //This is probably ONLY one step you want
            Frame * outgoing_frame = (Frame *) malloc (sizeof(Frame));
            strcpy(outgoing_frame->data, outgoing_cmd->message);
            outgoing_frame->src_id = outgoing_cmd->src_id;
            outgoing_frame->dst_id = outgoing_cmd->dst_id;
            outgoing_frame->seqNum = sender->seqNum;
            sender->seqNum++;
            //if (sender->seqNum >= SWS)
            //    sender->seqNum = 0;
            //if (sender->seqNum == 5)
            //    sender->seqNum++;

            //At this point, we don't need the outgoing_cmd
            free(outgoing_cmd->message);
            free(outgoing_cmd);

            //Convert the message to the outgoing_charbuf
            char * outgoing_charbuf = convert_frame_to_char(outgoing_frame);
            append_crc(outgoing_charbuf, MAX_FRAME_SIZE);
            //fprintf(stderr, "Sending %d with data %s\n", outgoing_frame->seqNum,outgoing_frame->data);
            ll_append_node(outgoing_frames_head_ptr,
                           outgoing_charbuf);
            int i = 0;
            for (; i < SWS; i++) {
                if (sender->sendQ[i].inuse == 0) {
                    sender->sendQSize++;
                    struct timeval * timeout = malloc(sizeof(struct timeval));
                    setTimeOutTime(timeout);
                    sender->sendQ[i].inuse = 1;
                    sender->sendQ[i].timeout = timeout;
                    sender->sendQ[i].msg =(Frame *)malloc(MAX_FRAME_SIZE);
                    memcpy(sender->sendQ[i].msg,outgoing_frame,MAX_FRAME_SIZE);
                    break;
                }
            }
            free(outgoing_frame);
        }
    }   
}


void handle_timedout_frames(Sender * sender,
                            LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling timed out datagrams
    //    1) Iterate through the sliding window protocol information you maintain for each receiver
    //    2) Locate frames that are timed out and add them to the outgoing frames
    //    3) Update the next timeout field on the outgoing frames
    int i = 0;
    struct timeval *time = malloc(sizeof(struct timeval));
    gettimeofday(time, NULL);
    for (;i < SWS; i++) {
        if (sender->sendQ[i].inuse == 1) {
            long timediff = timeval_usecdiff(time, sender->sendQ[i].timeout);
            if (timediff < 0) {
                Frame * outgoing_frame = (Frame *) sender->sendQ[i].msg;
                char * outgoing_msg = convert_frame_to_char(outgoing_frame);
                //fprintf(stderr, "%d frame timing out\n", outgoing_frame->seqNum);
                append_crc(outgoing_msg,MAX_FRAME_SIZE);
                ll_append_node(outgoing_frames_head_ptr, outgoing_msg);
                setTimeOutTime(sender->sendQ[i].timeout);
                //free(outgoing_frame);
            }
        }
    }
        
}


void * run_sender(void * input_sender)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Sender * sender = (Sender *) input_sender;    
    LLnode * outgoing_frames_head;
    struct timeval * expiring_timeval;
    long sleep_usec_time, sleep_sec_time;
    
    //This incomplete sender thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up
    //2. Grab the mutex protecting the input_cmd/inframe queues
    //3. Dequeues messages from the input queue and adds them to the outgoing_frames list
    //4. Releases the lock
    //5. Sends out the messages


    while(1)
    {    
        outgoing_frames_head = NULL;

        //Get the current time
        gettimeofday(&curr_timeval, 
                     NULL);

        //time_spec is a data structure used to specify when the thread should wake up
        //The time is specified as an ABSOLUTE (meaning, conceptually, you specify 9/23/2010 @ 1pm, wakeup)
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;

        //Check for the next event we should handle
        expiring_timeval = sender_get_next_expiring_timeval(sender);

        //Perform full on timeout
        if (expiring_timeval == NULL)
        {
            time_spec.tv_sec += WAIT_SEC_TIME;
            time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        }
        else
        {
            //Take the difference between the next event and the current time
            sleep_usec_time = timeval_usecdiff(&curr_timeval,
                                               expiring_timeval);

            //Sleep if the difference is positive
            if (sleep_usec_time > 0)
            {
                sleep_sec_time = sleep_usec_time/1000000;
                sleep_usec_time = sleep_usec_time % 1000000;   
                time_spec.tv_sec += sleep_sec_time;
                time_spec.tv_nsec += sleep_usec_time*1000;
            }   
        }

        //Check to make sure we didn't "overflow" the nanosecond field
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        
        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames or input commands should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&sender->buffer_mutex);

        //Check whether anything has arrived
        int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
        int inframe_queue_length = ll_get_length(sender->input_framelist_head);
        
        //Nothing (cmd nor incoming frame) has arrived, so do a timed wait on the sender's condition variable (releases lock)
        //A signal on the condition variable will wakeup the thread and reaquire the lock
        if (input_cmd_length == 0 &&
            inframe_queue_length == 0)
        {
            
            pthread_cond_timedwait(&sender->buffer_cv, 
                                   &sender->buffer_mutex,
                                   &time_spec);
        }
        //Implement this
        handle_incoming_acks(sender,
                             &outgoing_frames_head);

        //Implement this
        handle_input_cmds(sender,
                          &outgoing_frames_head);

        pthread_mutex_unlock(&sender->buffer_mutex);


        //Implement this
        handle_timedout_frames(sender,
                               &outgoing_frames_head);

        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *)  ll_outframe_node->value;

            //Don't worry about freeing the char_buf, the following function does that
            send_msg_to_receivers(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);
    return 0;
}
