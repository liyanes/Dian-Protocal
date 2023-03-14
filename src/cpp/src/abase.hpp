#pragma once
#include <asio.hpp>
#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/SyncAwait.h>
#define ASYNC_SIMPLE_HAS_NOT_AIO
#include <async_simple/executors/SimpleExecutor.h>

#include <chrono>
#include <concepts>

#include "base.hpp"

class AsioExecutor : public async_simple::Executor {
public:
    AsioExecutor(asio::io_context &io_context) : io_context_(io_context) {}

    virtual bool schedule(Func func) override {
        asio::post(io_context_, std::move(func));
        return true;
    }

private:
    asio::io_context &io_context_;
};

template <typename T>
requires(!std::is_reference<T>::value) struct AsioCallbackAwaiter {
public:
    using CallbackFunction =
        std::function<void(std::coroutine_handle<>, std::function<void(T)>)>;

    AsioCallbackAwaiter(CallbackFunction callback_function)
        : callback_function_(std::move(callback_function)) {}

    bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        callback_function_(handle, [this](T t) { result_ = std::move(t); });
    }

    auto coAwait(async_simple::Executor *executor) noexcept {
        return std::move(*this);
    }

    T await_resume() noexcept { return std::move(result_); }

private:
    CallbackFunction callback_function_;
    T result_;
};

namespace dian {
    namespace socket{
        namespace tcp {
            class asocket {
            protected:
                asio::ip::tcp::socket socket_;
                asio::io_context& io_context_;
            public:
                asocket(asio::io_context& io_context);
                inline asio::ip::tcp::socket& socket() { return socket_; }
                inline asio::io_context& io_context() { return io_context_; }

                async_simple::coro::Lazy<std::error_code> accept(asio::ip::tcp::acceptor& acceptor) noexcept;
                async_simple::coro::Lazy<std::error_code> connect(asio::ip::tcp::endpoint endpoint) noexcept;
                async_simple::coro::Lazy<std::error_code> read_some(asio::mutable_buffer buffer) noexcept;
                async_simple::coro::Lazy<std::error_code> write_some(asio::const_buffer buffer) noexcept;
                async_simple::coro::Lazy<std::pair<std::error_code, size_t>> read(asio::mutable_buffer buffer) noexcept;
                async_simple::coro::Lazy<std::error_code> write(asio::const_buffer buffer) noexcept;
                void shutdown(asio::socket_base::shutdown_type what);
                void shutdown(asio::socket_base::shutdown_type what, std::error_code &ec);
                void close();
                void close(std::error_code &ec);
            };

            class aacceptor: protected asocket {
            protected:
                asio::ip::tcp::acceptor acceptor_;
            public:
                aacceptor(asio::io_context& io_context, asio::ip::tcp::endpoint endpoint);
                async_simple::coro::Lazy<std::error_code> accept() noexcept;
                using asocket::close;
                using asocket::read_some;
                using asocket::write_some;
                using asocket::read;
                using asocket::write;
                using asocket::shutdown;
            };
        }
    }
}
