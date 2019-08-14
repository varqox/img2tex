#pragma once

#include "semaphore.h"

#include <deque>
#include <mutex>
#include <optional>

template <class Job>
class JobQueue {
private:
	Semaphore jobs_limit;
	Semaphore queued_jobs {0};
	std::mutex lock;
	std::deque<Job> jobs;

	Job extract_job() {
		std::lock_guard<std::mutex> guard(lock);
		if (jobs.empty()) {
			queued_jobs.post();
			throw NoMoreJobs();
		}

		auto job = jobs.front();
		jobs.pop_front();
		jobs_limit.post();
		return job;
	}

public:
	struct NoMoreJobs {};

	explicit JobQueue(unsigned size) : jobs_limit(size) {}

	JobQueue(const JobQueue&) = delete;
	JobQueue& operator=(const JobQueue&) = delete;

	Job get_job() {
		queued_jobs.wait();
		return extract_job();
	}

	std::optional<Job> try_get_job() {
		if (queued_jobs.try_wait())
			return extract_job();

		return std::nullopt;
	}

	void add_job(Job job) {
		jobs_limit.wait();
		std::lock_guard<std::mutex> guard(lock);
		jobs.emplace_back(std::move(job));
		queued_jobs.post();
	}

	// Calling add_job() after this method is forbidden
	void signal_no_more_jobs() { queued_jobs.post(); }

	void increment_queue_size() { jobs_limit.post(); }
};
