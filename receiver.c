#include "receiver.h"

void init_receiver(Receiver * receiver,
                   int id)
{
    receiver->recv_id = id;
    receiver->input_framelist_head = NULL;
    receiver->NFE = 0;
    receiver->recvQSize = 0;
    int i = 0;
    for (; i < RWS; i++) {
        receiver->recvQ[i].inuse = 0;
        receiver->recvQ[i].seqNum = 0;
    }
    pthread_cond_init(&receiver->buffer_cv, NULL);
    pthread_mutex_init(&receiver->buffer_mutex, NULL);
}


void handle_incoming_msgs(Receiver * receiver,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming frames
    //    1) Dequeue the Frame from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this receiver
    //    5) Do sliding window protocol for sender/receiver pair
    int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
    while (incoming_msgs_length > 0)
    {
        //Pop a node off the front of the link list and update the count
        LLnode * ll_inmsg_node = ll_pop_node(&receiver->input_framelist_head);
        incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        //DUMMY CODE: Print the raw_char_buf
        //NOTE: You should not blindly print messages!
        //      Ask yourself: Is this message really for me?
        //                    Is this message corrupted?
        //                    Is this an old, retransmitted message?           
        char * raw_char_buf = (char *) ll_inmsg_node->value;
        int flag = is_corrupted(raw_char_buf, MAX_FRAME_SIZE);
        //fprintf(stderr, "RFlag = %d\n", flag);
        if (flag == 0) {
//            fprintf(stderr, "CORRUPTED\n");
            break;
        }
        Frame * inframe = convert_char_to_frame(raw_char_buf);
        //fprintf(stderr, "CRC8 = %02x\n", inframe->CRC8);
        //Free raw_char_buf
        if (receiver->recv_id == inframe->dst_id) {
            int NFE = receiver->NFE;
            int seqNum = inframe->seqNum;
           //fprintf(stderr, "receiving NFE = %d, SEQNUM = %d\n", NFE, seqNum);
            unsigned char upper = NFE + RWS - 1;
            if (seqNum == NFE) {
                printf("<RECV_%d>:[%s]\n", receiver->recv_id, inframe->data);
                append_crc(raw_char_buf, MAX_FRAME_SIZE);
               //fprintf(stderr, "SENDING Ack FOR %d\n", inframe->seqNum);
                ll_append_node(outgoing_frames_head_ptr,raw_char_buf);
                receiver->NFE++;
                NFE++;
                char flag = 1;
                if (flag == 1) {
                    int i = 0;
                    flag = 0;
                    for (;i < RWS; i++) {
                        if (receiver->recvQ[i].inuse == 1 &&
                            receiver->recvQ[i].seqNum == NFE) {
                            flag = 1;
                            receiver->recvQ[i].inuse = 0;
                            Frame *tempFrame = (Frame *) receiver->recvQ[i].msg;
                            printf("<RECV_%d>:[%s]\n", receiver->recv_id,
                                    tempFrame->data);
                  //     fprintf(stderr, "Sending ACK for %d\n", tempFrame->seqNum);
                            char * tempChar = convert_frame_to_char(tempFrame);
                            append_crc(tempChar, MAX_FRAME_SIZE);
                            ll_append_node(outgoing_frames_head_ptr,tempChar);
                            receiver->NFE++;
                            NFE++;
                            receiver->recvQSize--;
                        }
                    }
                }

            }
            else if((upper >= NFE && seqNum >=NFE && seqNum <= upper) ||
                (upper < NFE && (seqNum >= NFE || seqNum <= upper))) {
                char flag = 0;
                int i = 0;
                for (; i < RWS; i++) {
                    if (receiver->recvQ[i].inuse == 1) {
                        if (seqNum == receiver->recvQ[i].seqNum) {
                            //fprintf(stderr, "Already Exists in %d\n", i);
                            flag = 1;
                            break;
                        }
                    }
                }
                i = 0;
                for (; i < RWS && flag == 0; i++) {
                    if(receiver->recvQ[i].inuse == 0) {
                        //fprintf(stderr,"Adding to recvQ in %d\n", i);
                        receiver->recvQSize++;
                        receiver->recvQ[i].inuse = 1;
                        receiver->recvQ[i].seqNum = seqNum;
                        receiver->recvQ[i].msg = (Frame*)malloc(MAX_FRAME_SIZE);
                        memcpy(receiver->recvQ[i].msg,inframe,MAX_FRAME_SIZE);
                        break;
                    }
                }
            }
            else {
        //        fprintf(stderr, "%d is out of bounds with data %s\n",
          //              inframe->seqNum, inframe->data);
              // fprintf(stderr, "Sending ack for %d\n", receiver->NFE);
                inframe->seqNum = receiver->NFE;
                char * buffer = convert_frame_to_char(inframe);
                append_crc(buffer, MAX_FRAME_SIZE);
                ll_append_node(outgoing_frames_head_ptr, buffer);
            }
            //fprintf(stderr, "Using %d in sendQ\n", receiver->recvQSize);
        }
        free(inframe);
        free(ll_inmsg_node);
    }
}

void * run_receiver(void * input_receiver)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Receiver * receiver = (Receiver *) input_receiver;
    LLnode * outgoing_frames_head;


    //This incomplete receiver thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up if there is nothing in the incoming queue(s)
    //2. Grab the mutex protecting the input_msg queue
    //3. Dequeues messages from the input_msg queue and prints them
    //4. Releases the lock
    //5. Sends out any outgoing messages

    
    while(1)
    {    
        //NOTE: Add outgoing messages to the outgoing_frames_head pointer
        outgoing_frames_head = NULL;
        gettimeofday(&curr_timeval, 
                     NULL);

        //Either timeout or get woken up because you've received a datagram
        //NOTE: You don't really need to do anything here, but it might be useful for debugging purposes to have the receivers periodically wakeup and print info
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;
        time_spec.tv_sec += WAIT_SEC_TIME;
        time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&receiver->buffer_mutex);

        //Check whether anything arrived
        int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        if (incoming_msgs_length == 0)
        {
            //Nothing has arrived, do a timed wait on the condition variable (which releases the mutex). Again, you don't really need to do the timed wait.
            //A signal on the condition variable will wake up the thread and reacquire the lock
            pthread_cond_timedwait(&receiver->buffer_cv, 
                                   &receiver->buffer_mutex,
                                   &time_spec);
        }

        handle_incoming_msgs(receiver,
                             &outgoing_frames_head);

        pthread_mutex_unlock(&receiver->buffer_mutex);
        
        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames user has appended to the outgoing_frames list
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *) ll_outframe_node->value;
            //The following function frees the memory for the char_buf object
            send_msg_to_senders(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);

}
