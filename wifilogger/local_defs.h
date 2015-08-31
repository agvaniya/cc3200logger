#ifndef LOCAL_DEFS_H_
#define LOCAL_DEFS_H_
typedef struct{
  char is_provisioned;
  char ssid[32];
  char pass[64];
  char cs;
} credentials_t;

typedef enum{ SEEN_NOTHING=0,
              READ_SSID=1,
              READ_PWD=2, 
              SEEN_REF=3, 
              SEEN_SSID=4, 
              SEEN_PWD=5
            } token_state_t;







#endif //LOCAL_DEFS_H
