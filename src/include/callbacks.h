#include <sys/_types/_ssize_t.h>

#include <functional>
#include <memory>

#include "timestamp.h"

class TcpConnection;
class Buffer;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

using TimerCallback = std::function<void()>;

using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;

using MessageCallback =
    std::function<void(const TcpConnectionPtr&, Buffer* buf, Timestamp)>;

using CloseCallback = std::function<void(const TcpConnectionPtr&)>
