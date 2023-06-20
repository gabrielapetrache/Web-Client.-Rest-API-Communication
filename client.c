#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

/**
 * @brief Get the token from the server response
 * 
 * @param response the response
 * @return the token
 */
char *get_token(char *response) {
	char *token = calloc(BUFLEN, sizeof(char));
	char *p = strstr(response, "token");
	p += 8;

	int i = 0;

	/* Remove the quotes */
	while (*p != '"') {
		token[i++] = *p;
		p++;
	}

	return token;
}

/**
 * @brief Checks if a string is a number
 * 
 * @param s the string
 * @return int 1 if it is a number, 0 otherwise
 */
int is_number(char *s) {
	int i;
	for (i = 0; i < strlen(s); i++) {
		if (s[i] < '0' || s[i] > '9') {
			return 0;
		}
	}

	return 1;
}

/**
 * @brief Creates a new user if the server allows it
 * 
 * @param sockfd the socket file descriptor
 * @param username the username
 * @param password  the password
 * @param logged_in the logged in status
 */
void register_user(int sockfd, char *username, char *password, int logged_in) {
	/* If the user is already logged in, don't register */
	if (logged_in) {
			printf("You are already logged in\n");
			return;
	}

	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);

	username = calloc(100, sizeof(char));
	printf("username=");
	char *tmp = calloc(100, sizeof(char));
	scanf("%c", tmp);
	scanf("%[^\n]", username);
	// check for spaces in username
	if (strstr(username, " ") != NULL) {
		printf("Username cannot contain spaces. Please enter another command\n");
		free(username);
		return;
	}
	json_object_set_string(root_object, "username", username);
			
	password = calloc(100, sizeof(char));
	printf("password=");
	scanf("%c", tmp);
	scanf("%[^\n]", password);
	// check for spaces in password
	if (strstr(password, " ") != NULL) {
		printf("Password cannot contain spaces. Please enter another command\n");
		free(password);
		free(username);
		return;
	}
	json_object_set_string(root_object, "password", password);

	char* message = compute_post_request("34.254.242.81", "/api/v1/tema/auth/register", 
						"application/json", json_serialize_to_string_pretty(root_value), 2, NULL, 0, NULL, 0);

	send_to_server(sockfd, message);
	char *response = receive_from_server(sockfd);
			
	/* If the response is empty, try again */
	if (strlen(response) == 0) {
		sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
		send_to_server(sockfd, message);
		response = receive_from_server(sockfd);
	}
	printf("%s\n", response);
}

/**
 * @brief Logs in a user if the server allows it
 * 
 * @param sockfd the socket file descriptor
 * @param username the username
 * @param password the password
 * @param logged_in the logged in status
 * @return the cookies
 */
char **login_user(int sockfd, char *username, char *password, int *logged_in) {
	/* If the user is already logged in, don't login */
	if (*logged_in) {
		printf("You are already logged in\n");
		return NULL;
	}

	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);

	username = calloc(100, sizeof(char));
	password = calloc(100, sizeof(char));
	printf("username=");
	scanf("%s", username);
	json_object_set_string(root_object, "username", username);

	printf("password=");
	scanf("%s", password);
	json_object_set_string(root_object, "password", password);

	char* message = compute_post_request("34.254.242.81", "/api/v1/tema/auth/login", 
						"application/json", json_serialize_to_string_pretty(root_value), 2, NULL, 0, NULL, 0);
	
	send_to_server(sockfd, message);
	char *response = receive_from_server(sockfd);

	/* If the response is empty, try again */
	if (strlen(response) == 0) {
		sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
		send_to_server(sockfd, message);
		response = receive_from_server(sockfd);
	}

	char *cookie = NULL;
	char **cookies = NULL;
	if (strstr(response, "error")) {
		/* Failed to login */
		printf("%s\n", response);
	} else {
		printf("%s\n", response);
		/* Store the cookie */
		char *p = strstr(response, "Set-Cookie:");
		if (p != NULL) {
			p += 12; // skip "Set-Cookie: "
			char *q = strstr(p, ";"); // find the end of the cookie
			if (q != NULL) {
				int len = q - p;
				*q = '\0';
				cookie = calloc(len + 1, sizeof(char));
				strcpy(cookie, p);
				cookies = calloc(1, sizeof(char *));
				cookies[0] = calloc(len + 1, sizeof(char));
				strcpy(cookies[0], p);
				/* Successfully logged in */
				*logged_in = 1;
			}
		}
	}
	return cookies;
}

/**
 * @brief Sends a message to the server and returns the response
 * 
 * @param sockfd the socket file descriptor
 * @param message the message to be sent
 * @return the response
 */
char *get_response(int sockfd, char *message) {
	send_to_server(sockfd, message);
	char *response = receive_from_server(sockfd);

	/* If the response is empty, try again */
	while (strlen(response) == 0) {
		sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
		send_to_server(sockfd, message);
		response = receive_from_server(sockfd);
	}
	return response;
}

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;
	char **cookies = NULL;
	char *token = NULL;
	char **tokens = NULL;
	int logged_in = 0;
	int access_granted = 0;
	JSON_Value *root_value;
	JSON_Object *root_object;
	char *username = NULL;
	char *password = NULL;
	char *id = NULL;
	char *url = NULL;
	char *title = NULL;
	char *author = NULL;
	char *genre = NULL;
	char *publisher = NULL;
	char *page_count = NULL;
	char *tmpfile = NULL;

	sockfd = open_connection("34.254.242.81", 8080, AF_INET, SOCK_STREAM, 0);
	char command[100];

	while (1) {
		scanf("%s", command);

		if (!strcmp(command, "register")) {
			register_user(sockfd, username, password, logged_in);

		} else if (!strcmp(command, "login")) {
			cookies = login_user(sockfd, username, password, &logged_in);
		
		} else if (!strcmp(command, "enter_library")) {
			if (!logged_in) {
				printf("You are not logged in\n");
				continue;
			}

			if (access_granted) {
				printf("You already have access to the library\n");
				continue;
			}
	
			message = compute_get_request("34.254.242.81", "/api/v1/tema/library/access", NULL, cookies, 1, NULL, 0);
			response = get_response(sockfd, message);
			printf("%s\n", response);

			if (strstr(response, "error")) {
				/* Failed to get access */
				access_granted = 0;
			} else {
				access_granted = 1;
				/* Get the token */
				token = get_token(response);
				tokens = calloc(1, sizeof(char *));
				/* Add the token to the tokens array */
				tokens[0] = calloc(strlen(token) + 1, sizeof(char));
				strcpy(tokens[0], token);
			}

		} else if (!strcmp(command, "get_books")) {
			if (!logged_in) {
				printf("You are not logged in\n");
				continue;
			}

			if (!access_granted) {
				printf("You don't have access to the library\n");
				continue;
			}

			/* Get the token */
			tokens[0] = calloc(strlen(token) + 1, sizeof(char));
			strcat(tokens[0], "Bearer ");
			strcat(tokens[0], token);
			
			message = compute_get_request("34.254.242.81", "/api/v1/tema/library/books",
							 NULL, cookies, 1, tokens, 1);

			response = get_response(sockfd, message);
			printf("%s\n", response);

		} else if (!strcmp(command, "get_book")) {
			if (!logged_in) {
				printf("You are not logged in\n");
				continue;
			}

			if (!access_granted) {
				printf("You don't have access to the library\n");
				continue;
			}

			id = calloc(100, sizeof(char));
			printf("id=");
			scanf("%s", id);

			/* Construct the url */
			url = calloc(strlen("/api/v1/tema/library/books/") + strlen(id) + 1, sizeof(char));
			strcat(url, "/api/v1/tema/library/books/");
			strcat(url, id);

			tokens[0] = calloc(strlen(token) + 1, sizeof(char));
			strcat(tokens[0], "Bearer ");
			strcat(tokens[0], token);

			message = compute_get_request("34.254.242.81", url, NULL, cookies, 1, tokens, 1);
			response = get_response(sockfd, message);

			printf("%s\n", response);
		} else if (!strcmp(command, "add_book")) {
			if (!logged_in) {
				printf("You are not logged in\n");
				continue;
			}

			if (!access_granted) {
				printf("You don't have access to the library\n");
				continue;
			}
			root_value = json_value_init_object();
			root_object = json_value_get_object(root_value);

			title = calloc(100, sizeof(char));
			author = calloc(100, sizeof(char));
			genre = calloc(100, sizeof(char));
			publisher = calloc(100, sizeof(char));
			page_count = calloc(100, sizeof(char));
			tmpfile = calloc(100, sizeof(char));

			printf("title=");
			scanf("%c", tmpfile);
			scanf("%[^\n]", title); // Read the title with spaces
			json_object_set_string(root_object, "title", title);
			if (strlen(title) == 0) {
				printf("No title given. Please enter another command\n");
				continue;
			}
			printf("author=");
			scanf("%c", tmpfile);
			scanf("%[^\n]", author); // Read the author with spaces
			json_object_set_string(root_object, "author", author);
			if (strlen(author) == 0) {
				printf("No author given. Please enter another command\n");
				continue;
			}

			printf("genre=");
			scanf("%s", genre);
			json_object_set_string(root_object, "genre", genre);
			if (strlen(genre) == 0) {
				printf("No genre given. Please enter another command\n");
				continue;
			}

			printf("page_count=");
			scanf("%c", tmpfile);
			scanf("%[^\n]", page_count); // Read the page_count with spaces
			// Check if page_count is a number
			int page_count_int;
			if (sscanf(page_count, "%d", &page_count_int) != 1) {
				printf("Page count must be a number. Please enter another command\n");
				continue;
			}
			json_object_set_string(root_object, "page_count", page_count);
			
			if (strlen(page_count) == 0) {
				printf("No page_count given. Please enter another command\n");
				continue;
			}
			printf("publisher=");
			scanf("%c", tmpfile);
			scanf("%[^\n]", publisher); // Read the publisher with spaces
			json_object_set_string(root_object, "publisher", publisher);
			if (strlen(publisher) == 0) {
				printf("No publisher given. Please enter another command\n");
				continue;
			}

			tokens[0] = calloc(strlen(token) + 1, sizeof(char));
			strcat(tokens[0], "Bearer ");
			strcat(tokens[0], token);

			message = compute_post_request("34.254.242.81", "/api/v1/tema/library/books",
							 "application/json", json_serialize_to_string(root_value), 5, cookies, 1, tokens, 1);
			response = get_response(sockfd, message);
			printf("%s\n", response);
		} else if (!strcmp(command, "delete_book")) {
			if (!logged_in) {
				printf("You are not logged in\n");
				continue;
			}

			if (!access_granted) {
				printf("You don't have access to the library\n");
				continue;
			}

			id = calloc(100, sizeof(char));
			printf("id=");
			scanf("%s", id);

			/* Construct the url */
			url = calloc(strlen("/api/v1/tema/library/books/") + strlen(id) + 1, sizeof(char));
			strcat(url, "/api/v1/tema/library/books/");
			strcat(url, id);

			tokens[0] = calloc(strlen(token) + 1, sizeof(char));
			strcat(tokens[0], "Bearer ");
			strcat(tokens[0], token);

			message = compute_delete_request("34.254.242.81", url, NULL, cookies, 1, tokens, 1);
			response = get_response(sockfd, message);

			printf("%s\n", response);
		} else if (!strcmp(command, "logout")) {
			if (!logged_in) {
				printf("You are not logged in\n");
				continue;
			}
			
			message = compute_get_request("34.254.242.81", "/api/v1/tema/auth/logout", NULL, cookies, 1, NULL, 0);
			response = get_response(sockfd, message);
			printf("%s\n", response);
			logged_in = 0;
			access_granted = 0;
		} else if (!strcmp(command, "exit")) {
			break;
		} else {
			printf("Invalid command\n");
			continue;
		}
	}
	
	/* Free memory */
	close_connection(sockfd);
	free(token);
	free(message);
	free(response);
	free(username);
	free(password);
	free(title);
	free(author);
	free(genre);
	free(publisher);
	free(page_count);
	free(id);
	free(url);
	free(tmpfile);
	json_object_clear(root_object);
	json_value_free(root_value);	
	
    return 0;
}
