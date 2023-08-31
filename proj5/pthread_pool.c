#include "pthread_pool.h"
#include <stdlib.h>

static void *worker(void *param)
{
    pthread_pool_t *pool = (pthread_pool_t *)param;

    while (1) {
        pthread_mutex_lock(&pool->mutex);

        while (pool->q_len == 0 && pool->running) {
            pthread_cond_wait(&pool->empty, &pool->mutex);
        }

        if (!pool->running && pool->q_len == 0) {
            pthread_mutex_unlock(&pool->mutex);
            break; // 스레드 풀 종료
        }

        task_t task = pool->q[pool->q_front];
        pool->q_front = (pool->q_front + 1) % pool->q_size;
        pool->q_len--;

        pthread_cond_signal(&pool->full);
        pthread_mutex_unlock(&pool->mutex);

        task.function(task.param);
    }

    return NULL;
}


int pthread_pool_init(pthread_pool_t *pool, size_t bee_size, size_t queue_size)
{
    if (bee_size > POOL_MAXBSIZE || queue_size > POOL_MAXQSIZE) {
        return POOL_FAIL;
    }

    pool->running = true;
    pool->q = (task_t *)malloc(sizeof(task_t) * queue_size);
    pool->q_size = queue_size;
    pool->q_front = 0;
    pool->q_len = 0;
    pool->bee = (pthread_t *)malloc(sizeof(pthread_t) * bee_size);
    pool->bee_size = bee_size;
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->full, NULL);
    pthread_cond_init(&pool->empty, NULL);

    for (size_t i = 0; i < bee_size; i++) {
        pthread_create(&pool->bee[i], NULL, worker, (void *)pool);
    }

    return POOL_SUCCESS;
}
int pthread_pool_submit(pthread_pool_t *pool, void (*f)(void *p), void *p, int flag)
{
    pthread_mutex_lock(&pool->mutex);

    while (pool->q_len == pool->q_size) {
        if (flag == POOL_NOWAIT) {
            pthread_mutex_unlock(&pool->mutex);
            return POOL_FULL;
        }
        pthread_cond_wait(&pool->full, &pool->mutex);
    }

    task_t task;
    task.function = f;
    task.param = p;

    pool->q[(pool->q_front + pool->q_len) % pool->q_size] = task;
    pool->q_len++;

    pthread_cond_signal(&pool->empty);
    pthread_mutex_unlock(&pool->mutex);

    return POOL_SUCCESS;
}

int pthread_pool_shutdown(pthread_pool_t *pool, int how)
{
    pthread_mutex_lock(&pool->mutex);

    pool->running = false;

    pthread_cond_broadcast(&pool->empty);
    pthread_mutex_unlock(&pool->mutex);

    for (size_t i = 0; i < pool->bee_size; i++) {
        pthread_join(pool->bee[i], NULL);
    }

    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->full);
    pthread_cond_destroy(&pool->empty);

    free(pool->q);
    free(pool->bee);

    return POOL_SUCCESS;
}
