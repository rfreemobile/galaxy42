
#include "turbosocket/turbosocket.hpp"

#include <atomic>

#include <boost/asio.hpp>


#include "utils/tasks/onetime.hpp"
#include "utils/tasks/thread_rich.hpp"

namespace n_turbosocket {


class m_command_system : public c_onetime_obj { ///< the TS command system
	public:
		m_command_system();
		virtual ~m_command_system();
		virtual bool start();
		virtual bool stop();

		std::queue< std::string > m_queue_in; ///< data that we read - commands to execute
		std::queue< std::string > m_queue_out; ///< data that we will write - replies etc

		std::atomic<bool> m_should_end; ///< we want to finish. this will be read by various "children" e.g. reader object (in thread)

	protected:
		unique_ptr<utils::c_thread> m_thread_in; ///< thread to move data from input into IN queue
		unique_ptr<utils::c_thread> m_thread_exec; ///< thread to execute data, and save result to OUT queue
		unique_ptr<utils::c_thread> m_thread_out; ///< thread to send the data from OUT queue to outside

		void join_and_erase(unique_ptr<utils::c_thread> & my_thread); ///< join this thread, then erase/nullptr it

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
m_command_system::~m_command_system() {
	destructor_used();
	// ...
}

bool m_command_system::start() {
	if (!start_begin()) return false;
	m_thread_in = make_unique<utils::c_thread>(
		"TS-cmd",
		[this]() {
			c_cmd_reader reader(*this);
			reader.loop();
		}
	);
	start_end(); return true;
}

void m_command_system::join_and_erase(unique_ptr<utils::c_thread> & my_thread) {
	if (my_thread) {
		_info("Joining thread...");
		my_thread->join();
		_info("Joining thread - done");
		my_thread = nullptr;
	}
}

bool m_command_system::stop() {
	_note("stop - begin");
	if (!stop_begin()) return false;
	m_should_end=true; // flag, will be seen by loops e.g. of reader, running in our threads
	_note("system stops: will join threads");

	if (m_thread_in) {
		m_thread_in->join(); // wait for all threads
		m_thread_in.reset(nullptr);
	}

	stop_end(); return true;
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

c_turbosocket_system::~c_turbosocket_system() {
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

