#include "mq_message.h"

MessagePtr createMessage(MessageType type, TaskIDPtr sender, MSecond ttl,\
       MessageInfo description, ContentPtr content) {
    /* initialize a message*/
    MessagePtr new_message = (MessagePtr)my_malloc(sizeof(Message));
    new_message->type = type;
    new_message->sender = sender;
    new_message->ttl = ttl;
    new_message->desc = description;
    new_message->tid = -1;
    new_message->content = content;
    new_message->pre = NULL;
    new_message->next = NULL;
    mq_debug("createMessage: type %d, ttl %d, desc %s", type, ttl, description);
    mq_debug("createReceiver: content data %s, length %d", content->data, \
            content->length);
    return new_message;
} 

void insertMessage(MessagePtr msg) {
    mq_debug("insertMessage: desc %s", msg->desc);
    /* insert a message into the head of the message queue*/
    if (g_mq_message_queue != NULL) {
        msg->next = g_mq_message_queue;
        g_mq_message_queue->pre = msg;
    }
    g_mq_message_queue = msg;
}

void destroyMessage(MessagePtr msg, int detach_queue) {
    mq_debug("destroyMessage: desc %s, tid %d, detach_queue %d", msg->desc, \
            msg->tid, detach_queue);
    if (msg == NULL) {
        return;
    }
    /* detach the message from the message queue*/
    if (detach_queue == 1) {
        if (msg->tid >= 0) {
            myClearTimeout(msg->tid);
            mq_debug("destroyMessage: clear timeout");
        }
        if (msg->pre == NULL) {
            g_mq_message_queue = msg->next;
        } else {
            msg->pre->next = msg->next;
        }
        if (msg->next != NULL) {
            msg->next->pre = msg->pre;
        }
        mq_debug("destroyMessage: detached ");
    }
    /* @NOTE content shoulde release by the user*/
    my_free(msg->content);
    my_free(msg);
    msg = NULL;
    mq_debug("destroyMessage: g_mq_message_queue %d", g_mq_message_queue);
}

int compareMessageDescription(MessageInfo msg_desc, MessageInfo other_desc) {
    int i = 0;
    mq_debug("compareMessageDescription: msg_desc %s", msg_desc);
    mq_debug("compareMessageDescription: other_desc %s", other_desc);
    /* message description is a character string, need to compare each 
     * characters
     */
    if (msg_desc != NULL) {
        if (other_desc == NULL) {
            return 0;
        }
        while (msg_desc[i] != 0 && other_desc[i] != 0) {
            if (msg_desc[i] != other_desc[i]) {
                return 0;
            }
            i ++;
        }
        if (msg_desc[i] != msg_desc[i]) {
            return 0;
        }
    }
    return 1;
}

int matchMessageReceiver(MessagePtr msg, ReceiverPtr receiver) {
    if (compareMessageDescription(msg->desc, receiver->desc) == 0) {
        mq_debug("matchMessageReceiver: match failed");
        return 0;
    }
    mq_debug("matchMessageReceiver: match success");
    return 1;
}

void messageTimeoutCallback(long arg0, long arg1, long arg2, long arg3, \
        long arg4, long arg5, long arg6, long arg7, long arg8, long arg9) {
    MessagePtr msg = (MessagePtr)arg0;
    mq_debug("messageTimeoutCallback: desc %s, tid %d", msg->desc, msg->tid);
    msg->tid = -1;
    lockMqGlobalInfo();
    destroyMessage(msg, 1);
    unlockMqGlobalInfo();
}
