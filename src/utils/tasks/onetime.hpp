
#pragma once

#include "libs0.hpp"

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
		virtual ~c_onetime_obj(); ///< calls a stop().
		/// override it as (see also c_example_onetime): { destructor_used(); .................... }

		virtual bool start(); ///< should run it's main body at most 1 time (then returns true, else false).
		/// override it as (see also c_example_onetime): { if(!start_begin()) return false; ....... start_end(); return true; }
		virtual bool stop(); ///< should run it's main body at most 1 time if start() completed before,
		// (then returns true, else false)
		/// override it as (see also c_example_onetime): { if(!stop_begin()) return false; ....... stop_end(); return true; }

		// use this methods:
		void destructor_used() noexcept ;	///< if you inherit/overwrite the destructor, you MUST run this method at every begining of it

		bool start_begin(); ///< if you inherit/overwrite start(), you MUST run this method at every begining of it
		void start_end(); ///< if you inherit/overwrite start(), you MUST run this method at every end of it

		bool stop_begin(); ///< if you inherit/overwrite stop(), you MUST run this method at every begining of it
		void stop_end(); ///< if you inherit/overwrite stop(), you MUST run this method at every end of it

};


class c_example_onetime : public c_onetime_obj {
	public:
		c_example_onetime();
		virtual ~c_example_onetime();
		virtual bool start();
		virtual bool stop();
	protected:
		int *vvv;
};

