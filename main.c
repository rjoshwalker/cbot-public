#include "socket.h"

#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

char server[] = "irc.chat.twitch.tv";
char port[]   = "6667";
char tail[]   = "\r\n";

char nick[128]    = "";
char oauth[128]   = "";
char channel[128] = "";
char program[2]   = "";

const char options[] =
		"-n nick\n"
		"-o oauth\n"
		"-c channel\n"
		"-p program\n"
		"-h shows this help page\n\n";

//char caps[][64] = {
//		"tags"
//};

static struct message {
	char user[64];
	char channel[64];
	char text[256];
};

void send_msg(socket_t *sock, char *msg)
{
	char send[512];
	snprintf(send, sizeof(send), "PRIVMSG #%s :%s%s", channel, msg, tail);
}

bool parse_msg(char *msg, struct message *ret)
{
	if (sscanf(msg, ":%63[^!]!%*[^ ] PRIVMSG #%63[^ ] :%255[^\r\n]", ret->user, ret->channel, ret->text) != 3)
		return false;

	return true;
}

int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i = i + 2) {
		char *x = argv[i];
		char *y;

		if (i + 1 > argc || argv[i+1][0] == '-') {
			printf("%s", options);
			return 0;
		} else {
			y = argv[i+1];
		}

		if (strcmp(x, "-n") == 0) {
			snprintf(nick, sizeof(nick), "%s", y);
		} else if (strcmp(x, "-o") == 0) {
			snprintf(oauth, sizeof(oauth), "%s", y);
		} else if (strcmp(x, "-c") == 0) {
			snprintf(channel, sizeof(channel), "%s", y);
		} else if (strcmp(x, "-p") == 0) {
			snprintf(program, sizeof(program), "%s", y);
		} else {
			printf("%s", options);
			return 0;
		}
	}

	printf("STARTING! Program[%s] Nick[%s] Channel[%s]\n", program, nick, channel);



	socket_t *sock = _socket(server, port);

	if (!sock->connect(sock, 1)) {
		fprintf(stderr, "FAILED TO CONNECT!\n");
		return -1;
	}

	printf("CONNECTED!\n");

	char woauth[256];
	snprintf(woauth, sizeof(woauth), "PASS %s%s", oauth, tail);
	sock->write_n(sock, woauth, strlen(woauth));

	char wnick[256];
	snprintf(wnick, sizeof(wnick), "NICK %s%s", nick, tail);
	sock->write_n(sock, wnick, strlen(wnick));

	char buf[512];
	sock->read_line(sock, buf, sizeof(buf));

	if (strstr(buf, "Welcome") == NULL) {
		fprintf(stderr, "FAILED TO LOGIN!\n");
		sock->close(sock);
		return -2;
	}

	printf("LOGGED IN!\n");

	while (strstr(buf, ":>") == NULL) {
		sock->read_line(sock, buf, sizeof(buf));
	}


//	for (int j = 0; j < (sizeof(caps)/sizeof(caps[0])); ++j) {
//		char cap[128];
//		snprintf(cap, sizeof(cap), "CAP REQ :twitch.tv/%s%s", caps[j], tail);
//		sock->write_n(sock, cap, strlen(cap));
//
//		sock->read_line(sock, buf, sizeof(buf));
//
//		if (strstr(buf, caps[j]) == NULL) {
//			fprintf(stderr, "FAILED ADDING CAP [%s]\n", caps[j]);
//			return -4;
//		} else {
//			printf("ADDED CAP [%s]\n", caps[j]);
//		}
//	}


	char join[256];
	snprintf(join, sizeof(join), "JOIN #%s%s", channel, tail);
	sock->write_n(sock, join, strlen(join));

	sock->read_line(sock, buf, sizeof(buf));
	if (strstr(buf, "JOIN") == NULL) {
		fprintf(stderr, "FAILED TO JOIN CHANNEL!\n");
		sock->close(sock);
		return -3;
	}

	printf("JOINED CHANNEL!\n");

	while (strstr(buf, "End of /NAMES list") == NULL) {
		sock->read_line(sock, buf, sizeof(buf));
	}


	while (1) {
		fd_set rd;
		FD_ZERO(&rd);
		FD_SET(sock->fd, &rd);

		/* Max fd */
		int nfd = sock->fd+1;

		/* Select from read set */
		int s = select(nfd, &rd, NULL, NULL, NULL);

		/* Try again */
		if (s == -1 && errno == EINTR)
			continue;

		/* Critical */
		if (s == -1) {
			fprintf(stderr, "Loop Select Failure!\n");
			break;
		}


		if (FD_ISSET(sock->fd, &rd)) {
			sock->read_line(sock, buf, sizeof(buf));

			if (strstr(buf, "PING :") != NULL) {
				char repeat[256];
				sscanf(buf, "PING %255[^\r\n]", repeat);
				char pong[256];
				snprintf(pong, sizeof(pong), "PONG %s%s", repeat, tail);
				sock->write_n(sock, pong, strlen(pong));
				printf("RECEIVED PING, SENT PONG!\n");
			} else if (strstr(buf, "PRIVMSG") != NULL) {
				struct message recmsg;
				if (parse_msg(buf, &recmsg))
					printf("MSG [%s] (%s)\n", recmsg.user, recmsg.text);
				else
					printf("Unable to parse message!\n");
			}
		}
	}

	return 0;
}
