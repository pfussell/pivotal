#include <botan/tls_server.h>

class standard_policy : public Botan::TLS::Policy
{
public:
    std::vector<std::string> allowed_ciphers() const override
    { 
		return std::vector<std::string>({"AES-128","AES-128/GCM"});
    }

    std::vector<std::string> allowed_signature_hashes() const override
    {
		return std::vector<std::string>({"SHA-1","SHA-256"});
    }

    std::vector<std::string> allowed_key_exchange_methods() const override
    {
		return std::vector<std::string>({"RSA"});
    }

    bool acceptable_protocol_version(Botan::TLS::Protocol_Version /*version*/) const override
    {
		return true;
    }
};
