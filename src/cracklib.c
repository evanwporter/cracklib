#include "../include/cracklib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

char default_ABC[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
char filename[255];
char statname[259];
int num_threads = 2;

pthread_mutex_t pwdMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t finishedMutex = PTHREAD_MUTEX_INITIALIZER;

CrackStatus crack_status;

void save_status() {
	FILE* file = fopen(statname, "wb");
	if (!file) return;
	pthread_mutex_lock(&finishedMutex);
	fwrite(&crack_status, sizeof(CrackStatus), 1, file);
	pthread_mutex_unlock(&finishedMutex);
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
	char* ok = malloc(PWD_LEN + 1);
	pthread_mutex_lock(&pwdMutex);
	strcpy(ok, crack_status.password);
	nextpass2(crack_status.password, crack_status.curr_len - 1);
	pthread_mutex_unlock(&pwdMutex);
	return ok;
}

void* status_thread() {
	int pwds;
	const short status_sleep = 3;
	while (1) {
		sleep(status_sleep);
		pthread_mutex_lock(&finishedMutex);
		pwds = crack_status.counter / status_sleep;
		crack_status.counter = 0;
		if (crack_status.finished) break;
		pthread_mutex_unlock(&finishedMutex);
		pthread_mutex_lock(&pwdMutex);
		printf("Probing: '%s' [%d pwds/sec]\n", crack_status.password, pwds);
		pthread_mutex_unlock(&pwdMutex);
		save_status();
	}
	return NULL;
}

void* crack_thread() {
	char* current;
	char ret[200];
	char cmd[400];
	FILE* Pipe;

	while (1) {
		current = nextpass();
		sprintf(cmd, "unrar t -y -p%s %s 2>&1", current, filename);
		Pipe = popen(cmd, "r");

		while (fgets(ret, 200, Pipe)) {
			if (strcasestr(ret, "ok")) {
				strcpy(crack_status.password_good, current);
				pthread_mutex_lock(&finishedMutex);
				crack_status.finished = 1;
				printf("GOOD: Password cracked: '%s'\n", current);
				pthread_mutex_unlock(&finishedMutex);
				save_status();
				break;
			}
		}
		pclose(Pipe);

		pthread_mutex_lock(&finishedMutex);
		crack_status.counter++;
		if (crack_status.finished) {
			pthread_mutex_unlock(&finishedMutex);
			break;
		}
		pthread_mutex_unlock(&finishedMutex);
		free(current);
	}
	return NULL;
}

void crack(const char* file, int threads) {
	strcpy(filename, file);
	sprintf(statname, "%s.bin", filename);
	num_threads = threads;

	if (load_status()) return;

	strcpy(crack_status.ABC, default_ABC);
	crack_status.curr_len = 1;
	crack_status.counter = 0;
	crack_status.finished = 0;
	strcpy(crack_status.password, default_ABC);

	pthread_t th[12];

	for (unsigned int i = 0; i < num_threads; i++) {
		pthread_create(&th[i], NULL, crack_thread, NULL);
	}

	pthread_create(&th[12], NULL, status_thread, NULL);

	for (unsigned int i = 0; i < num_threads; i++) {
		pthread_join(th[i], NULL);
	}

	pthread_join(th[12], NULL);
}
