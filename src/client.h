#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "conf.h"
#include "token_authenticator.h"

#include "pswmgr.grpc.pb.h"

class PasswordManagerClient final
{
public:
    PasswordManagerClient(conf& conf_file, std::shared_ptr<grpc::Channel> channel);

    bool Authenticate(const std::string& user, const std::string& pass, const std::string& token, bool& need2fa, bool create = false);
    bool CreateUser(const std::string& user, const std::string& pass, std::string& tfaSecret, std::vector<int>& scratchCodes, std::string& qrcode);
    bool UpdateUserPassword(const std::string& user, const std::string& pass);

    const std::string& GetLastError() const { return m_LastError; }

    bool AddPassword(const std::string& account_name, const std::string& username, const std::string& password, const std::string& extra);
    bool DeletePassword(const std::string& account_name);
    bool ListPasswords();
    bool ModifyPassword(const std::string& account_name, const std::string& new_password);

    static auto GetChannel(conf& conf_file, const std::string& address) -> decltype(grpc::CreateChannel("", grpc::SslCredentials(grpc::SslCredentialsOptions())))
    {
        std::ifstream file(conf_file.get_server_ca_file(), std::ifstream::in);
        std::string ca = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        auto credOptions = grpc::SslCredentialsOptions();
        credOptions.pem_root_certs = ca;
        auto sslCreds = grpc::SslCredentials(credOptions);
        return grpc::CreateChannel(address, sslCreds);
    }

    static auto GetChannel(conf& conf_file, const std::string& address, const std::shared_ptr<grpc::CallCredentials>& callCreds) ->decltype(GetChannel(conf_file, ""))
    {
        std::ifstream file(conf_file.get_server_ca_file(), std::ifstream::in);
        std::string ca = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        auto credOptions = grpc::SslCredentialsOptions();
        credOptions.pem_root_certs = ca;

        auto sslCreds = grpc::SslCredentials(credOptions);
        auto compCreds = grpc::CompositeChannelCredentials(sslCreds, callCreds);
        return grpc::CreateChannel(address, compCreds);
    }

private:
    std::unique_ptr<pswmgr::Authentication::Stub>  m_AuthStub;
    std::unique_ptr<pswmgr::PasswordManager::Stub> m_PassMgrStub;
    std::unique_ptr<pswmgr::UserManagement::Stub>  m_UserMgrStub;
    std::string m_LastError;

    TokenAuthenticator* m_TokenAuth;
    conf& m_Conf;
};
