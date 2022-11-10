#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <mosquitto.h>

char camera_topic[128];
char camera_state_topic[128];
char camera_data_topic[128];
char camera_cmd_topic[128];
char camera_dump_topic[128];

char *online_str = "online";
char *offline_str = "offline";

char *req_cmd = "req";
char *stop_cmd = "stop";
char *dump_cmd = "dump";

int camera_subs = 0;
long long last_dump_msec = 0;

struct mosquitto *client;

typedef struct {
	char *buf;
	int len;
} img_t;

img_t examples_arr[256];
int examples_arr_len = 0;
int curr_example_ind = 0;
char send_examples = 0;

int read_files(char *dir_name) {
	char fname[256];
	DIR* FD;
	struct dirent *in_file;
	FILE *entry_file;
	if (NULL == (FD = opendir(dir_name))) {
		fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
		return 1;
	}

	examples_arr_len = 0;
	while (in_file = readdir(FD)) {
		if (!strcmp(in_file->d_name, ".")) {
			continue;
		}
        if (!strcmp(in_file->d_name, "..")) {
			continue;
		}

		if (examples_arr_len == sizeof(examples_arr)) {
			printf("reached files limit, returning\n");
			return 0;
		}
		fname[0] = 0;
		strcat(fname, dir_name);
		strcat(fname, "/");
		strcat(fname, in_file->d_name);
		printf("reading file %s\n", fname);
		entry_file = fopen(fname, "r");
        if (entry_file == NULL) {
            fprintf(stderr, "Error : Failed to open entry file %s - %s\n", fname, strerror(errno));
            return 2;
        }
		fseek(entry_file, 0, SEEK_END);
		int size = ftell(entry_file);
		fseek(entry_file, 0, SEEK_SET);
		char *buf = malloc(size);
		fread(buf, 1, size, entry_file);
		img_t im = {
			.buf = buf,
			.len = size
		};
		examples_arr[examples_arr_len] = im;
		examples_arr_len++;
		fclose(entry_file);
	}
}

void gen_rand_arr(unsigned char *buf, int len) {
	for (int i = 0; i < len; i++) {
		buf[i] = rand() % 256;
	}
}

void dump_camera_topic(char *topic) {
	if (send_examples) {
		if (curr_example_ind == examples_arr_len) {
			curr_example_ind = 0;
		}
		img_t im = examples_arr[curr_example_ind];
		printf("%d %d %d\n", curr_example_ind, examples_arr_len, im.len);
		mosquitto_publish(client, NULL, topic, im.len, im.buf, 0, 0);
		curr_example_ind++;
	} else {
		int len = 5;
		unsigned char buf[len];
		gen_rand_arr(buf, len);
		mosquitto_publish(client, NULL, topic, len, buf, 0, 0);
	}
}

long long get_curr_msec(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

int is_cmd(unsigned char *payload, int len, unsigned char *cmd) {
	return len == strlen(cmd) && strcmp(payload, cmd) == 0;
}

void msg_callback(struct mosquitto *client, void *ptr, const struct mosquitto_message *msg) {
	printf("got msg in topic %s\n", msg->topic);
	int len = msg->payloadlen;
	unsigned char *payload = msg->payload;
	for (int i = 0; i < len; i++) {
		printf("%x ", payload[i]);
	}
	printf("\n");

	if (strcmp(msg->topic, camera_cmd_topic) == 0) {
		if (is_cmd(payload, len, req_cmd)) {
			camera_subs++;
		} else if (is_cmd(payload, len, stop_cmd)) {
			camera_subs--;
		} else if (is_cmd(payload, len, dump_cmd)) {
			dump_camera_topic(camera_dump_topic);
		}
	}
}

void print_help(char *prog_name) {
	printf("Usage: %s <host> <port> <client_name> <optional_user> <optional_pass> <optional_img_dir>\n", prog_name);
}

int main(int argc, char *argv[]) {
	if (argc < 4) {
		print_help(argv[0]);
		return 1;
	}
	int r;
	r = mosquitto_lib_init();
	if (r != MOSQ_ERR_SUCCESS) {
		printf("cannot init lib\n");
		return 2;
	}

	char *host = argv[1];
	int port = atoi(argv[2]);
	char *client_name = argv[3];
	char *user = NULL;
	char *pass = NULL;
	char *img_dir = NULL;
	if (argc > 5) {
		user = argv[4];
		pass = argv[5];
	}
	if (argc > 6) {
		img_dir = argv[6];
		send_examples = 1;
		r = read_files(img_dir);
		if (r) {
			printf("read files err %d\n", r);
			return 9;
		}
	}

	strcat(camera_topic, "cameras/");
	strcat(camera_topic, client_name);

	strcat(camera_state_topic, camera_topic);
	strcat(camera_state_topic, "/state");

	strcat(camera_data_topic, camera_topic);
	strcat(camera_data_topic, "/data");

	strcat(camera_cmd_topic, camera_topic);
	strcat(camera_cmd_topic, "/cmd");

	strcat(camera_dump_topic, camera_topic);
	strcat(camera_dump_topic, "/dump");


	client = mosquitto_new(client_name, 1, NULL);
	if (!client) {
		printf("cannot create client\n");
		mosquitto_lib_cleanup();
		return 3;
	}

	r = mosquitto_will_set(client, camera_state_topic, strlen(offline_str), offline_str, 1, 1);
	if (r != MOSQ_ERR_SUCCESS) {
		printf("cannot set will\n");
		mosquitto_destroy(client);
		mosquitto_lib_cleanup();
		return 4;
	}

	mosquitto_message_callback_set(client, msg_callback);
	if (user) {
		mosquitto_username_pw_set(client, user, pass);
	}

	r = mosquitto_connect(client, host, port, 10);
	if (r != MOSQ_ERR_SUCCESS) {
		printf("cannot connect to %s:%d (%s)\n", host, port, strerror(r));
		mosquitto_destroy(client);
		mosquitto_lib_cleanup();
		return 5;
	}

	r = mosquitto_subscribe(client, NULL, camera_cmd_topic, 1);
	if (r != MOSQ_ERR_SUCCESS) {
		printf("cannot sub\n");
		mosquitto_destroy(client);
		mosquitto_lib_cleanup();
		return 6;
	}

	r = mosquitto_publish(client, NULL, camera_state_topic, strlen(online_str), online_str, 1, 1);
	if (r != MOSQ_ERR_SUCCESS) {
		printf("cannot publish: %s\n", strerror(r));
		mosquitto_destroy(client);
		mosquitto_lib_cleanup();
		return 7;
	}

	while (1) {
		r = mosquitto_loop(client, 100, 1);
		if (r != MOSQ_ERR_SUCCESS) {
			printf("loop err: %s\n", strerror(r));
			return 8;
		}
		long long curr_msec = get_curr_msec();
		if (camera_subs && (curr_msec - last_dump_msec) >= 100) {
			last_dump_msec = curr_msec;
			dump_camera_topic(camera_data_topic);
			printf("sending data\n");
		}
	}

	return 0;
}
