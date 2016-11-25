
#pragma once

#include "libs1.hpp"
#include "utils/tasks/onetime.hpp"


namespace n_turbosocket {

class c_turbosocket_system_pimpl;

class c_turbosocket_system : public c_onetime_obj {
	protected:
		unique_ptr<c_turbosocket_system_pimpl> m_pimpl;
	public:
		c_turbosocket_system();
		virtual ~c_turbosocket_system() noexcept;

		virtual bool start();
		virtual bool stop();
};


} // namespace n_turbosocket

