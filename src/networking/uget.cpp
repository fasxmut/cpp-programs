//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <botan/asio_stream.h>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <string_view>
#include <botan/auto_rng.h>
#include <botan/certstor_system.h>
#include <botan/tls.h>
#include <boost/signals2.hpp>

const std::string program_name = "uget";

using std::string_literals::operator""s;

using std::placeholders::_1;

namespace uget
{
	class tls_client;

	class net_monitor
	{
	public:
		using signal_type = boost::signals2::signal<void(const std::string &)>;
		using slot_type = signal_type::slot_type;
		using connection_type = boost::signals2::connection;
	private:
		connection_type __connection;
	public:
		net_monitor()
		{
		}
		virtual ~net_monitor()
		{
			__connection.disconnect();
		}
	public:
		void attach(
			int prior__,
			uget::tls_client & client__
		);
	public:
		void queue_message(const std::string & msg__)
		{
			std::clog << "================[net monitor] " << msg__ << std::endl;
		}
	};

	class credentials_manager:
		virtual public Botan::Credentials_Manager
	{
	private:
		Botan::System_Certificate_Store __store;
	public:
		std::vector<Botan::Certificate_Store *>
			trusted_certificate_authorities(const std::string &, const std::string &) override
		{
			return {&__store};
		}
	};

	class tls_client: virtual public std::enable_shared_from_this<uget::tls_client>
	{
	private:
		const std::string __host;
		const std::string __port;
		const std::string __uri;
	private:
		boost::asio::ip::tcp::resolver __resolver;
	private:
		std::shared_ptr<Botan::RNG> __tls_rng;
		std::shared_ptr<Botan::Credentials_Manager> __tls_credentials;
		std::shared_ptr<Botan::TLS::Session_Manager> __tls_session;
		std::shared_ptr<Botan::TLS::Policy> __tls_policy;
		Botan::TLS::Server_Information __tls_server_info;
		std::shared_ptr<Botan::TLS::Context> __tls_context;
		std::shared_ptr<Botan::TLS::Stream<boost::beast::tcp_stream>> __tls_stream;
	private:
		boost::beast::http::request<boost::beast::http::empty_body> __request;
		boost::beast::http::response<boost::beast::http::string_body> __response;
		boost::beast::flat_buffer __buffer;
	private:
		uget::net_monitor::signal_type __signal;
		uget::net_monitor & __monitor;
	public:
		virtual ~tls_client()
		{
			this->signal("tls_client destructor is called");
		}
	public:
		tls_client(
			const std::string_view host__,
			const std::string_view port__,
			const std::string_view uri__,
			boost::asio::any_io_executor executor__,
			uget::net_monitor & monitor__
		):
			__host{host__},
			__port{port__},
			__uri{uri__},

			__resolver{executor__},

			__tls_rng{
				std::make_shared<Botan::AutoSeeded_RNG>()
			},
			__tls_credentials{
				std::make_shared<uget::credentials_manager>()
			},
			__tls_session{
				std::make_shared<Botan::TLS::Session_Manager_In_Memory>(__tls_rng)
			},
			__tls_policy{
				std::make_shared<Botan::TLS::Policy>()
			},
			__tls_server_info{},

			__tls_context{
				std::make_shared<Botan::TLS::Context>(
					__tls_credentials,
					__tls_rng,
					__tls_session,
					__tls_policy,
					__tls_server_info
				)
			},

			__tls_stream{
				std::make_shared<Botan::TLS::Stream<boost::beast::tcp_stream>>(
					__tls_context,
					executor__
				)
			},

			__monitor{monitor__}
		{
			__monitor.attach(0, * this);
		}
	public:
		boost::asio::awaitable<void> run()
		{
			auto result_body = co_await this->run_it();
			if (result_body.size() == 0)
			{
				std::clog << "got nothing" << std::endl;
			}
			else
			{
				std::clog << "got: " << std::flush;
				std::cout  << result_body << std::endl;
			}
			co_await this->shutdown();
		}
	public:
		boost::asio::awaitable<std::string> run_it()
		{
			auto results = co_await this->resolve();

			bool status = co_await this->connect(results, 0);
			if (! status)
				co_return "";

			status = co_await this->handshake(0);
			if (! status)
				co_return "";

			__request.set(boost::beast::http::field::host, __host);
			__request.method(boost::beast::http::verb::get);
			__request.version(11);
			__request.target(__uri);
			__request.set(boost::beast::http::field::content_type, "text/html");
			status = co_await this->write(0);
			if (! status)
				co_return "";

			co_return co_await this->read(0);
		}
	public:
		boost::asio::awaitable<boost::asio::ip::tcp::resolver::results_type> resolve()
		{
			auto [ec, results] = co_await __resolver.async_resolve(
				__host,
				__port,
				boost::asio::as_tuple(
					boost::asio::use_awaitable
				)
			);
			if (ec)
				throw std::system_error{ec, "async resolve error"};
			std::clog << "async resolve ok" << std::endl;
			co_return results;
		}
	public:
		boost::asio::awaitable<bool> connect(auto results__, int times__)
		{
			__tls_stream->next_layer().expires_after(std::chrono::seconds(2));
			auto [ec, ep] = co_await __tls_stream->next_layer().async_connect(
				results__,
				boost::asio::as_tuple(boost::asio::use_awaitable)
			);
			++times__;
			if (! ec)
			{
				std::clog << "connected after trying " << times__ << " times" << std::endl;
				co_return true;
			}
			else if (times__ > 10)
			{
				throw std::system_error{ec, "async connect error (have tried "s
				+  std::to_string(times__) + " times)"};
				co_return false;
			}
			else
			{
				std::clog << "Connection retrying ... " << times__ << std::endl;
				co_return co_await this->connect(results__, times__);
			}
			co_return false;
		}
	public:
		boost::asio::awaitable<bool> handshake(int times__)
		{
			__tls_stream->next_layer().expires_after(std::chrono::seconds(2));
			auto [ec] = co_await __tls_stream->async_handshake(
				Botan::TLS::Connection_Side::Client,
				boost::asio::as_tuple(boost::asio::use_awaitable)
			);
			++times__;
			if (! ec)
			{
				this->signal("async handshaked after trying "s + std::to_string(times__) + " times");
				co_return true;
			}
			else if (times__ > 10)
			{
				throw std::system_error{ec,
					"handshake error after trying "s + std::to_string(times__) + " times"
				};
				co_return false;
			}
			else
			{
				std::clog << "Handshake retrying ... " << times__ << std::endl;
				co_return co_await this->handshake(times__);
			}
			co_return false;
		}
	public:
		boost::asio::awaitable<bool> write(int times__)
		{
			__tls_stream->next_layer().expires_after(std::chrono::seconds(2));
			auto [ec, bytes] = co_await boost::beast::http::async_write(
				* __tls_stream,
				__request,
				boost::asio::as_tuple(boost::asio::use_awaitable)
			);
			++times__;
			if (! ec)
			{
				std::clog << "Request ok after trying " << times__ << " times\n";
				co_return true;
			}
			else if (times__ > 10)
			{
				throw std::system_error{
					ec,
					"async write error after trying "s + std::to_string(times__) + " times"
				};
				co_return false;
			}
			else
			{
				std::clog << "Request retrying ..." << times__ << std::endl;
				co_return co_await this->write(times__);
			}
			co_return false;
		}
	public:
		boost::asio::awaitable<std::string> read(int times__)
		{
			__tls_stream->next_layer().expires_after(std::chrono::seconds(2));
			auto [ec, bytes] = co_await boost::beast::http::async_read(
				* __tls_stream,
				__buffer,
				__response,
				boost::asio::as_tuple(boost::asio::use_awaitable)
			);
			++times__;
			if (! ec)
			{
				std::clog << "Async read ok ater trying " << times__ << " times\n";
				this->signal("Read OK: http body is got successfully.");
				co_return __response.body();
			}
			else if (times__ > 10)
			{
				throw std::system_error{
					ec,
					"async read error after trying "s + std::to_string(times__) + " times"
				};
				co_return "";
			}
			else
			{
				std::clog << "Read retrying ... " << times__ << std::endl;
				co_return co_await this->read(times__);
			}
			co_return "";
		}
	public:
		boost::asio::awaitable<void> shutdown()
		{
			auto [ec] = co_await __tls_stream->async_shutdown(
				boost::asio::as_tuple(boost::asio::use_awaitable)
			);
			std::clog << "closed: " << ec << std::endl;
			this->signal("async shutdown OK. "s + ec.message());
		}
	public:
		uget::net_monitor::connection_type connect(
			int prior__,
			const uget::net_monitor::slot_type & slot__
		)
		{
			return __signal.connect(prior__, slot__);
		}
		void signal(const std::string msg__)
		{
			this->__signal(msg__);
		}
	};

	void uget::net_monitor::attach(
		int prior__,
		uget::tls_client & client__
	)
	{
		client__.connect(
			prior__,
			std::bind(
				&uget::net_monitor::queue_message,
				this,
				_1
			)
		);
	}
}	// namespace uget

int main(int argc, char * argv[])
try
{
	std::string host;
	std::string port;
	std::string uri;

	if (argc == 4)
	{
		host = argv[1];
		port = argv[2];
		uri = argv[3];
	}
	else
	{
		std::string line3 = ""s + program_name + " <host> <port> <uri>";
		std::string line4 = ""s + "For example: " + program_name + " example.com 443 /cpp";
		std::clog
			<< "\n\n" << "command options:\n"
			<< line3 << '\n' << line4 << "\n\n";
		throw std::runtime_error{"arguments error"};
	}

	uget::net_monitor monitor;

	boost::asio::io_context io_context;
	boost::asio::co_spawn(
		io_context,
		[&host, &port, &uri, &monitor] -> boost::asio::awaitable<void>
		{
			try
			{
				co_await std::make_shared<uget::tls_client>(
					host,
					port,
					uri,
					co_await boost::asio::this_coro::executor,
					monitor
				)->run();
			}
			catch (const std::exception & e)
			{
				std::cerr << "\n\n\n";
				std::cerr << "Caught std::exception network:\n" << e.what()
					<< std::endl << std::endl;
			}
		},
		boost::asio::detached
	);

	io_context.run();

	return 0;
}
catch (std::exception & e)
{
	std::cerr << "\n\n\n";
	std::cerr << "Caught std::exception:\n" << e.what() << std::endl << std::endl;
	return 1;
}

