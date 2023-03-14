#include "abase.hpp"

using namespace dian::socket::tcp;
using namespace std;
using namespace async_simple::coro;

asocket::asocket(asio::io_context &io_context) : socket_(io_context), io_context_(io_context) {}

Lazy<error_code> asocket::accept(asio::ip::tcp::acceptor& acceptor) noexcept {
    co_return co_await AsioCallbackAwaiter<error_code>{
        [&](coroutine_handle<> handle, auto set_resume_value) {
            acceptor.async_accept(
                socket_, [handle, set_resume_value = std::move(set_resume_value)](auto ec) mutable {
                    set_resume_value(std::move(ec));
                    handle.resume();
                });
        }};
}

Lazy<error_code> asocket::connect(asio::ip::tcp::endpoint endpoint) noexcept {
    co_return co_await AsioCallbackAwaiter<error_code>{
        [&](coroutine_handle<> handle, auto set_resume_value) {
            socket_.async_connect(
                endpoint, [handle, set_resume_value = std::move(set_resume_value)](auto ec) mutable {
                    set_resume_value(std::move(ec));
                    handle.resume();
                });
        }};
}

Lazy<error_code> asocket::read_some(asio::mutable_buffer buffer) noexcept {
    co_return co_await AsioCallbackAwaiter<error_code>{
        [&](coroutine_handle<> handle, auto set_resume_value) {
            socket_.async_read_some(
                buffer, [handle, set_resume_value = std::move(set_resume_value)](auto ec, auto size) mutable {
                    set_resume_value(std::move(ec));
                    handle.resume();
                });
        }};
}

Lazy<error_code> asocket::write_some(asio::const_buffer buffer) noexcept {
    co_return co_await AsioCallbackAwaiter<error_code>{
        [&](coroutine_handle<> handle, auto set_resume_value) {
            socket_.async_write_some(
                buffer, [handle, set_resume_value = std::move(set_resume_value)](auto ec, auto size) mutable {
                    set_resume_value(std::move(ec));
                    handle.resume();
                });
        }};
}

Lazy<std::pair<error_code, size_t>> asocket::read(asio::mutable_buffer buffer) noexcept {
    co_return co_await AsioCallbackAwaiter<pair<error_code, size_t>>{
        [&](coroutine_handle<> handle, auto set_resume_value) mutable {
            asio::async_read(
                socket_, buffer,
                [handle, set_resume_value = std::move(set_resume_value)](auto ec, auto size) mutable {
                    set_resume_value(std::make_pair(std::move(ec), size));
                    handle.resume();
                });
        }};
}

Lazy<error_code> asocket::write(asio::const_buffer buffer) noexcept {
    co_return co_await AsioCallbackAwaiter<error_code>{
        [&](coroutine_handle<> handle, auto set_resume_value) mutable {
            asio::async_write(
                socket_, buffer,
                [handle, set_resume_value = std::move(set_resume_value)](auto ec, auto size) mutable {
                    set_resume_value(std::move(ec));
                    handle.resume();
                });
        }};
}

void asocket::close() {
    socket_.close();
}

void asocket::close(error_code& ec) {
    socket_.close(ec);
}

void asocket::shutdown(asio::socket_base::shutdown_type type) {
    socket_.shutdown(type);
}

void asocket::shutdown(asio::socket_base::shutdown_type type, error_code& ec) {
    socket_.shutdown(type, ec);
}

aacceptor::aacceptor(asio::io_context& io_context, asio::ip::tcp::endpoint endpoint)
    : acceptor_(io_context, endpoint), asocket(io_context) {}

Lazy<error_code> aacceptor::accept() noexcept {
    return asocket::accept(acceptor_);
}
