/*
 *  mandelbrot.c
 *  Serves the mandelbrot fractal based on parameters in the url
 *  or serves an existing js viewer as a backup
 *
 *  Andrew Yu & Alex Huang 
 *  16/04/17 
 */

#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include "pixelColor.h"
#include "mandelbrot.h"

#define SIMPLE_SERVER_VERSION 1.0
#define REQUEST_BUFFER_SIZE 1000
#define DEFAULT_PORT 1917
#define BYTES_PER_PIXEL 3
#define NUMBER_PLANES 1
#define BITS_PER_PIXEL (BYTES_PER_PIXEL*8)
#define PIX_PER_METRE 2835
#define MAGIC_NUMBER 0x4d42 // BM 
#define NO_COMPRESSION 0
#define BMP_HEADER_SIZE 54 
#define DIB_HEADER_SIZE 40 
#define NUM_COLORS 0
#define SIZE 512

#define MAX_ITERATIONS 256


typedef struct _complexNumber{
   double real; 
   double imaginary; 
} complexNumber;


int waitForConnection (int serverSocket);
int makeServerSocket (int portno);
void serveBMP (int socket, double inputX, double inputY, int zoom); 
static void serveHTML (int socket);
double getDistance(double x1, double y1, double x2, double y2);
int escapeSteps(double x, double y);
void writeHeader(char *header); 
int main (int argc, char *argv[]) {
      
   printf ("************************************\n");
   printf ("Starting simple server %f\n", SIMPLE_SERVER_VERSION);
   printf ("Serving bmps since 2012\n");   
   
   int serverSocket = makeServerSocket (DEFAULT_PORT);   
   printf ("Access this server at http://localhost:%d/\n", 
           DEFAULT_PORT);
   printf ("************************************\n");
   
   char request[REQUEST_BUFFER_SIZE];
   int numberServed = 0;
   while (1) {
      
      printf ("*** So far served %d pages ***\n", numberServed);
      
      int connectionSocket = waitForConnection (serverSocket);
      // wait for a request to be sent from a web browser, open a new
      // connection for this conversation
      
      // read the first line of the request sent by the browser  
      int bytesRead;
      bytesRead = read (connectionSocket, request, (sizeof request)-1);
      assert (bytesRead >= 0); 
      // were we able to read any data from the connection?
      // print entire request to the console 
      printf (" *** Received http request ***\n %s\n", request);
      
      // parse request, example: GET /tile_x-1.0_y0.5_z8.bmp HTTP/1.1
      // delimit request 
      char *token;
      double inputX, inputY; 
      int zoom;
      token = strtok(request, "/");
      // the null parameter indicates strtok to return the next token 
      token = strtok(NULL, "/");
      
      if(sscanf(token,
         "tile_x%lf_y%lf_z%d.bmp HTTP",
         &inputX, &inputY, &zoom) == 3) {
            printf("X: %f, Y:%f, Z:%d\n", inputX, inputY, zoom);
            // Return mandelbrot set seeded with appropriate 
            // parameters 
            serveBMP(connectionSocket, inputX, inputY, zoom);
         } else {
            serveHTML(connectionSocket);
            
         }  

      // close the connection after sending the page
      close(connectionSocket);
      numberServed++;
   } 
   
   // close the server connection after we are done
   printf ("** shutting down the server **\n");
   close (serverSocket);
   
   return EXIT_SUCCESS; 
}

void serveBMP (int socket, double inputX, double inputY, int zoom) {
   char* message;
   
   // first send the http response header
   
   // (if you write strings one after another like this on separate
   // lines the c compiler kindly joins them togther for you into
   // one long string)
   message = "HTTP/1.0 200 OK\r\n"
                "Content-Type: image/bmp\r\n"
                "\r\n";
   printf ("about to send=> %s\n", message);
   write (socket, message, strlen (message));

   // Declare Char Array
   // BMP Size is irrelevant
   unsigned char headerArray[] = 
   {'B', 'M', 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 
    0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x20, 0x00, 0x36, 0x00, 0x0b, 0x13,
    0x00, 0x00, 0x0b, 0x13, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
   // Send Header
   write (socket, headerArray, sizeof(headerArray));
   
   // Send BMP Data
   unsigned char bmpData[SIZE*SIZE*BYTES_PER_PIXEL];
   int row = 0;
   int col = 0;
   int counter = 0; // Keep track of pixel array counter 
   
   // Distance between pixels relative to zoom 
   double pixelDistance = 1 / pow(2, zoom);
   
   double x;
   double y;
   
   while(col < SIZE){
      row = 0;
      while(row < SIZE){
         // printf("X:%d Y:%d\n", x-sizeX, y-sizeY);
         // bmpData[counter] = escapeSteps(x, y);
         
         // 'centre' point 
         x = inputX -  (SIZE / 2) * pixelDistance;
         y = inputY - (SIZE / 2) * pixelDistance;
         
         // adjust relative to zoom 
         x += row * pixelDistance; 
         y += col * pixelDistance; 
         
         int steps = escapeSteps(x, y);
         
         // write 3 bytes for R G B 
         bmpData[counter] = stepsToRed(steps);
         counter++;
         bmpData[counter] = stepsToGreen(steps);
         counter++;
         bmpData[counter] = stepsToBlue(steps);
         counter++;
         
         x+= pixelDistance; 
         row++;
         
      }
      x = inputX - ( (SIZE / 2)-1) * pixelDistance;
      y -= pixelDistance; 
      col++;
   }
   
   printf ("sending bmp data...\n");
   // send bmp buffer
   // i opted to keep a buffer rather than send each pixel individually
   write(socket, bmpData, sizeof(bmpData));

}

static void serveHTML (int socket) {
   char* message;
 
   // first send the http response header
   message =
      "HTTP/1.0 200 Found\n"
      "Content-Type: text/html\n"
      "\n";
   printf ("about to send=> %s\n", message);
   write (socket, message, strlen (message));
 
   message =
      "<!DOCTYPE html>\n"
      "<script "
      "src=\"http://almondbread.cse.unsw.edu.au/tiles.js\">"
      "</script>"
      "\n";      
   write (socket, message, strlen (message));
}

// start the server listening on the specified port number
int makeServerSocket (int portNumber) { 
   
   // create socket
   int serverSocket = socket (AF_INET, SOCK_STREAM, 0);
   assert (serverSocket >= 0);   
   // error opening socket
   
   // bind socket to listening port
   struct sockaddr_in serverAddress;
   memset ((char *) &serverAddress, 0,sizeof (serverAddress));
   
   serverAddress.sin_family      = AF_INET;
   serverAddress.sin_addr.s_addr = INADDR_ANY;
   serverAddress.sin_port        = htons (portNumber);
   
   // let the server start immediately after a previous shutdown
   int optionValue = 1;
   setsockopt (
      serverSocket,
      SOL_SOCKET,
      SO_REUSEADDR,
      &optionValue, 
      sizeof(int)
   );

   int bindSuccess = 
      bind (
         serverSocket, 
         (struct sockaddr *) &serverAddress,
         sizeof (serverAddress)
      );
   
   assert (bindSuccess >= 0);
   // if this assert fails wait a short while to let the operating 
   // system clear the port before trying again
   
   return serverSocket;
}

// wait for a browser to request a connection,
// returns the socket on which the conversation will take place
int waitForConnection (int serverSocket) {
   // listen for a connection
   const int serverMaxBacklog = 10;
   listen (serverSocket, serverMaxBacklog);
   
   // accept the connection
   struct sockaddr_in clientAddress;
   socklen_t clientLen = sizeof (clientAddress);
   int connectionSocket = 
      accept (
         serverSocket, 
         (struct sockaddr *) &clientAddress, 
         &clientLen
      );
   
   assert (connectionSocket >= 0);
   // error on accept
   
   return (connectionSocket);
}






int escapeSteps(double x, double y){
   // z subscript n = ( z subscript n - 1 ) ^ 2 + c 
   // c = x + yi 
   // Get distance from origin to z subscript n 
   // if distance > 2 or steps >= 256, return 
   // ( x + yi )^2 = x^2 + 2xyi - y^2  
   // Therefore, z subscript n = x^2 - y^2 + 2xyi + a + bi 
   // where a and b equal the original x and y 
   
   complexNumber c;
   c.real = x; 
   c.imaginary = y; 
   
   complexNumber z; 
   z.real = 0; 
   z.imaginary = 0;
   
   int i = 0;
   double distance = 0;
   while( i < MAX_ITERATIONS && distance <= 2){
      double tempZReal = z.real; 
      z.real = z.real * z.real - z.imaginary * z.imaginary + c.real; 
      // Real: x^2 - y^2 + a 
      z.imaginary = 2 * tempZReal * z.imaginary + c.imaginary; 
      // Imaginary: 2xy + b 
      distance = getDistance(0, 0, z.real, z.imaginary);
      i++;
   }
   return i;
  
}

double getDistance(double x1, double y1, double x2, double y2){
   return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}