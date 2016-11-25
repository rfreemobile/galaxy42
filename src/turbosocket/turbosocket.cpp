
#include "turbosocket/turbosocket.hpp"

#include <atomic>

#include <boost/asio.hpp>


#include "utils/tasks/onetime.hpp"

namespace n_turbosocket {

class m_command_system : public c_onetime_obj { ///< the TS command system
	public:
		m_command_system();
		virtual ~m_command_system() noexcept;
		virtual bool start();
		virtual bool stop();

		std::atomic<bool> m_should_end; ///< we want to finish. this will be read by various "children" e.g. reader object (in thread)

	protected:
		unique_ptr<std::thread> m_thread_reader;
};

class c_cmd_reader {
	friend class m_command_system;
	protected:
		m_command_system &sys; ///< system that is my parent
	public:
		c_cmd_reader(m_command_system &sys_);
		virtual void loop();
};

c_cmd_reader::c_cmd_reader(m_command_system &sys_) : sys(sys_) { }

void c_cmd_reader::loop() {
	while (! sys.m_should_end ) { // until our system wants to exit, loop:
	};
}

m_command_system::m_command_system() { }
m_command_system::~m_command_system() noexcept {
	destructor_used();
	// ...
}

bool m_command_system::start() {
	if (!start_begin()) return false;
	m_thread_reader = make_unique<std::thread>(
		[this]() {
			c_cmd_reader reader(*this);
			reader.loop();
		}
	);
	start_end(); return true;
}

bool m_command_system::stop() {
	if (!start_begin()) return false;
	m_should_end=true; // flag, will be seen by loops e.g. of reader, running in our threads
	_note("system stops: will join threads");
	if (m_thread_reader) {
		m_thread_reader->join(); // wait for all threads
		m_thread_reader.reset(nullptr);
	}
	start_end(); return true;
}


class c_turbosocket_system_pimpl {
	friend class c_turbosocket_system;
	private:
		unique_ptr<m_command_system> m_sys_cmd;
};


c_turbosocket_system::c_turbosocket_system()
: m_pimpl(make_unique<c_turbosocket_system_pimpl>())
{
	_note("Creating turbosocket system");
}

c_turbosocket_system::~c_turbosocket_system() noexcept {
	_note("Removing turbosocket system");
	destructor_used();
}

bool c_turbosocket_system::start() {
	if (!start_begin()) return false;
	_note("Starting turbosocket system");
	m_pimpl->m_sys_cmd = make_unique<m_command_system>();
	m_pimpl->m_sys_cmd->start();
	start_end();  return true;
}

bool c_turbosocket_system::stop() {
	if (!stop_begin()) return false;
	_note("Stopping turbosocket system");
	m_pimpl->m_sys_cmd->stop();
	stop_end();  return true;
}


 } // namespace n_turbosocket

