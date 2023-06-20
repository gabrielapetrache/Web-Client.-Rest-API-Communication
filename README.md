# Web client. REST API
### Petrache Gabriela Andreea

## Introduction
The purpose of this homework was to implement a web client that can communicate with a web server using HTTP protocol. The communication is done using REST API.

## Implementation
For my implementation, I used the `parson` library for parsing the JSON responses from the server, because of its intuitive interface. 
The client is implemented in `client.c`. The client can make the following requests: `register`, `login`, `enter_library`, `get_books`, `get_book`, `add_book`, `delete_book`, `logout`. The client can also exit the application by typing `exit`. Here is a short description of each request:
* `register` - reads the username and password from STDIN and sends a POST request to the server to register the user. If the user is already registered, the server will return an error. It also checks to see if another user is already logged in. If so, it will return an error message and not proceed with the request.
* `login` - reads the username and password from STDIN and sends a POST request to the server to login the user. If the user doesn't exist the server will return an error. At this point, if the login is successfull, the server will provide the current user with a cookie. The client will store this cookie and use it for the next requests. If the user is already logged in, the server will return an error message and not proceed with the request.
* `enter_library` - sends a GET request to the server to enter the library. If the user is not logged in, the server will return an error. If the user is already logged in, the server will provide the current user with a JWT token. The client will store this token and use it for the next requests.
* `get_books` - sends a GET request to the server to get all the books from the library. If the user is already logged in, the server will return a JSON response containing all the books from the library.
* `get_book` - reads the id of the book from STDIN and sends a GET request to the server to get the book with the specified id. The server will return a JSON response containing the information regarding the book with the specified id.
* `add_book` - reads the title, author, genre and publisher of the book from STDIN and sends a POST request to the server to add the book to the library. The title, author and publisher are strings that can contain spaces. The page count must be a positive integer.
* `delete_book` - reads the id of the book from STDIN and sends a DELETE request to the server to delete the book with the specified id.
* `logout` - sends a GET request to the server to logout the current user. The library access will be revoked.
* `exit` - exits the application and frees the allocated memory.

## Usage
To compile the application, run `make`. An executable named `client` will be created. To run the application, run `./client`. The client will wait for commands from STDIN(`register`, `login`, `enter_library`, `get_books`, `get_book`, `add_book`, `delete_book`, `logout`, `exit`). The client will send the requests to the server and print the responses to STDOUT. To exit the application, type `exit`.

