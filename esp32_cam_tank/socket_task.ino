/*
   Communication task - handles socket communication and processes incoming commands
*/


// ==========================================================================
// Task constants
//

//
// Command processor state
//
#define CMD_ST_IDLE 0
#define CMD_ST_CMD 1
#define CMD_ST_VAL 2

#define TERM_CHAR 0x0D


//
// Write length - images are sent in chunks so we can look for incoming data
// between writes for better responsiveness
//
#define MAX_WRITE_LEN 4096



// ==========================================================================
// Task variables
//

//
// Command processor variables
//
int cmd_state = CMD_ST_IDLE;
char cmd_op;
int cmd_arg;
int cmd_val;
bool cmd_has_val;
bool val_is_neg;



// ==========================================================================
// Task
//
void socket_task(void * parameter)
{
  int socket_img_index = 0;   // Current ping-pong buffer index;

  Serial.println("socket_task starting");

  // Tasks never complete
  while (true) {
    // Wait for something to connect to our WiFi
    //    while (!wifi_connected()) {
    //      delay(100);
    //    }
    Serial.println("Device connected to WiFi");

    // Start the socket server
    wifiServer.begin();

    // Wait for the client to open a connection
    while (!wifiServer.hasClient()) {
      delay(25);
    }
    Serial.println("Device connected to socket");
    WiFiClient client = wifiServer.available();

    if (client) {
      is_connected = true;
      while (client.connected()) {
        // Process any commands from the remote controller
        process_rx_data(&client);

        // Look for images to send
        if (imgs[socket_img_index].img_ready) {
          process_image(&client, socket_img_index);
          imgs[socket_img_index].img_ready = false;
          socket_img_index = (socket_img_index == 0) ? 1 : 0;
        }

        // Allow scheduler (WDT) to run
        delay(10);
      }
    }

    Serial.println("Client disconnected");
    client.stop();  // Free the resource
    is_connected = false;
    socket_img_index = 0;
  }
}



// ==========================================================================
// Task routines
//

// Call frequently to handle incoming commands
void process_rx_data(WiFiClient* client)
{
  char c;

  while (client->available()) {
    c = client->read();
    process_rx_char(c);
  }
}


void process_rx_char(char c)
{
  int v;

  switch (cmd_state) {
    case CMD_ST_IDLE:
      if ((c == 'L') || (c == 'M')) {
        cmd_op = c;
        cmd_has_val = false;
        cmd_arg = 0;
        cmd_state = CMD_ST_CMD;
      }
      break;

    case CMD_ST_CMD:
      if (c == TERM_CHAR) {
        cmd_state = CMD_ST_IDLE;
      } else {
        if ((v = is_valid_dec(c)) >= 0) {
          cmd_arg = (cmd_arg * 10) + v;
        } else if (c == '=') {
          cmd_state = CMD_ST_VAL;
          cmd_val = 0;
          cmd_has_val = true;
          val_is_neg = false;
        } else if (c != ' ') {
          cmd_state = CMD_ST_IDLE;
          Serial.println("Illegal command");
        }
      }
      break;

    case CMD_ST_VAL:
      if (c == TERM_CHAR) {
        if (val_is_neg) cmd_val = -1 * cmd_val;
        process_command();
        cmd_state = CMD_ST_IDLE;
      } else {
        if ((v = is_valid_dec(c)) >= 0) {
          cmd_val = (cmd_val * 10) + v;
        } else if (c == '-') {
          val_is_neg = true;
        } else if (c != ' ') {
          cmd_state = CMD_ST_IDLE;
          Serial.println("Illegal command");
        }
      }
      break;

    default:
      cmd_state = CMD_ST_IDLE;
  }
}


// Returns -1 for invalid decimal character, 0 - 9 for valid decimal number
int is_valid_dec(char c) {
  if ((c >= '0') && (c <= '9')) {
    return (c - '0');
  }
  return -1;
}


void process_command()
{
  //Serial.printf("%c %d = %d\n", cmd_op, cmd_arg, cmd_val);
  switch (cmd_op) {
    case 'L':
      if (cmd_val == 0) {
        led_set_headlight_off();
      } else {
        led_set_headlight_on();
      }
      break;

    case 'M':
      if ((cmd_arg > 0) && (cmd_arg < 3)) {
        motor_set(cmd_arg, cmd_val);
      }
      break;
  }
}


void process_image(WiFiClient* client, int n)
{
  int remain_len;
  int write_len;
  int written_len;
  uint8_t* readP;

  // Setup to send an image
  remain_len = imgs[n].img_len;
  readP = imgs[n].img;

  // Chunk image and check for incoming data between chunks for improved responsiveness
  while (remain_len > 0) {
    write_len = (remain_len > MAX_WRITE_LEN) ? MAX_WRITE_LEN : remain_len;
    if ((written_len = client->write(readP, write_len)) != write_len) {
      Serial.printf("write failed: attempted %d, sent %d\n", write_len, written_len);
      return;
    }
    remain_len -= write_len;
    readP += write_len;

    // Check for incoming data
    process_rx_data(client);
  }
}
