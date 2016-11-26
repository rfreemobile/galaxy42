

#include "utils/tasks/thread_rich.hpp"

namespace utils {

c_thread::~c_thread() {
	_info( string_id() << " - destructed");
}

std::string c_thread::string_id() const {
	std::ostringstream oss;
	oss <<"ThreadName="<<m_name;
	oss << ",(ID: ";
	if (m_thread) oss<<m_thread->get_id(); else oss<<"(null)";
	oss << ")";
	return oss.str();
}

void c_thread::join() {
	_info( string_id() << " - joining");

	auto time1 = std::chrono::high_resolution_clock::now();
	assert(m_thread);
	m_thread->join();
	auto time2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = time2 - time1;
	_info( string_id() << " - joined after " << elapsed.count() << " ms."	);
}

} // namespace utils

