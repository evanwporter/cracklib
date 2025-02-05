#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <direct.h>
#define popen _popen
#define pclose _pclose
#define sleep(x) Sleep(1000 * (x))
#define snprintf _snprintf
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <vector>

#define PWD_LEN 128

char default_ABC[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
char filename[255];
char statname[259];
int num_threads = 2;

std::mutex pwdMutex;
std::mutex finishedMutex;

struct CrackStatus {
	char ABC[64];
	char password[PWD_LEN];
	char password_good[PWD_LEN];
	unsigned int curr_len;
	long counter;
	int finished;
};

CrackStatus crack_status;

void save_status() {
	FILE* file = fopen(statname, "wb");
	if (!file) return;

	finishedMutex.lock();
	fwrite(&crack_status, sizeof(CrackStatus), 1, file);
	finishedMutex.unlock();

	fclose(file);
}

int load_status() {
	FILE* file = fopen(statname, "rb");
	if (!file) return 0;

	fread(&crack_status, sizeof(CrackStatus), 1, file);
	fclose(file);

	if (crack_status.finished) {
		printf("GOOD: Archive successfully cracked\n");
		printf("      Password: '%s'\n", crack_status.password_good);
		return 1;
	}
	else {
		printf("INFO: Resuming cracking from password: '%s'\n", crack_status.password);
	}

	return 0;
}

void nextpass2(char* p, unsigned int n) {
	if (p[n] == crack_status.ABC[strlen(crack_status.ABC) - 1]) {
		p[n] = crack_status.ABC[0];
		if (n > 0)
			nextpass2(p, n - 1);
		else {
			memmove(p + 1, p, crack_status.curr_len);
			p[0] = crack_status.ABC[0];
			p[++crack_status.curr_len] = '\0';
		}
	}
	else {
		p[n] = crack_status.ABC[strchr(crack_status.ABC, p[n]) - crack_status.ABC + 1];
	}
}

char* nextpass() {
	char* ok = (char*)malloc(PWD_LEN + 1);

	pwdMutex.lock();
	strcpy(ok, crack_status.password);
	nextpass2(crack_status.password, crack_status.curr_len - 1);
	pwdMutex.unlock();

	return ok;
}

void status_thread() {
	int pwds;
	const short status_sleep = 3;

	while (true) {
		sleep(status_sleep);
		finishedMutex.lock();
		pwds = crack_status.counter / status_sleep;
		crack_status.counter = 0;

		if (crack_status.finished) {
			finishedMutex.unlock();
			break;
		}
		finishedMutex.unlock();

		pwdMutex.lock();
		printf("Probing: '%s' [%d pwds/sec]\n", crack_status.password, pwds);
		pwdMutex.unlock();

		save_status();
	}
}

void crack_thread() {
	char* current;
	char ret[200];
	char cmd[400];
	FILE* Pipe;

	while (true) {
		current = nextpass();

#ifdef _WIN32
		snprintf(cmd, sizeof(cmd), "unrar.exe t -y -p%s \"%s\" 2>&1", current, filename);
#else
		snprintf(cmd, sizeof(cmd), "unrar t -y -p%s \"%s\" 2>&1", current, filename);
#endif

		Pipe = popen(cmd, "r");

		while (fgets(ret, sizeof(ret), Pipe)) {
			if (strcasestr(ret, "ok")) {
				strcpy(crack_status.password_good, current);

				finishedMutex.lock();
				crack_status.finished = 1;
				printf("GOOD: Password cracked: '%s'\n", current);
				finishedMutex.unlock();

				save_status();
				break;
			}
		}

		pclose(Pipe);

		finishedMutex.lock();
		crack_status.counter++;

		if (crack_status.finished) {
			finishedMutex.unlock();
			break;
		}
		finishedMutex.unlock();

		free(current);
	}
}

void crack(const char* file, int threads) {
	strcpy(filename, file);
	snprintf(statname, sizeof(statname), "%s.bin", filename);
	num_threads = threads;

	if (load_status()) return;

	strcpy(crack_status.ABC, default_ABC);
	crack_status.curr_len = 1;
	crack_status.counter = 0;
	crack_status.finished = 0;
	strcpy(crack_status.password, default_ABC);

	std::vector<std::thread> thread_pool;

	for (int i = 0; i < num_threads; i++) {
		thread_pool.push_back(std::thread(crack_thread));
	}

	std::thread status_thread_obj(status_thread);

	for (auto& th : thread_pool) {
		th.join();
	}

	status_thread_obj.join();
}

void status_to_json(const char* file) {
	char status_file[259];
	snprintf(status_file, sizeof(status_file), "%s.bin", file);

	FILE* fp = fopen(status_file, "rb");
	if (!fp) {
		printf("{\"error\": \"Status file not found.\"}\n");
		return;
	}

	CrackStatus status;
	if (fread(&status, sizeof(CrackStatus), 1, fp) != 1) {
		printf("{\"error\": \"Failed to read status file.\"}\n");
		fclose(fp);
		return;
	}

	fclose(fp);

	printf("{\n");
	printf("  \"ABC\": \"%s\",\n", status.ABC);
	printf("  \"current_password\": \"%s\",\n", status.password);
	printf("  \"password_good\": \"%s\",\n", status.password_good);
	printf("  \"current_length\": %u,\n", status.curr_len);
	printf("  \"tested_count\": %ld,\n", status.counter);
	printf("  \"finished\": %d\n", status.finished);
	printf("}\n");
}
