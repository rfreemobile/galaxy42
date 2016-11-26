
#pragma once
#include "libs0.hpp"


namespace utils {

struct c_thread {
	public:
		const std::string m_name;
		unique_ptr<std::thread> m_thread;

		template< class Function, class... Args >
		explicit c_thread(const std::string &name, Function&& f, Args&&... args )
		: m_name(name), m_thread(make_unique<std::thread>(f, std::forward(args)... ))
		{
			_info( string_id() << " - created");
		}

		virtual ~c_thread();

		std::string string_id() const;

		virtual void join();

};

}

