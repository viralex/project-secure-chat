#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include "command.h"

class command-manager-exception : public exception
{
    public:
        command-manager-exception(const std::string &message) throw();
        ~command-manager-exception() throw();

        const char *what() const throw();

    private:
        std::string user_meessage;
};

class command-manager
{
    list<command>  commands;

    string get_opcode(string raw)
    string get_params(string raw)

  public:
    command-manager(string id, handler hnd);
    ~command-manager();

    void       add_command(string id, handler hnd);
    bool       execute(string raw_plaintext);

};
#endif