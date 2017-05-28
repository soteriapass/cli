#include "token_authenticator.h"

TokenAuthenticator::TokenAuthenticator(const std::string& token) 
: m_Token(token) 
{
}

grpc::Status TokenAuthenticator::GetMetadata(grpc::string_ref service_url, grpc::string_ref method_name, const grpc::AuthContext& channel_auth_context, std::multimap<grpc::string, grpc::string>* metadata) 
{
    metadata->insert(std::make_pair("x-custom-auth-ticket", m_Token));
    return grpc::Status::OK;
}
