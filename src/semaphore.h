#pragma once

#include <cstring>
#include <semaphore.h>
#include <stdexcept>

class Semaphore {
	sem_t sem;

public:
	explicit Semaphore(unsigned value) {
		if (sem_init(&sem, 0, value))
			throw std::runtime_error(std::string("sem_init() - ") +
			                         strerror(errno));
	}

	void wait() {
		for (;;) {
			if (sem_wait(&sem) == 0)
				return;

			if (errno == EINTR)
				continue;

			throw std::runtime_error(std::string("sem_wait() - ") +
			                         strerror(errno));
		}
	}

	// Returns false iff the operation would block
	bool try_wait() {
		for (;;) {
			if (sem_trywait(&sem) == 0)
				return true;

			if (errno == EINTR)
				continue;

			if (errno == EAGAIN)
				return false;

			throw std::runtime_error(std::string("sem_trywait() - ") +
			                         strerror(errno));
		}
	}

	void post() {
		if (sem_post(&sem))
			throw std::runtime_error(std::string("sem_post() - ") +
			                         strerror(errno));
	}

	~Semaphore() { (void)sem_destroy(&sem); }
};
