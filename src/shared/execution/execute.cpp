#include "execute.h"

void *exec_thread(void *params)
{
    execute_thread_params* t_param = (execute_thread_params*) arg;
    
    if(!t_param)
        pthread_exit(NULL);

    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    
    sessions *sess = t_param->sess;
    execute  *exec = t_param->exec;
    session  *temp_session = NULL;
    string   message;

    while(1) // while condizione morte thread ... per ora non muore mai
    {
        temp_session = sess->get_exec_pointer();
        message = temp_session->get_message();
        exec->exec(message);
        sess->release_exec_pointer();
    }
    
    if (t_param)
        delete t_param;

    pthread_exit(NULL);

}

execute::execute()
{

}

execute::~execute()
{
    //for (int i = 0; i < tids.size(); i++)
    //    pthread_join(tids[i], &status);
}

execute::start_threads(int n)
{
    for (int i = 0 ; i<n ; i++)
        start_thread();
}

execute::start_thread()
{
    pthread_t      tid;
    pthread_attr_t tattr;

    int ret;

    ret = pthread_attr_init(&tattr);
    ret = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED); //PTHREAD_CREATE_JOINABLE);

    execute_thread_params* t_params = new execute_thread_params;
    t_params->exec = this;
    t_params->sess = session_pointer;

    if (ret = pthread_create(&tid, &tattr, exec_thread, (void*)t_params))
    {
        perror("pthread_create");
        delete t_params;
    }

    tids.push_back(tid);
    pthread_attr_destroy(&tattr);

    INFO("verbose", "* thread %d started \n", tid);
}