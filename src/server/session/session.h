#include "../../shared/session/user-session.h"

class Session
{
    public:
        Session(UserSession* pUser)
        {
            pthread_mutex_init(&mutex_session, NULL);
            pthread_mutex_init(&mutex_m_active, NULL);
            pthread_mutex_init(&mutex_net, NULL);
            pthread_mutex_init(&mutex_exec, NULL);
            m_pUser = pUser;
            m_pUser->SetSession(this);
            m_active = 1;
        }

        ~Session()
        {
            pthread_mutex_destroy(&mutex_session);
            pthread_mutex_destroy(&mutex_m_active);
            pthread_mutex_destroy(&mutex_net);
            pthread_mutex_destroy(&mutex_exec);
        }

        // TODO Inserire tempo di creazione della session per controllo di pacchetti precedenti
        void SetSession(UserSession* pUser, uint32 id) { m_pUser = pUser; m_active = 1; m_pUser->SetId(id); m_pUser->SetSession(this);}
        UserSession* GetUserSession() { return m_pUser; }

        bool IsActive() { return m_active == 1 ? true : false; }
        bool IsFree() { return m_active == 0 ? true : false; }
        bool IsToDelete() { return m_active == -1 ? true : false; }
        void Free() 
        {
            delete m_pUser;
            m_pUser = NULL;
            m_active = 0;
            releaselock_exec();
            releaselock_net();
        }
        void ToDelete()
        {
            m_active = -1;
        }
        
        void  getlock_session() { pthread_mutex_lock(&mutex_session); }
        void  releaselock_session() { pthread_mutex_unlock(&mutex_session); }
        void  getlock_active() { pthread_mutex_lock(&mutex_m_active); }
        void  releaselock_actives() { pthread_mutex_unlock(&mutex_m_active); }

        void  releaselock_net() { pthread_mutex_unlock(&mutex_net); }
        // Non bloccante
        bool getlock_net()
        {
            if (!IsActive())
                return  false;
            if (pthread_mutex_trylock (&mutex_net) != 0)
                return  false;
            else
                return true;
        }        

        void  releaselock_exec() { pthread_mutex_unlock(&mutex_exec); }
        // Non bloccante
        bool getlock_exec()
        {
            if (IsToDelete())                                           // E' da cancellare
                if (pthread_mutex_trylock (&mutex_exec) == 0)           // Cerco di ottenere il mutex di exec
                    if (pthread_mutex_trylock (&mutex_net) == 0)        // Cerco di ottenere il mutex di net
                    {
                        Free();                                         // Cancello
                        return false;
                    }
                    else
                    {
                        releaselock_exec();                             // Non ho ottenuto il mutex di net, rilascio quello di exec
                        return false;
                    }
                        
           // if (IsFree() || (IsActive() && m_pUser->RecvSize() == 0))     // Non e' valida se e' una sessione libera o se la coda di pacchetti 
           //      return  false;                                         // da servire e' vuota
            if (pthread_mutex_trylock (&mutex_exec) != 0)               // Provo a prendere il mutex di exec
                return  false;
            else
                return true;
        }
        

    private:
        pthread_mutex_t    mutex_session;
        pthread_mutex_t    mutex_m_active;

        pthread_mutex_t    mutex_net;
        pthread_mutex_t    mutex_exec;

        int m_active;
        UserSession* m_pUser;
};
