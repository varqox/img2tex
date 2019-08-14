#pragma once

#include <cstring>
#include <stdexcept>
#include <unistd.h>

class TemporaryFile {
	std::string path_; // absolute path
	int fd_ = -1;

public:
	/// Does NOT create a temporary file
	TemporaryFile() = default;

	/// The last six characters of template must be "XXXXXX" and these are
	/// replaced with a string that makes the filename unique.
	explicit TemporaryFile(std::string templ) {
		fd_ = mkstemp(templ.data());
		if (fd_ == -1)
			throw std::runtime_error(std::string("mkstemp() - ") +
			                         strerror(errno));
		path_ = std::move(templ);
	}

	TemporaryFile(const TemporaryFile&) = delete;
	TemporaryFile& operator=(const TemporaryFile&) = delete;

	TemporaryFile(TemporaryFile&& tf) noexcept
	   : path_(std::move(tf.path_)), fd_(tf.fd_) {
		tf.fd_ = -1;
	}

	TemporaryFile& operator=(TemporaryFile&& tf) noexcept {
		if (is_open()) {
			unlink(path_.c_str());
			(void)close(fd_);
		}

		path_ = std::move(tf.path_);
		fd_ = tf.fd_;
		tf.fd_ = -1;

		return *this;
	}

	~TemporaryFile() {
		if (is_open()) {
			unlink(path_.c_str());
			(void)close(fd_);
		}
	}

	operator int() const { return fd_; }

	bool is_open() const noexcept { return (fd_ != -1); }

	const std::string& path() const noexcept { return path_; }
};