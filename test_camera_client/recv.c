#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <mosquitto.h>

char camera_topic[128];
char camera_data_topic[128];
char filename[64];
int img_ind = 0;

void msg_callback(struct mosquitto *client, void *ptr, const struct mosquitto_message *msg) {
	printf("got msg in topic %s\n", msg->topic);
	int len = msg->payloadlen;
	unsigned char *payload = msg->payload;
	// for (int i = 0; i < len; i++) {
	// 	printf("%x ", payload[i]);
	// }
	// printf("\n");
	printf("new img %d\n", img_ind);

	sprintf(filename, "images/%d.jpeg", img_ind);
	img_ind++;
	FILE *f = fopen(filename, "wb");
	if (f) {
		fwrite(payload, 1, len, f);
		fclose(f);
	} else {
		printf("cannot open file\n");
	}
}

void print_help(char *prog_name) {
	printf("Usage: %s <host> <port> <listen_camera_name> <optional_user> <optional_pass>\n", prog_name);
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
	char *camera_name = argv[3];
	char *user = NULL;
	char *pass = NULL;
	if (argc > 5) {
		user = argv[4];
		pass = argv[5];
	}

	strcat(camera_topic, "cameras/");
	strcat(camera_topic, camera_name);

	strcat(camera_data_topic, camera_topic);
	strcat(camera_data_topic, "/data");

	struct mosquitto *client = mosquitto_new("test", 1, NULL);
	if (!client) {
		printf("cannot create client\n");
		mosquitto_lib_cleanup();
		return 3;
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

	r = mosquitto_subscribe(client, NULL, camera_data_topic, 1);
	if (r != MOSQ_ERR_SUCCESS) {
		printf("cannot sub\n");
		mosquitto_destroy(client);
		mosquitto_lib_cleanup();
		return 6;
	}

	while (1) {
		r = mosquitto_loop(client, 100, 1);
		if (r != MOSQ_ERR_SUCCESS) {
			printf("loop err: %s\n", strerror(r));
			return 8;
		}
	}

	return 0;
}
