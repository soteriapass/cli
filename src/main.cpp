#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <grpc++/grpc++.h>

#include "client.h"
#include "conf.h"

enum CMD_ACTIONS
{
    ACTION_ADDUSER,
    ACTION_MODIFYUSER,
    ACTION_ADDPASSWORD,
    ACTION_AUTHENTICATE,
    ACTION_DELETEPASSWORD,
    ACTION_HELP,
    ACTION_LISTPASSWORDS,
    ACTION_MODIFYPASSWORD,
};

bool add_user(PasswordManagerClient& client, const std::string& new_user)
{
    if(new_user.empty())
    {
        std::cerr << "username not specified" << std::endl;
    }
    else
    {
        std::string new_pass;
        std::string new_pass_confirm;
        while(new_pass.empty() || new_pass != new_pass_confirm)
        {
            std::cout << "password for " << new_user << ":";
            std::cin >> new_pass;

            std::cout << "confirm password for " << new_user << ":";
            std::cin >> new_pass_confirm;
        };

        std::string secret;
        std::vector<int> scratch_codes;
        std::string qrcode;
        if(!client.CreateUser(new_user, new_pass, secret, scratch_codes, qrcode))
        {
            std::cerr << "Could not create user" << std::endl;
            std::cerr << client.GetLastError() << std::endl;
            return false;
        }
        std::cout << "user " << new_user << " created successfully" << std::endl;
        std::cout << "  2fa secret: " << secret << std::endl;
        std::cout << "  Scratch Codes: " << std::endl;
        for(int i = 0; i < scratch_codes.size(); ++i)
        {
            std::cout << "    " << scratch_codes[i] << std::endl;
        }
        std::ofstream qrcode_file("qrcode.png", std::ios::out | std::ios::binary);
        if(qrcode_file.is_open())
        {
            qrcode_file.write(qrcode.c_str(), qrcode.size());
            qrcode_file.close();
        }
    }
    return true;
}

bool modify_user(PasswordManagerClient& client, const std::string& user)
{
    std::string new_pass;
    std::string new_pass_confirm;
    while(new_pass.empty() || new_pass != new_pass_confirm)
    {
        std::cout << "new password for " << user << ":";
        std::cin >> new_pass;

        std::cout << "confirm new password for " << user << ":";
        std::cin >> new_pass_confirm;
    };

    std::string secret;
    std::vector<int> scratch_codes;
    std::string qrcode;
    if(!client.UpdateUserPassword(user, new_pass))
    {
        std::cerr << "Could not modify user" << std::endl;
        std::cerr << client.GetLastError() << std::endl;
        return false;
    }

    return true;
}

bool login(PasswordManagerClient& client, const std::string& user)
{
    std::string pass;
    if(!user.empty())
    {
        std::cout << "password for " << user << ":";
        std::cin >> pass;
    }
    bool need2fa = false;
    std::string token;
    if(!client.Authenticate(user, pass, token, need2fa, false))
    {
        bool authenticated = false;
        if(need2fa)
        {
            do
            {
                std::cout << "Two factor token: ";
                std::string token;
                std::cin >> token;
                authenticated = client.Authenticate(user, pass, token, need2fa, false);
            }
            while(need2fa && !authenticated);
        }

        if(!authenticated)
        {
            std::cerr << client.GetLastError() << std::endl;
            return false;
        }
    }
    std::cout << "Authenticated" << std::endl;
    return true;
}

bool add_password(PasswordManagerClient& client)
{
    std::string account_name;
    std::cout << "account name: ";
    std::cin >> account_name;

    std::string username;
    std::cout << "username: ";
    std::cin >> username;

    std::string password;
    std::cout << "password: ";
    std::cin >> password;

    std::string extra;
    std::cout << "extra: ";
    std::cin >> extra;

    if(!client.AddPassword(account_name, username, password, extra))
    {
        std::cerr << client.GetLastError() << std::endl;
        return false;
    }
    return true;
}

bool list_passwords(PasswordManagerClient& client)
{
    if(!client.ListPasswords())
    {
        return false;
    }
    return true;
}

bool delete_password(PasswordManagerClient& client)
{
    std::string account_name;
    std::cout << "account name: ";
    std::cin >> account_name;

    if(!client.DeletePassword(account_name))
    {
        return false;
    }
    return true;
}

bool modify_password(PasswordManagerClient& client)
{
    std::string account_name;
    std::cout << "account name: ";
    std::cin >> account_name;

    std::string new_account_password;
    std::cout << "password: ";
    std::cin >> new_account_password;

    if(!client.ModifyPassword(account_name, new_account_password))
    {
        return false;
    }
    
    return true;
}

int main(int argc, char** argv)
{
    std::vector<CMD_ACTIONS> actions;

    std::string conf_filename("pswmgr.conf");
    std::string user;
    std::string new_user;
    std::string modify_username;

    for(int i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "-add_user") == 0 || strcmp(argv[i], "-a") == 0) 
        {
            actions.push_back(ACTION_ADDUSER);
            if(i+1 < argc)
                new_user = argv[i+1];
        }
        else if(strcmp(argv[i], "-login") == 0 || strcmp(argv[i], "-l") == 0)
        {
            actions.push_back(ACTION_AUTHENTICATE);
        }
        else if(strcmp(argv[i], "-user") == 0 || strcmp(argv[i], "-u") == 0)
        {
            if(i+1 < argc)
                user = argv[i+1];
        }
        else if(strcmp(argv[i], "-modify_user") == 0)
        {
            if(i+1 < argc)
                modify_username = argv[i+1];
            actions.push_back(ACTION_MODIFYUSER);
        }
        else if(strcmp(argv[i], "-add_password") == 0)
        {
            actions.push_back(ACTION_ADDPASSWORD);
        }
        else if(strcmp(argv[i], "-list") == 0)
        {
            actions.push_back(ACTION_LISTPASSWORDS);
        }
        else if(strcmp(argv[i], "-delete_password") == 0)
        {
            actions.push_back(ACTION_DELETEPASSWORD);
        }
        else if(strcmp(argv[i], "-modify_password") == 0)
        {
            actions.push_back(ACTION_MODIFYPASSWORD);
        }
        else if(strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "-config") == 0)
        {
            if(i + 1 < argc)
                conf_filename = argv[i+1];
        }
    }
 
    conf conf_file(conf_filename); 
    
    PasswordManagerClient client(conf_file, PasswordManagerClient::GetChannel(conf_file, conf_file.get_authentication_address_and_port()));

    if(actions.empty() && !user.empty())
    {
        actions.push_back(ACTION_AUTHENTICATE);
    }
    if(actions.empty())
    {
        actions.push_back(ACTION_HELP);
    }
    for(auto a : actions)
    {
        switch(a)
        {
            case ACTION_ADDUSER:
            {
                if(!add_user(client, new_user))
                {
                    return 1;
                }
                break;
            }
            case ACTION_MODIFYUSER:
            {
                if(!modify_user(client, modify_username))
                {
                    return 2;
                }
                break;
            }
            case ACTION_ADDPASSWORD:
            {
                add_password(client);
                break;
            }
            case ACTION_AUTHENTICATE:
            {
                if(!login(client, user))
                {
                    return 3;
                }
                break;
            }
            case ACTION_HELP:
            {
                std::cout << "Password Manager Command Line Interface" << std::endl;
                std::cout << "v1.0.0.0" << std::endl;
                std::cout << std::endl;
                std::cout << "TO BE COMPLETED" << std::endl;
                break;
            }
            case ACTION_LISTPASSWORDS:
            {
                if(!list_passwords(client))
                {
                    return 4;
                }
                break;
            }
            case ACTION_DELETEPASSWORD:
            {
                if(!delete_password(client))
                {
                    return 5;
                }       
                break;
            }
            case ACTION_MODIFYPASSWORD:
            {
                if(!modify_password(client))
                {
                    return 6;
                }
                break;
            }
        }
    }
}
