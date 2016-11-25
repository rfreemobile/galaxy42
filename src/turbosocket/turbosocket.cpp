
#include "turbosocket/turbosocket.hpp"

#include <atomic>

#include <boost/asio.hpp>

typedef enum {
	e_onetime_run_new=10,
	e_onetime_run_starting=20,
	e_onetime_run_running=30,
	e_onetime_run_stopping=40,
	e_onetime_run_done=50
} t_onetime_run;

/*
Class that has start() and stop() methods that are forced to be called each at most one time:
* start() - when overwriten by child-class correctly: will execute it's main code exactly 1 time
* stop() - when overwriten by child-class correctly: will execute it's main code exactly 1 time, or
0 times if no start() was called
*/
class c_onetime_obj {
	protected:
		t_onetime_run m_run_status; //< use only under mutex m_run_status_mutex
		std::mutex m_run_status_mutex;

	protected:
		c_onetime_obj(); ///< starts object with status='new'

		// you probably want to override this methods, do it exactly as documented:
		virtual ~c_onetime_obj() noexcept; ///< calls a stop().
		/// override it as (see also c_example_onetime): { destructor_used(); .................... }

		virtual bool start(); ///< should run it's main body at most 1 time (then returns true, else false).
		/// override it as (see also c_example_onetime): { if(!start_begin()) return false; ....... start_end(); return true; }
		virtual bool stop(); ///< should run it's main body at most 1 time if start() completed before,
		// (then returns true, else false)
		/// override it as (see also c_example_onetime): { if(!stop_begin()) return false; ....... stop_end(); return true; }

		// use this methods:
		void destructor_used() noexcept;	///< if you inherit/overwrite the destructor, you MUST run this method at every begining of it

		bool start_begin(); ///< if you inherit/overwrite start(), you MUST run this method at every begining of it
		void start_end(); ///< if you inherit/overwrite start(), you MUST run this method at every end of it

		bool stop_begin(); ///< if you inherit/overwrite stop(), you MUST run this method at every begining of it
		void stop_end(); ///< if you inherit/overwrite stop(), you MUST run this method at every end of it

};

c_onetime_obj::c_onetime_obj() : m_run_status(e_onetime_run_new) {}
c_onetime_obj::~c_onetime_obj() noexcept {
	destructor_used();
	// ...
}

void c_onetime_obj::destructor_used() noexcept {
	try {
		stop();
	} catch(...) {}
}

bool c_onetime_obj::stop_begin() {
	bool we_must_stop_it=false;
	bool we_must_wait_for_done=false;
	{
		std::lock_guard<std::mutex> lg_run_status(m_run_status_mutex);
		if (m_run_status == e_onetime_run_stopping) { we_must_wait_for_done=true;	}
		else if (m_run_status == e_onetime_run_new) ;
		else if (m_run_status == e_onetime_run_running) {
			we_must_stop_it=true;
			m_run_status = e_onetime_run_stopping; // we are stopping it; mark this so no other ::stop() can colide
		}
	}
	if (we_must_stop_it) return true; // tell caller function stop() to carry on with it's main body
	if (we_must_wait_for_done) { // some other stop is running, wait for it
		auto time1 = std::chrono::high_resolution_clock::now();
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			{
				std::lock_guard<std::mutex> lg_run_status(m_run_status_mutex);
				if (m_run_status == e_onetime_run_done) break;
			}
		} // spin
		auto time2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = time2 - time1;
		_note("Waited for stopping of object for " << elapsed.count() << " ms." );
	} // must wait for done
	return false; // nothing to be done
}
void c_onetime_obj::stop_end() {
	std::lock_guard<std::mutex> lg_run_status(m_run_status_mutex);
	if (m_run_status != e_onetime_run_stopping) {
		_erro("Onetime object invalid use, stopping in incorrect state=" << static_cast<int>(m_run_status));
	}
	m_run_status = e_onetime_run_done;
}

bool c_onetime_obj::start_begin() {
	std::lock_guard<std::mutex> lg_run_status(m_run_status_mutex);
	if (m_run_status != e_onetime_run_new) {
		_info("Onetime object: can not be stared: current state="<<static_cast<int>(m_run_status));
		return false; // tell caller from user's ::start() to skip it's body
	}
	m_run_status = e_onetime_run_starting;
	return true;
}
void c_onetime_obj::start_end() {
	std::lock_guard<std::mutex> lg_run_status(m_run_status_mutex);
	if (m_run_status != e_onetime_run_starting) {
		_erro("Onetime object invalid use, starting in incorrect state=" << static_cast<int>(m_run_status));
	}
	m_run_status = e_onetime_run_running;
}

bool c_onetime_obj::start() {
	if (!start_begin()) return false;
	// ...
	start_end();
	return true;
}
bool c_onetime_obj::stop() {
	if (!stop_begin()) return false;
	// ...
	stop_end();
	return true;
}


class c_example_onetime : public c_onetime_obj {
	public:
		c_example_onetime();
		virtual ~c_example_onetime() noexcept;
		virtual bool start();
		virtual bool stop();
	protected:
		int *vvv;
};

c_example_onetime::c_example_onetime() : vvv(nullptr) { }
c_example_onetime::~c_example_onetime() noexcept {
	stop();
	if (vvv) { delete vvv; vvv=nullptr; }
}

bool c_example_onetime::start() {
	if (!start_begin()) return false;
	_info("start of " << (void*)this );
	start_end();  return true;
}
bool c_example_onetime::stop() {
	if (!stop_begin()) return false;
	_info("stop of " << (void*)this );
	stop_end();  return true;
}




namespace n_turbosocket {

class c_cmd_sys : public c_onetime_obj { ///< the TS command system
	public:
		c_cmd_sys();
		virtual ~c_cmd_sys() noexcept;
		virtual bool start();
		virtual bool stop();

		std::atomic<bool> m_should_end; ///< we want to finish. this will be read by various "children" e.g. reader object (in thread)

	protected:
		unique_ptr<std::thread> m_thread_reader;
};

class c_cmd_reader {
	friend class c_cmd_sys;
	protected:
		c_cmd_sys &sys; ///< system that is my parent
	public:
		c_cmd_reader(c_cmd_sys &sys_);
		virtual void loop();
};

c_cmd_reader::c_cmd_reader(c_cmd_sys &sys_) : sys(sys_) { }

void c_cmd_reader::loop() {
	while (! sys.m_should_end ) { // until our system wants to exit, loop:
	};
}

c_cmd_sys::c_cmd_sys() { }
c_cmd_sys::~c_cmd_sys() noexcept {
	destructor_used();
	// ...
}

bool c_cmd_sys::start() {
	if (!start_begin()) return false;
	m_thread_reader = make_unique<std::thread>(
		[this]() {
			c_cmd_reader reader(*this);
			reader.loop();
		}
	);
	start_end(); return true;
}

bool c_cmd_sys::stop() {
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
		unique_ptr<c_cmd_sys> m_cmd_sys;
};


c_turbosocket_system::c_turbosocket_system()
: m_pimpl(make_unique<c_turbosocket_system_pimpl>())
{
	_note("Creating turbosocket system");
}

c_turbosocket_system::~c_turbosocket_system() {
	_note("Removing turbosocket system");
}

void c_turbosocket_system::start() {
	_note("Starting turbosocket system");
}

};



