#pragma once

namespace million {
namespace gateway {

class UserSession;
class UserSessionHandle {
public:
    UserSessionHandle() = default;
    explicit UserSessionHandle(UserSession* user_session)
        : user_session_(user_session) {}
    ~UserSessionHandle() = default;

    UserSessionHandle(const UserSessionHandle&) = default;
    void operator=(const UserSessionHandle& v) {
        user_session_ = v.user_session_;
    }

    UserSession& user_session() const { return *user_session_; }
    // auto iter() const { return iter_; }

private:
    UserSession* user_session_;
};

} // namespace gateway
} // namespace million