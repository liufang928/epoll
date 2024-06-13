#include <iostream>
#include <string>

class MsgType
{
public:
    enum Type
    {
        LOGIN = 0,
        LOGOUT,
        UNKNOW
    };
    static std::string ToString(MsgType::Type type)
    {
        switch (type)
        {
#define XX(name)        \
    case MsgType::name:  \
        return #name    
            XX(LOGIN);
            XX(LOGOUT);
        default:
            return "UNDEFINE";
#undef XX
        }
    }
    static MsgType::Type FromStr(const std::string& str)
    {
#define XX(type,v)              \
    if(str == #v){              \
        return MsgType::type;   \
    }
    XX(LOGIN,login);
    XX(LOGOUT,logout);
    return MsgType::UNKNOW;
    }
};

class Header
{
public:
    Header()
    {
        type = MsgType::UNKNOW;
        len = 0;
    }
public:
    MsgType::Type type;
    int len;
};

class MessageLogin : public Header
{
public:
    char username[32];
    char password[32];
    char data[952];
};

int main() {
    std::cout << "Size of Header: " << sizeof(Header) << " bytes" << std::endl;
    std::cout << "Size of MessageLogin: " << sizeof(MessageLogin) << " bytes" << std::endl;
    return 0;
}
