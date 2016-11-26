
#include "utils/tasks/onetime.hpp"
#include "libs0.hpp"



// Example of use - copy/paste


c_example_onetime::c_example_onetime() : vvv(nullptr) { }
c_example_onetime::~c_example_onetime() {
	destructor_used();
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



c_onetime_obj::c_onetime_obj() : m_run_status(e_onetime_run_new) {}
c_onetime_obj::~c_onetime_obj() {
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

