/**
 * @file   aq.c
 * @Author 02335 team
 * @date   October, 2024
 * @brief  Alarm queue thread-safe (Task 2)
 */

#include "aq.h"
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

struct MsgNode {
  void *payload;
  struct MsgNode *next;
};

struct AlarmQueueStruct {
  struct MsgNode *head;
  struct MsgNode *tail;
  bool hasAlarm;
  void *alarm_payload;
  int size;

  // Task 2 additions
  pthread_mutex_t mtx;
  pthread_cond_t  nonempty;    // signaled when size becomes > 0
  pthread_cond_t  alarm_free;  // signaled when alarm slot becomes free
};

AlarmQueue aq_create(void) {
  struct AlarmQueueStruct *q = (struct AlarmQueueStruct *)malloc(sizeof(*q));
  if (!q) return NULL;
  q->head = q->tail = NULL;
  q->hasAlarm = false;
  q->alarm_payload = NULL;
  q->size = 0;
  pthread_mutex_init(&q->mtx, NULL);
  pthread_cond_init(&q->nonempty, NULL);
  pthread_cond_init(&q->alarm_free, NULL);
  return (AlarmQueue)q;
}

int aq_send(AlarmQueue aq, void *msg, MsgKind k) {
  if (!aq)  return AQ_UNINIT;
  if (!msg) return AQ_NULL_MSG;

  struct AlarmQueueStruct *queue = (struct AlarmQueueStruct *)aq;

  pthread_mutex_lock(&queue->mtx);

  if (k == AQ_ALARM) {
    // Block while an alarm is already present
    while (queue->hasAlarm) {
      pthread_cond_wait(&queue->alarm_free, &queue->mtx);
    }
    queue->alarm_payload = msg;
    queue->hasAlarm = true;
    queue->size++;
    // Wake a receiver if it was waiting for any message
    pthread_cond_signal(&queue->nonempty);
    pthread_mutex_unlock(&queue->mtx);
    return 0;
  } else {
    struct MsgNode *newNode = (struct MsgNode*)malloc(sizeof(struct MsgNode));
    if (!newNode) {
      pthread_mutex_unlock(&queue->mtx);
      return AQ_NO_ROOM;
    }
    newNode->payload = msg;
    newNode->next = NULL;

    if (queue->tail != NULL) {
      queue->tail->next = newNode;
    } else {
      queue->head = newNode;
    }
    queue->tail = newNode;
    queue->size++;
    // Wake a receiver if it was waiting for any message
    pthread_cond_signal(&queue->nonempty);

    pthread_mutex_unlock(&queue->mtx);
    return 0;
  }
}

int aq_recv(AlarmQueue aq, void **msg) {
  if (!aq)  return AQ_UNINIT;
  if (!msg) return AQ_NO_MSG;

  struct AlarmQueueStruct *queue = (struct AlarmQueueStruct *)aq;

  pthread_mutex_lock(&queue->mtx);

  // Block while empty
  while (queue->size == 0) {
    pthread_cond_wait(&queue->nonempty, &queue->mtx);
  }

  if (queue->hasAlarm) {
    *msg = queue->alarm_payload;
    queue->alarm_payload = NULL;
    queue->hasAlarm = false;
    queue->size--;
    // An alarm slot just freed up: wake one waiting alarm sender
    pthread_cond_signal(&queue->alarm_free);
    pthread_mutex_unlock(&queue->mtx);
    return AQ_ALARM;
  } else {
    struct MsgNode *node = queue->head;
    queue->head = node->next;
    if (queue->head == NULL) {
      queue->tail = NULL;
    }
    *msg = node->payload;
    free(node);
    queue->size--;
    pthread_mutex_unlock(&queue->mtx);
    return AQ_NORMAL;
  }
}

int aq_size(AlarmQueue aq) {
  if (!aq) return 0;
  struct AlarmQueueStruct *queue = (struct AlarmQueueStruct *)aq;
  pthread_mutex_lock(&queue->mtx);
  int n = queue->size;
  pthread_mutex_unlock(&queue->mtx);
  return n;
}

int aq_alarms(AlarmQueue aq) {
  if (!aq) return 0;
  struct AlarmQueueStruct *queue = (struct AlarmQueueStruct *)aq;
  pthread_mutex_lock(&queue->mtx);
  int n = queue->hasAlarm ? 1 : 0;
  pthread_mutex_unlock(&queue->mtx);
  return n;
}
