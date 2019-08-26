#pragma once

#include <cstring>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

// Returns true iff the command was executed and exited successfully
template <class... Args>
bool run_command(bool quiet, const std::string& cmd, Args&&... args) {
	pid_t pid = fork();
	if (pid < 0)
		throw std::runtime_error(std::string("fork() - ") + strerror(errno));

	if (pid == 0) {
		close(STDIN_FILENO);
		if (quiet) {
			freopen("/dev/null", "w", stdout);
			freopen("/dev/null", "w", stderr);
		}

		execlp(
		   cmd.data(), cmd.data(), std::string(args).data()..., (char*)nullptr);
		_exit(-1);
	}

	int status;
	if (waitpid(pid, &status, 0) != pid)
		throw std::runtime_error(std::string("waitpid() - ") + strerror(errno));

	return (WIFEXITED(status) and WEXITSTATUS(status) == 0);
}
