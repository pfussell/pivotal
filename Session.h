class Session
{
public:
	Session(boost::asio::io_service& io_service,
		boost::asio::ssl::context& context)
	: socket_(io_service, context)
	{
	}

	ssl_socket::lowest_layer_type& socket()
	{
		return socket_.lowest_layer();
	}

	void Start()
	{
		socket_.async_handshake(boost::asio::ssl::stream_base::server,
			boost::bind(&session::handle_handshake, this,
				boost::asio::placeholders::error));
	}

	void HandleHandshake(const boost::system::error_code& error)
	{
		if (!error)
		{
			socket_.async_read_some(boost::asio::buffer(data_, max_length),
				boost::bind(&session::handle_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			delete this;
		}
	}

	void HandleRead(const boost::system::error_code& error, size_t bytes_transferred)
	{
		if (!error)
		{
			boost::asio::async_write(socket_,
				boost::asio::buffer(data_, bytes_transferred),
				boost::bind(&session::handle_write, this,
					boost::asio::placeholders::error));
		}
		else
		{
			delete this;
		}
	}

	void HandleWrite(const boost::system::error_code& error)
	{
		if (!error)
		{
			socket_.async_read_some(boost::asio::buffer(data_, max_length),
				boost::bind(&session::handle_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			delete this;
		}
	}

private:
	ssl_socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
};
