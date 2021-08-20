# Yet Another Tiny Web Server

## Main features

* Main-reactor is responsible for accepting new connections (via `Acceptor`), and dispatches each new connection to a sub-reactor, which resides in a threadpool initiated during the construction of `TcpServer`. Later, the sub-reactor will handle all I/O, timers and business logic related callbacks of that assigned connection.
* The design of `Buffer` is to efficiently coordinate with non-blocking I/O, and fully take advantage of the thread. Also, it makes the application code easier to write. E.g. the application needs only to call `TcpConnection::send()`, and is freed from the burdom of directly calling `send()`
* `timerfd_*` syscall is used to treat timers as normal file descriptors to make the code more consistent. `std::set` is used as container for timers to efficiently get expired timers
* RAII and smart pointers are used to prevent memory related issues

## TODO
- [ ] WebBench stress test
- [ ] Encapsulation of `epoll`
