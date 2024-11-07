#include "data_utils.hpp"


namespace timer {
	bool _started;
	std::chrono::time_point<std::chrono::steady_clock> _start_time;
	std::chrono::time_point<std::chrono::steady_clock> _stop_time;

	void start(){
		assert(!_started);
		_start_time = std::chrono::steady_clock::now();
		_started = true;
	}

	void stop(){
		assert(_started);
		_stop_time = std::chrono::steady_clock::now();
		_started = false;
	}

	double n_seconds(){
		return(std::chrono::duration<double>(_stop_time - _start_time).count());
	}
}

namespace data_utils{
}


