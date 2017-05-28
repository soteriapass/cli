#include "client.h"

PasswordManagerClient::PasswordManagerClient(conf& conf_file, std::shared_ptr<grpc::Channel> channel)
: m_AuthStub(pswmgr::Authentication::NewStub(channel)) 
, m_TokenAuth(nullptr)
, m_Conf(conf_file)
{
}

bool PasswordManagerClient::Authenticate(const std::string& user, const std::string& pass, const std::string& token, bool& need2fa, bool createUser)
{
    grpc::ClientContext context;
    pswmgr::AuthenticationRequest request;
    request.set_username(user);
    request.set_password(pass);

    pswmgr::AuthReply response;

    if(token.size() != 0 && need2fa)
    {
        int tfa_token = atoi(token.c_str());
        request.set_tfa_token(tfa_token);
    }

    grpc::Status status = m_AuthStub->Authenticate(&context, request, &response);
    if(status.error_code() == grpc::StatusCode::UNAUTHENTICATED)
    {
        m_LastError = status.error_message();
        return false;
    }
    else if(!status.ok())
    {
        m_LastError = "Could not connect to server";
        return false;
    }

    if(response.token_needed_for_2fa())
    {
        m_LastError = status.error_message();
        need2fa = response.token_needed_for_2fa();
        return false;
    }

    m_TokenAuth = new TokenAuthenticator(response.token());

    auto callCreds = grpc::MetadataCredentialsFromPlugin(std::unique_ptr<grpc::MetadataCredentialsPlugin>(m_TokenAuth));
    m_PassMgrStub = pswmgr::PasswordManager::NewStub(GetChannel(m_Conf, m_Conf.get_password_manager_address_and_port(), callCreds));
    m_UserMgrStub = pswmgr::UserManagement::NewStub(GetChannel(m_Conf, m_Conf.get_user_mangement_address_and_port(), callCreds));

    return true;
}

bool PasswordManagerClient::CreateUser(const std::string& user, const std::string& pass, std::string& tfaSecret, std::vector<int>& scratchCodes, std::string& qrcode)
{
    grpc::ClientContext context;
    pswmgr::UserCreationRequest request;
    request.set_username(user);
    request.set_password(pass);
    request.set_add_2fa(true);

    pswmgr::UserCreationReply response;

    grpc::Status status = m_UserMgrStub->CreateUser(&context, request, &response);
    if(!status.ok())
    {
        m_LastError = "Could not connect to the server";
        return false;
    }

    if(!response.success())
    {
        m_LastError = "Unknown";
        return false;
    }

    tfaSecret = response.secret();
    for(int i = 0; i < response.scratch_codes_size(); ++i)
    {
        scratchCodes.push_back(response.scratch_codes(i));
    }

    qrcode = response.qrcode();
    return true;
}

bool PasswordManagerClient::UpdateUserPassword(const std::string& user, const std::string& pass)
{
    grpc::ClientContext context;
    pswmgr::UserPasswordUpdateRequest request;
    request.set_username(user);
    request.set_password(pass);

    pswmgr::SimpleReply response;

    grpc::Status status = m_UserMgrStub->UpdateUserPassword(&context, request, &response);
    if(!status.ok())
    {
        m_LastError = status.error_message();
        return false;
    }

    return true;
}

bool PasswordManagerClient::AddPassword(const std::string& account_name, const std::string& username, const std::string& password, const std::string& extra)
{
    grpc::ClientContext context;
    pswmgr::PasswordEntry request;
    request.set_account_name(account_name);
    request.set_username(username);
    request.set_password(password);
    request.set_extra(extra);

    pswmgr::SimpleReply response;
    grpc::Status status = m_PassMgrStub->AddPassword(&context, request, &response);
    if(!status.ok())
    {
        m_LastError = "Could not connect to the server";
        return false;
    }

    if(!response.success())
    {
        m_LastError = "Uknown";
        return false;
    }

    return true;
}

bool PasswordManagerClient::DeletePassword(const std::string& account_name)
{
    grpc::ClientContext context;
    pswmgr::PasswordEntry request;
    request.set_account_name(account_name);

    pswmgr::SimpleReply response;
    grpc::Status status = m_PassMgrStub->DeletePassword(&context, request, &response);
    if(!status.ok())
    {
        m_LastError = status.error_message();
        return false;
    }

    return true;
}

bool PasswordManagerClient::ListPasswords()
{
    grpc::ClientContext context;
    pswmgr::SimpleRequest request;
    pswmgr::PasswordList response;

    grpc::Status status = m_PassMgrStub->ListPasswords(&context, request, &response);
    if(!status.ok())
    {
        m_LastError = status.error_message();
        return false;
    }

    for(const pswmgr::PasswordEntry& entry: response.passwords())
    {
        std::cout << "account name: " << entry.account_name() << std::endl;
        std::cout << "username: " << entry.username() << std::endl;
        std::cout << "password: " << entry.password() << std::endl;
        std::cout << "extra: " << entry.extra() << std::endl;
        std::cout << std::endl;
    }
    return true;
}

bool PasswordManagerClient::ModifyPassword(const std::string& account_name, const std::string& new_password)
{
    grpc::ClientContext context;
    pswmgr::PasswordEntry request;
    request.set_account_name(account_name);
    request.set_password(new_password);

    pswmgr::SimpleReply response;
    grpc::Status status = m_PassMgrStub->ModifyPassword(&context, request, &response);
    if(!status.ok())
    {
        m_LastError = status.error_message();
        return false;
    }

    return true;
}

