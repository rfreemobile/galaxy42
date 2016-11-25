
#include "libs1.hpp"

#pragma once

namespace n_turbosocket {

class c_turbosocket_system_pimpl;

class c_turbosocket_system {
	protected:
		unique_ptr<c_turbosocket_system_pimpl> m_pimpl;
	public:
		c_turbosocket_system();
		virtual ~c_turbosocket_system();

		virtual void start();
};


};

