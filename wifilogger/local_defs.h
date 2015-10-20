#ifndef LOCAL_DEFS_H_
#define LOCAL_DEFS_H_

#define PASS_LEN 64
#define SSID_LEN 32

typedef struct{
  char is_provisioned;
  char ssid[SSID_LEN];
  char pass[PASS_LEN];
  char cs;
} credentials_t;

typedef enum{ SEEN_NOTHING=0,
              READ_SSID=1,
              READ_PWD=2, 
              SEEN_REF=3, 
              SEEN_SSID=4, 
              SEEN_PWD=5
            } token_state_t;
#define BIT2G 0.156f

const char REF_TOKEN[] = "s.cgi?";
const char SSID_TOKEN[] = "ssid=";
const char PWD_TOKEN[] = "pwd=";





#endif //LOCAL_DEFS_H
